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

int getPathArray(char** pathArray) {
	char* path = getenv("MYPATH");
	pathArray[0] = strtok(path, ":");
	int i = 0;
	while (pathArray[i] != NULL) {
		i++;
		pathArray[i] = strtok(NULL, ":");
	}
	int pathLength = i;
	return pathLength;
}

void deleteConnec(int temp) {
	pid_t pid = waitpid(-1, NULL, WNOHANG);
	if (pid > 0) {
		printf("[process %d completed]\n", pid);
	}
}


void executeWithPipe(char** commandArgs) {
	char* firstCommand[MAX_COMMAND_LENGTH];
	char* secondCommand[MAX_COMMAND_LENGTH];

	int i = 0;
	while (0 != strcmp(commandArgs[i], "|")) {
		firstCommand[i] = commandArgs[i];
		i++;
	}
	i++;
	int j = i;
	while (commandArgs[i] != NULL) {
		secondCommand[i-j] = commandArgs[i];
		i++;
	}
	#ifdef DEBUG
		int test = 0;
		while (firstCommand[test] != NULL) {
			printf("%s\n", firstCommand[test]);
			test++;
		}
		test = 0;
		while (secondCommand[test] != NULL) {
			printf("%s\n", secondCommand[test]);
			test++;
		}
	#endif

	char* pathArray[MAX_PATH_LENGTH];
	int pathLength = getPathArray(pathArray);
	int background = 0;

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

				int p[2];
				int rc = pipe(p);

				if ( rc < 0 ) {
				    perror( "pipe() failed" );
				    return;
				}

				int childPID2 = fork();
				if (childPID2 == 0) {
					close(p[0]);
					dup2(p[1], 1);
					execvp(firstCommand[0], firstCommand);
				}
				else {
					close(p[1]);
					dup2(p[0], 0);
					execvp(secondCommand[0], secondCommand);
				}
				
			}
			else if (background) {
				signal(SIGCHLD, &deleteConnec);
				waitpid(-1, NULL, WNOHANG);
			}
			else {
				pid_t tempPID = waitpid(childPID, NULL, 0);
				printf("Process %d terminated.\n", tempPID);
			}
			break;
		}
	}

	


}
void executeCommand(char* command) {
	#ifdef DEBUG
		printf("Inside executeCommand\n");
		printf("%s\n", command);
	#endif

	
	
	int i = 0;

	

	char* commandArgs[MAX_COMMAND_LENGTH];
	commandArgs[0] = strtok(command, " ");
	//int p = 0;
	int background = 0;
	
	i = 0;

	while (commandArgs[i] != NULL) {
		#ifdef DEBUG
			printf("%s %d\n", commandArgs[i], i);
		#endif
		i++;
		commandArgs[i] = strtok(NULL, " ");
	}
	int commandArgsLen = i;
	for (i = 0; i < commandArgsLen; i++) {
		if (0 == strcmp(commandArgs[i], "&")) {
			//printf("%s\n", commandArgs[i]);
			background = 1;
		}
		if (0 == (strcmp(commandArgs[i], "|"))) {
			executeWithPipe(commandArgs);
			return;
		}
	}

	char* pathArray[MAX_PATH_LENGTH];
	int pathLength = getPathArray(pathArray);

	#ifdef DEBUG
		for (i = 0; i < pathLength; i++) {
			printf("%s\n", pathArray[i]);
		}
	#endif

	#ifdef DEBUG
		printf("%s", command);
		printf("------\n");
	#endif

	if (0 == strcmp(commandArgs[0], "cd")) {
		chdir(commandArgs[1]);
		return;
	}
	if (0 == strcmp(commandArgs[0], "echo")) {
		if ('$' == commandArgs[1][0]) {
			char* enviroVar = getenv(++commandArgs[1]);
			printf("%s\n", enviroVar);
		}
		else {
			printf("%s\n", commandArgs[1]);
		}
		return;
	}
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
			else if (background) {
				signal(SIGCHLD, &deleteConnec);
				printf("[process running in background with pid %d]\n", childPID);
				waitpid(-1, NULL, WNOHANG);
			}
			else {
				
				pid_t tempPID = waitpid(childPID, NULL, 0);
				printf("Process %d terminated.\n", tempPID);
			}
			return;
		}
	}

	printf("ERROR: command '%s' not found\n", commandArgs[0]);
	
	
}
void findCommand(char* currentCommand, char commandHistory[][MAX_COMMAND_LENGTH], int currentCommandIndex) {
	
	#ifdef DEBUG
		printf("Inside findCommand\n");
		printf("%c\n", currentCommand[1]);
	#endif

	char tempCommand[MAX_COMMAND_LENGTH];
	if (currentCommand[1] == '!') {
		strcpy(tempCommand, commandHistory[(currentCommandIndex-1)%HISTORY_SIZE]);
		#ifdef DEBUG
			printf("%s", tempCommand);
		#endif
		strcpy(commandHistory[currentCommandIndex%HISTORY_SIZE], tempCommand);
		executeCommand(tempCommand); //printf("%s\n", referenceCommand);
	}
	else if (isdigit(currentCommand[1]) && isdigit(currentCommand[2]) && isdigit(currentCommand[3])) {
		char indexAsString[4];
		//printf("%c %c %c\n", currentCommand[1], currentCommand[2], currentCommand[3]);
		sprintf(indexAsString, "%c%c%c", currentCommand[1], currentCommand[2], currentCommand[3]);
		//printf("%s\n", indexAsString);
		int index = atoi(indexAsString);
		index--;
		strcpy(tempCommand, commandHistory[index]);
		strcpy(commandHistory[currentCommandIndex%HISTORY_SIZE], tempCommand);
		executeCommand(tempCommand);
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
		for (i = currentCommandIndex; i%HISTORY_SIZE > 0; i--) {
			#ifdef DEBUG
				printf("%s %s\n", currentCommand, commandHistory[i]);
			#endif
			if (strncmp(currentCommand, commandHistory[i], commandLen) == 0) {
				strcpy(commandHistory[currentCommandIndex%HISTORY_SIZE], commandHistory[i]);
				strcpy(tempCommand, commandHistory[i]);
				executeCommand(tempCommand);
				break;
			}
		}
		for (i = 999; i > currentCommandIndex; i--) {
			if (strncmp(currentCommand, commandHistory[i], commandLen) == 0) {
				strcpy(commandHistory[currentCommandIndex%HISTORY_SIZE], commandHistory[i]);
				strcpy(tempCommand, commandHistory[i]);
				executeCommand(tempCommand);
				break;
			}
		}
		//printf("%c\n", currentCommand[1]);
	}

}

int main(int argc, char* argv[]) {


	char commandHistory[HISTORY_SIZE][MAX_COMMAND_LENGTH];
	int currentCommandIndex = 0;
	while (1) {
		char cwd[1024];
		getcwd(cwd, 1024);
		printf("%s$ ", cwd);
		fflush(0);

		char currentCommand[MAX_COMMAND_LENGTH];
		int sizeRead = read(0, currentCommand, MAX_COMMAND_LENGTH-1);
		currentCommand[sizeRead-1] = '\0';
		//printf("%s\n", (char*) currentCommand);

		#ifdef DEBUG
			printf("%d\n", sizeRead);
			printf("%c\n", currentCommand[0]);
		#endif
		
		if (currentCommand[0] == '!') {
			
			#ifdef DEBUG
				printf("Inside if\n");
			#endif

			findCommand(currentCommand, commandHistory, currentCommandIndex);
			currentCommandIndex++;
		}
		else if (0 == strcmp(currentCommand, "exit\0") || 0 == strcmp(currentCommand, "quit\0")) {
			#ifdef DEBUG
				printf("Inside else if\n");
			#endif
			
			printf("BYE\n");
			exit(0);
			break;
		}
		else if (0 == strcmp(currentCommand, "history\0")) {
			#ifdef DEBUG
				printf("Printing history: \n");
			#endif
			strcpy(commandHistory[currentCommandIndex%HISTORY_SIZE], currentCommand);
			currentCommandIndex++;
			int i = 0;
			if (currentCommandIndex < 999) {
				
				for (; i < currentCommandIndex; i++) {
					printf("%03d %s\n", i+1, commandHistory[i]);
				}
			}
			else {
				for (i = 0; i < 999; i++) {
					printf("%03d %s\n", currentCommandIndex-HISTORY_SIZE+i, commandHistory[(currentCommandIndex+i)%HISTORY_SIZE] );
				}
			}

		}
		else {
			#ifdef DEBUG
				printf("Inside else\n");
			#endif
			strcpy(commandHistory[currentCommandIndex%HISTORY_SIZE], currentCommand);
			currentCommandIndex++;	
			executeCommand(currentCommand);		
		}
	}
	return EXIT_SUCCESS;
}