#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#define MAX_COMMAND_LENGTH 1000
#define HISTORY_SIZE 1000
#define MAX_PATH_LENGTH 1000



void executeCommand(char* command) {
	#ifdef DEBUG
		printf("Inside executeCommand\n");
		printf("%s\n", command);
	#endif
		
	char* path = getenv("MYPATH");
	char* pathArray[MAX_PATH_LENGTH];
	pathArray[0] = strtok(path, ":");
	int i = 0;
	while (pathArray[i] != NULL) {
		i++;
		pathArray[i] = strtok(NULL, ":");
	}
	int pathLength = i;

	#ifdef DEBUG
		for (i = 0; i < pathLength; i++) {
			printf("%s\n", pathArray[i]);
		}
	#endif

	char* commandArgs[MAX_COMMAND_LENGTH];
	commandArgs[0] = strtok(command, " ");
	i = 0;
	while (commandArgs[i] != NULL) {
		#ifdef DEBUG
			printf("%s\n", commandArgs[i]);
		#endif
		i++;
		commandArgs[i] = strtok(NULL, " ");
		// if (strcmp(commandArgs[i], "|") == 0) {
		// 	//do other things
		// }
	}

	#ifdef DEBUG
		printf("------\n");
	#endif

	for (i = 0; i < pathLength; i++) {
		char commandDir[100];
		struct stat lstatBuff;
		sprintf(commandDir, "%s/%s", pathArray[i], commandArgs[0]);
		if (lstat(pathArray[i], &lstatBuff) == 0) {

			#ifdef DEBUG
				printf("%s\n", pathArray[i]);
			#endif

			int childPID = fork();

			if (childPID == 0) {
				execvp(commandArgs[0], commandArgs);
			}
			else {
				pid_t tempPID = wait(NULL);
				printf("Process %d terminated.", tempPID);
			}
			break;
		}
	}
	
}
void findCommand(char* currentCommand, char commandHistory[][MAX_COMMAND_LENGTH], int currentCommandIndex) {
	
	#ifdef DEBUG
		printf("Inside findCommand\n");
		printf("%c\n", currentCommand[1]);
	#endif

	char* referenceCommand;
	if (currentCommand[1] == '!') {
		referenceCommand = commandHistory[(currentCommandIndex-1)%HISTORY_SIZE];
		#ifdef DEBUG
			printf("%s", referenceCommand);
		#endif
		executeCommand(referenceCommand); //printf("%s\n", referenceCommand);
	}
	else if (isdigit(currentCommand[1]) && isdigit(currentCommand[2]) && isdigit(currentCommand[3])) {
		char indexAsString[4];
		//printf("%c %c %c\n", currentCommand[1], currentCommand[2], currentCommand[3]);
		sprintf(indexAsString, "%c%c%c", currentCommand[1], currentCommand[2], currentCommand[3]);
		//printf("%s\n", indexAsString);
		int index = atoi(indexAsString);
		executeCommand(commandHistory[index]);
		//the arithmetic converts the individual 3 digits into a usable integer
	} 
	else {
		int commandLen = 1;
		while (currentCommand[commandLen] != '\0') {
			commandLen++;
		}
		commandLen--;
		int i;
		currentCommand++;
		for (i = currentCommandIndex-1; i%HISTORY_SIZE > 0; i--) {
			#ifdef DEBUG
				printf("%s %s\n", currentCommand, commandHistory[i]);
			#endif
			if (strncmp(currentCommand, commandHistory[i], commandLen) == 0) {
				executeCommand(commandHistory[i]);
				break;
			}
		}
		for (i = 999; i >= currentCommandIndex; i--) {
			if (strncmp(currentCommand, commandHistory[i], commandLen) == 0) {
				executeCommand(commandHistory[i]);
				break;
			}
		}
		printf("%c\n", currentCommand[1]);
	}

}

int main(int argc, char* argv[]) {

	printf("Inside main; printf\n");
	char commandHistory[HISTORY_SIZE][MAX_COMMAND_LENGTH];
	int currentCommandIndex = 1;
	while (1) {
		char currentCommand[MAX_COMMAND_LENGTH];
		int sizeRead = read(0, currentCommand, MAX_COMMAND_LENGTH-1);
		currentCommand[sizeRead-1] = '\0';
		printf("%s\n", (char*) currentCommand);

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
			executeCommand(currentCommand);
			currentCommandIndex++;			
		}
	}
	return EXIT_SUCCESS;
}