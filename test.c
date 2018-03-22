#include<stdio.h>
#include<stdlib.h>
#include<errno.h>
#include<fcntl.h>
#include<string.h>
#include<unistd.h>
 
#define BUFFER_LENGTH 1024
static char receive[BUFFER_LENGTH];
 
int main() {

	int ret, fd, exit;
	char stringToSend[BUFFER_LENGTH];

	printf("Starting device test code example...\n");

	fd = open("/dev/chardevice", O_RDWR);

	if (fd < 0) {
		perror("Failed to open the device...");
		return errno;
	}

	do {
		printf("\nType in a string to send to the kernel module (press ENTER to skip):\n");
		fgets(stringToSend, BUFFER_LENGTH, stdin);
		
		if (stringToSend[strlen(stringToSend) - 1] == '\n')
			stringToSend[strlen(stringToSend) - 1] = '\0';
			
		if (strcmp(stringToSend, "") != 0) {
			printf("Writing message to the device [%s].\n", stringToSend);
			ret = write(fd, stringToSend, strlen(stringToSend));
		} else {
			break;
		}
	} while (strcmp(stringToSend, "") != 0);

	if (ret < 0) {
		perror("Failed to write the message to the device.");
		return errno;
	}
 
	printf("Press ENTER to read from the device, or e to exit...\n");
	exit = getchar();
	  
	if (exit != 'e') {
		printf("Reading from the device...\n");
		ret = read(fd, receive, BUFFER_LENGTH);

		if (ret < 0) {
			perror("Failed to read the message from the device.");
			return errno;
		}

		printf("The received message is: [%s]\n", receive);
	}
	
	printf("\nEnd of the program\n");

	return 0;
}
