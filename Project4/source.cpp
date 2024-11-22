#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <sstream>
#include <vector>
#include <cstring>

#define NUM_PROCESSES 10
#define RANGE 1000

bool isPrime(int num) {
    if (num < 2) return false;
    for (int i = 2; i * i <= num; ++i) {
        if (num % i == 0) return false;
    }
    return true;
}

void findPrimesInRange(int start, int end, int writePipe) {
    std::ostringstream resultStream;
    resultStream << "[Process " << getpid() << "] Primes in range [" << start << ", " << end << "]: ";
    for (int i = start; i <= end; ++i) {
        if (isPrime(i)) {
            resultStream << i << " ";
        }
    }
    std::string result = resultStream.str();
    std::cout << result << "\n"; 
    write(writePipe, result.c_str(), result.size());
    close(writePipe); 
    _exit(0);
}

void processOne(int pipes[NUM_PROCESSES][2], int writeToParent) {
    std::ostringstream aggregatedResults;
    std::cout << "[Process 1] Collecting results and calculating primes...\n";

    int start = 0;
    int end = RANGE - 1; 
    findPrimesInRange(start, end, pipes[0][1]);

    for (int i = 1; i < NUM_PROCESSES; ++i) {
        close(pipes[i][1]); 
        char buffer[1024];
        ssize_t bytesRead;
        while ((bytesRead = read(pipes[i][0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytesRead] = '\0'; 
            aggregatedResults << buffer;
        }
        close(pipes[i][0]); 
    }

    std::string finalOutput = aggregatedResults.str();
    std::cout << "[Process 1] Final primes collected:\n" << finalOutput << "\n";

    write(writeToParent, finalOutput.c_str(), finalOutput.size());
    close(writeToParent); 
    _exit(0);
}

void createProcesses() {
    int pipes[NUM_PROCESSES][2]; 
    int pipeToParent[2];         
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

        if (pids[i] == 0) { 
            int start = i * RANGE;
            int end = (i == NUM_PROCESSES - 1) ? 10000 : (i + 1) * RANGE - 1; 
            if (i == 0) {
                processOne(pipes, pipeToParent[1]); 
            }
            else {
                close(pipes[i][0]); 
                findPrimesInRange(start, end, pipes[i][1]);
            }
        }
    }

    close(pipeToParent[1]);
    for (int i = 1; i < NUM_PROCESSES; ++i) {
        close(pipes[i][0]); 
        close(pipes[i][1]); 
    }

    char buffer[1024];
    ssize_t bytesRead;
    std::cout << "Final primes collected by Process 1:\n";
    while ((bytesRead = read(pipeToParent[0], buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytesRead] = '\0';
        std::cout << buffer; 
    }
    close(pipeToParent[0]); 

    for (int i = 0; i < NUM_PROCESSES; ++i) {
        waitpid(pids[i], nullptr, 0);
    }
}

int main() {
    createProcesses();
    return 0;
}
