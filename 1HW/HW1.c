#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define MAX_COMMAND_LENGTH 1000
#define HISTORY_SIZE 1000

void findCommand(char* currentCommand, char commandHistory[][MAX_COMMAND_LENGTH], int currentCommandIndex) {
	
	#ifdef DEBUG
		printf("Inside findCommand\n");
		printf("%c\n", currentCommand[1]);
	#endif

	char* referenceCommand;
	if (currentCommand[1] == '!') {
		referenceCommand = commandHistory[(currentCommandIndex-1)%HISTORY_SIZE];
		printf("> %s\n", referenceCommand);
	}
	else {
		printf("> %c\n", currentCommand[1]);
	}

}

int main(int argc, char* argv[]) {

	printf("Inside main; printf\n");
	char commandHistory[HISTORY_SIZE][MAX_COMMAND_LENGTH];
	int currentCommandIndex = 0;
	while (1) {
		char currentCommand[MAX_COMMAND_LENGTH];
		int sizeRead = read(0, currentCommand, MAX_COMMAND_LENGTH-1);
		currentCommand[sizeRead-1] = '\0';
		printf("> %s\n", (char*) currentCommand);

		#ifdef DEBUG
			printf("%d\n", sizeRead);
			printf("%c\n", currentCommand[0]);
		#endif
		
		if (currentCommand[0] == '!') {
			
			#ifdef DEBUG
				printf("Inside if\n");
			#endif

			findCommand(currentCommand, commandHistory, currentCommandIndex);
		}
		else if (0 == strcmp(currentCommand, "exit\0") || 0 == strcmp(currentCommand, "quit\0")) {
			#ifdef DEBUG
				printf("Inside else if\n");
			#endif
			
			printf("BYE\n");
			break;
		}
		else {
			#ifdef DEBUG
				printf("Inside else\n");
			#endif
			strcpy(commandHistory[currentCommandIndex%HISTORY_SIZE], currentCommand);
			currentCommandIndex++;			
		}
	}
	return EXIT_SUCCESS;
}