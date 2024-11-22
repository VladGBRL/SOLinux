#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cstring>
#include <string>

#define NUM_PROCESSES 10

void CreatePipesAndProcesses() {
    int hPipes[NUM_PROCESSES][2];
    pid_t pid[NUM_PROCESSES];
    char commandLine[256];
    std::string finalOutput = "";

    for (int i = 0; i < NUM_PROCESSES; i++) {
       
        if (pipe(hPipes[i]) == -1) {
            std::cerr << "Error creating pipe " << i + 1 << " (Error code: " << strerror(errno) << ").\n";
            return;
        }

        pid[i] = fork();
        if (pid[i] == -1) {
            std::cerr << "Error creating process " << i + 1 << " (Error code: " << strerror(errno) << ").\n";
            return;
        }

        if (pid[i] == 0) {  // Child process
            // Close the write end of the pipe in the child process
            close(hPipes[i][0]);

            // Prepare the command line for prime_checker
            snprintf(commandLine, sizeof(commandLine), "prime_checker %d %d", i * 1000, (i + 1) * 1000);

            // Execute the command
            execlp(commandLine, commandLine, NULL);
            // If execlp fails
            std::cerr << "Error executing command " << commandLine << " (Error code: " << strerror(errno) << ").\n";
            return;
        }
        else {  // Parent process
            // Close the read end of the pipe in the parent process
            close(hPipes[i][1]);

            // Read output from the pipe
            char buffer[1024];
            ssize_t bytesRead;
            std::cout << "Reading from pipe " << i + 1 << "...\n";
            while ((bytesRead = read(hPipes[i][0], buffer, sizeof(buffer) - 1)) > 0) {
                buffer[bytesRead] = '\0';  // Null-terminate the string
                finalOutput += buffer;
                std::cout << "Output from process " << i + 1 << ": " << buffer << "\n";
            }

            if (bytesRead == -1) {
                std::cerr << "Error reading from pipe " << i + 1 << " (Error code: " << strerror(errno) << ").\n";
                return;
            }

            // Close the read end in the parent
            close(hPipes[i][0]);
        }
    }

    for (int i = 0; i < NUM_PROCESSES; i++) {
        waitpid(pid[i], NULL, 0);
    }

    std::cout << "\nFinal output after all processes completed:\n" << finalOutput << "\n";
}

int main() {
    CreatePipesAndProcesses();
    return 0;
}
