#include <stdio.h>
#include <syslog.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {

	if(argc != 3) {
		printf("usage: writer writefile writestr\n");
		return 1;
	}

	const char* writefile = argv[1];
	const char* writestr = argv[2];

	openlog(NULL, 0, LOG_USER);

	FILE* file = fopen(writefile, "wb");

	if (file == NULL) {
		printf("file %s cannot be opened\n", writefile);
		perror("perror returned");
		syslog(LOG_ERR, "file %s cannot be opened\n", writefile);
		exit(EXIT_FAILURE);
	} else {
		syslog(LOG_DEBUG, "Writing %s to %s\n", writefile, writestr);
		fprintf(file, "%s\n", writestr);
		fclose(file);
	}

	exit(EXIT_SUCCESS);
}
