#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <vector>
#include <sstream>

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
    for (int i = start; i < end; ++i) {
        if (isPrime(i)) {
            resultStream << i << " ";
        }
    }
    std::string result = resultStream.str();
    write(writePipe, result.c_str(), result.size());
    close(writePipe);
    _exit(0); // Asigurăm terminarea procesului copil
}

void createProcesses() {
    int pipes[NUM_PROCESSES][2];
    pid_t pids[NUM_PROCESSES];

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

        if (pids[i] == 0) { // Proces copil
            close(pipes[i][0]); // Închidem capătul de citire
            int start = i * RANGE;
            int end = (i + 1) * RANGE;
            findPrimesInRange(start, end, pipes[i][1]);
        }
        else { // Proces părinte
            close(pipes[i][1]); // Închidem capătul de scriere
        }
    }

    // Proces părinte colectează rezultatele
    for (int i = 0; i < NUM_PROCESSES; ++i) {
        waitpid(pids[i], nullptr, 0); // Așteaptă procesul copil să termine
        char buffer[1024];
        ssize_t bytesRead;
        std::cout << "Primes from process " << i + 1 << ": ";
        while ((bytesRead = read(pipes[i][0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytesRead] = '\0';
            std::cout << buffer;
        }
        std::cout << "\n";
        close(pipes[i][0]);
    }
}

int main() {
    createProcesses();
    return 0;
}
