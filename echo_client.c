/* A simple echo client using TCP */
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#define SERVER_TCP_PORT 3000 /* well-known port */
#define BUFLEN 256			 /* buffer length */
#define FILE_BUFLEN 100		 // Since we will read 100 bytes at a time

int main(int argc, char **argv)
{
	int n, i, j, bytes_to_read;
	int sd, port;
	struct hostent *hp;
	struct sockaddr_in server;
	char *host, *bp, rbuf[BUFLEN], sbuf[BUFLEN], filename[BUFLEN], final_filename[BUFLEN], filename_from_server[BUFLEN];
	FILE *fp;
	int return_status;
	int n_full_chunk; // Number of 100 byte chunks
	int rem_bytes;	  // Remaining bytes after reading all the 100 byte chunks

	int file_size; // This will store the total file size in bytes which the server may have
				   // This variable will help us reading multiple 100 byte chunks
				   // if file size is greater or less than 100 bytes

	char *file_data_read_buffer; // Important: This array to be initialized dynamically

	switch (argc)
	{
	case 2:
		host = argv[1];
		port = SERVER_TCP_PORT;
		break;
	case 3:
		host = argv[1];
		port = atoi(argv[2]);
		break;
	default:
		fprintf(stderr, "Usage: %s host [port]\n", argv[0]);
		exit(1);
	}

	/* Create a stream socket	*/
	if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		fprintf(stderr, "Can't creat a socket\n");
		exit(1);
	}

	bzero((char *)&server, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	if (hp = gethostbyname(host))
		bcopy(hp->h_addr, (char *)&server.sin_addr, hp->h_length);
	else if (inet_aton(host, (struct in_addr *)&server.sin_addr))
	{
		fprintf(stderr, "Can't get server's address\n");
		exit(1);
	}

	/* Connecting to the server */
	if (connect(sd, (struct sockaddr *)&server, sizeof(server)) == -1)
	{
		fprintf(stderr, "Can't connect \n");
		exit(1);
	}

	printf("Enter the file name: \n"); // AA: my just added
	fflush(stdout);

	gets(filename); // read string, filename retrieved
	fflush(stdin);
	// Now we gotta send this filename to the server
	write(sd, filename, strlen(filename) + 1); // AA: IMPORTANT: we are sending the filename to the server. Adding 1 to take the null char into account
	// Now we read the sent filename back here to verify
	printf("Now reading back the sent filename from the server: \n");
	fflush(stdout);

	bp = filename_from_server;
	bytes_to_read = strlen(filename) + 1; // IMPORTANT: Adding 1 to take the null char into account
	while ((i = read(sd, bp, bytes_to_read)) > 0)
	{
		bp += i;
		bytes_to_read -= i;
	}
	printf("Filename echoed back from server: %s\n", filename_from_server);
	fflush(stdout);

	// IMPORTANT: Reading the file size from the server
	file_size = 0;
	return_status = read(sd, &file_size, sizeof(file_size));
	if (return_status > 0)
	{

		file_size = ntohl(file_size);
		printf("Received file size info from the server: %d\n", file_size);
		fflush(stdout);
	}
	else
	{
		// Handling erros here
	}

	n_full_chunk = file_size / FILE_BUFLEN;
	rem_bytes = file_size % FILE_BUFLEN;

	file_data_read_buffer = malloc(sizeof(char) * file_size);

	bp = file_data_read_buffer;
	for (i = 0; i < n_full_chunk; i++)
	{
		bytes_to_read = FILE_BUFLEN;
		read(sd, bp, bytes_to_read);
		bp = bp + FILE_BUFLEN;
		printf("After the pass %d of full chunk reading\n", i + 1);
		fflush(stdout);
	}
	if (rem_bytes > 0)
	{
		bytes_to_read = rem_bytes;
		read(sd, bp, bytes_to_read);
		printf("After the read of remaining bytes\n");
		fflush(stdout);
	}

	// Now append the read data into the file named 'filename_from_server'
	sprintf(final_filename, "./%s", filename_from_server);
	fp = fopen(final_filename, "a"); // IMPORTANT: This should create a new file if not exist

	if (fp == NULL)
	{
		/* Unable to open file hence exit */
		printf("\nUnable to open '%s' file.\n", final_filename);
		fflush(stdout);
		exit(1);
	}

	fprintf(fp, "%s", file_data_read_buffer);
	fclose(fp);

	free(file_data_read_buffer); 

	close(sd);
	return (0);
}
