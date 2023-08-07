/* A simple echo server using TCP */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#define SERVER_TCP_PORT 3000 /* well-known port */
#define BUFLEN 256			 /* buffer length */
#define FILE_BUFLEN 100		 // Since we will read 100 bytes at a time

int echod(int);
void reaper(int);

int main(int argc, char **argv)
{
	int sd, new_sd, client_len, port;
	struct sockaddr_in server, client;

	switch (argc)
	{
	case 1:
		port = SERVER_TCP_PORT;
		break;
	case 2:
		port = atoi(argv[1]);
		break;
	default:
		fprintf(stderr, "Usage: %d [port]\n", argv[0]);
		exit(1);
	}

	/* Create a stream socket	*/
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		fprintf(stderr, "Can't creat a socket\n");
		exit(1);
	}

	/* Bind an address to the socket	*/
	bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(sd, (struct sockaddr *)&server, sizeof(server)) == -1)
	{
		fprintf(stderr, "Can't bind name to socket\n");
		exit(1);
	}

	/* queue up to 5 connect requests  */
	listen(sd, 5);

	(void)signal(SIGCHLD, reaper);

	while (1)
	{
		client_len = sizeof(client);
		new_sd = accept(sd, (struct sockaddr *)&client, &client_len);
		if (new_sd < 0)
		{
			fprintf(stderr, "Can't accept client \n");
			exit(1);
		}
		switch (fork())
		{
		case 0: /* child */
			(void)close(sd);
			exit(echod(new_sd));
		default: /* parent */
			(void)close(new_sd);
			break;
		case -1:
			fprintf(stderr, "fork: error\n");
		}
	}
}

// A helper function to check filesize

int findSize(char file_name[])
{
	// opening the file in read mode
	FILE *fp = fopen(file_name, "r");

	// checking if the file exist or not
	if (fp == NULL)
	{
		printf("File Not Found!\n");
		return -1;
	}

	fseek(fp, 0L, SEEK_END);

	// calculating the size of the file
	int res = ftell(fp);

	// closing the file
	fclose(fp);

	return res;
}

// helper function to read data from a file char by char
void readFile(FILE *fPtr, char read_buffer[])
{
	char ch;
	int i = 0;

	while ((ch = fgetc(fPtr)) != EOF)
	{
		// ch = fgetc(fPtr);

		putchar(ch); // for debugging
		fflush(stdout);
		read_buffer[i] = ch;
		i++;
	}
	read_buffer[i] = '\0';
}

void show_char_arr(int num_of_bytes, char arr[])
{
	int i = 0;
	char ch;
	for (i = 0; i < num_of_bytes; i++)
	{
		ch = arr[i];
		putchar(ch);
		fflush(stdout);
	}
}
/*	echod program	*/
int echod(int sd)
{
	char *bp, buf[BUFLEN], filename[BUFLEN], final_filename[BUFLEN];
	int n, i, bytes_to_read;
	long file_size;
	char *file_data_buf; // Used to read in the data char by char from a opened file
	char *file_data_buf_rem;
	FILE *fp;
	int network_file_size = 0;
	int n_full_chunk; // Number of 100 byte chunks
	int rem_bytes;	  // Remaining bytes after sending all the 100 byte chunks

	read(sd, filename, BUFLEN); // Reading the filename from the client
	printf("Received filename is: %s\n", filename);
	fflush(stdout);
	write(sd, filename, strlen(filename) + 1); // Echoing back the file name to the client; adding a one offset to account for null char

	// Now we check whether this file exixts on the server end

	if (access(filename, F_OK) == 0)
	{
		// file exists
		printf("File exixts at the server side\n");
		fflush(stdout);
		// Sending to the client the error code. In this case file exists. So we are sen

		// formating the filepath
		sprintf(final_filename, "./%s", filename);
		// Now open the file
		if ((fp = fopen(final_filename, "r")) == NULL)
		{
			printf("Error! opening file\n");
			fflush(stdout);
			// Program exits if the file pointer returns NULL.
			exit(1);
		}

		// Moving the fptr to the end of the file
		fseek(fp, 0L, SEEK_END);
		// calculating the size of the file
		file_size = ftell(fp) + 2;
		// Move the file pointer to the start before sending the file content
		fseek(fp, 0, SEEK_SET);

		// send the filesize info. to the server
		network_file_size = htonl(file_size);
		write(sd, &network_file_size, sizeof(network_file_size));

		// iMPORTANT: implementing the dynamic file size logic

		n_full_chunk = file_size / FILE_BUFLEN;
		rem_bytes = file_size % FILE_BUFLEN;

		file_data_buf = malloc(sizeof(char) * file_size);
		readFile(fp, file_data_buf);
		bp = file_data_buf;

		for (i = 0; i < n_full_chunk; i++)
		{

			write(sd, bp, FILE_BUFLEN);
			printf("After Write full pass no. %d\n", i + 1);
			fflush(stdout);
			if (i < n_full_chunk)
			{
				bp = bp + FILE_BUFLEN;
			}
		}

		//  Now we send the num of remaining bytes
		if (rem_bytes > 0)
		{
			bp = file_data_buf + (n_full_chunk * FILE_BUFLEN);
			printf("Now in the rem_bytes block, rem bytes = %d\n", rem_bytes);
			fflush(stdout);
			write(sd, bp, rem_bytes);
			printf("After rem Write\n ");
			fflush(stdout);
		}

		free(file_data_buf);
		fclose(fp);
	}
	else
	{
		// file doesn't exist
		printf("No such File exixts at the server side\n");
		fflush(stdout);
	}

	close(sd);

	return (0);
}

/*	reaper		*/
void reaper(int sig)
{
	int status;
	while (wait3(&status, WNOHANG, (struct rusage *)0) >= 0)
		;
}
