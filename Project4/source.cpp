#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>
#include <sstream>
#include <cstring>

#define NUM_PROCESSES 10
#define RANGE 1000

// Function to check if a number is prime
bool isPrime(int num) {
    if (num < 2) return false;
    for (int i = 2; i * i <= num; ++i) {
        if (num % i == 0) return false;
    }
    return true;
}

// Child process function to find primes in a range and write them to a pipe
void findPrimesInRange(int start, int end, int writePipe) {
    std::ostringstream resultStream;
    for (int i = start; i < end; ++i) {
        if (isPrime(i)) {
            resultStream << i << " ";
        }
    }
    std::string result = resultStream.str();
    write(writePipe, result.c_str(), result.size());
    close(writePipe); // Ensure the pipe is closed after writing
    _exit(0); // Terminate the child process
}

// Process 1 collects results from all other processes and sends to the parent
void processOne(int pipes[NUM_PROCESSES][2], int writeToParent) {
    std::ostringstream aggregatedResults;

    // Process 1 collects results from other child processes
    for (int i = 1; i < NUM_PROCESSES; ++i) {
        close(pipes[i][1]); // Close the write end of the pipe
        char buffer[1024];
        ssize_t bytesRead;
        while ((bytesRead = read(pipes[i][0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytesRead] = '\0'; // Null-terminate the string
            aggregatedResults << buffer;
        }
        close(pipes[i][0]); // Close the read end after reading
    }

    // Send the aggregated results to the parent
    std::string finalOutput = aggregatedResults.str();
    write(writeToParent, finalOutput.c_str(), finalOutput.size());
    close(writeToParent); // Close the pipe to parent
    _exit(0); // Terminate Process 1
}

// Function to create child processes and manage pipes
void createProcesses() {
    int pipes[NUM_PROCESSES][2]; // Pipes for communication between parent and children
    int pipeToParent[2];         // Pipe for Process 1 to send results to the parent
    pid_t pids[NUM_PROCESSES];

    if (pipe(pipeToParent) == -1) {
        std::cerr << "Error creating pipe to parent.\n";
        exit(1);
    }

    for (int i = 0; i < NUM_PROCESSES; ++i) {
        if (pipe(pipes[i]) == -1) {
            std::cerr << "Error creating pipe " << i << "\n";
            exit(1);
        }

        pids[i] = fork();
        if (pids[i] == -1) {
            std::cerr << "Error creating process " << i << "\n";
            exit(1);
        }

        if (pids[i] == 0) { // Child process
            if (i == 0) { // Process 1
                close(pipeToParent[0]); // Close read end of parent pipe
                processOne(pipes, pipeToParent[1]);
            }
            else { // Other processes
                close(pipes[i][0]); // Close read end of child pipe
                int start = i * RANGE;
                int end = (i + 1) * RANGE;
                findPrimesInRange(start, end, pipes[i][1]);
            }
        }
    }

    // Parent process
    close(pipeToParent[1]); // Close write end of parent pipe
    for (int i = 0; i < NUM_PROCESSES; ++i) {
        if (i != 0) close(pipes[i][0]); // Close read ends for all other pipes
        if (i != 0) close(pipes[i][1]); // Close write ends for all other pipes
    }

    // Read the final results from Process 1
    char buffer[1024];
    ssize_t bytesRead;
    std::cout << "Final primes collected by Process 1:\n";
    while ((bytesRead = read(pipeToParent[0], buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytesRead] = '\0'; // Null-terminate the string
        std::cout << buffer; // Print the output as it is received
    }
    close(pipeToParent[0]); // Close the read end of the pipe

    // Wait for all child processes to finish
    for (int i = 0; i < NUM_PROCESSES; ++i) {
        waitpid(pids[i], nullptr, 0);
    }
}

int main() {
    createProcesses();
    return 0;
}
