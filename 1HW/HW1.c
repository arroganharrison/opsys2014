#include <stdio.h>
#include <unistd.h>

void findCommand(char* currentCommand, char* commandHistory[], int currentCommandIndex) {
	printf("Inside findCommand");
	char* referenceCommand;
	if (currentCommand[1] == '!') {
		printf("Hello");
		referenceCommand = commandHistory[currentCommandIndex%1000];
		write(1, referenceCommand, 1000);
	}
	else {
		printf(currentCommand[1]);
	}

}
int main(int argc, char* argv[]) {
	write(1, "Inside main\0", 30);
	char* commandHistory[1000];
	int currentCommandIndex = 0;
	while (1) {
		char currentCommand[1000];
		int sizeRead = read(0, currentCommand, 999);
		write(1, currentCommand[0], 1);
		if (currentCommand[0] == '!') {
			findCommand(currentCommand, commandHistory, currentCommandIndex);
		}
		else {
			commandHistory[currentCommandIndex%1000] = currentCommand;
			currentCommandIndex++;
			currentCommand[sizeRead] = '\n';
			int sizeWrote = write(1, currentCommand, sizeRead);
		}
	}
}