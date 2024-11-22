#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <sstream>
#include <vector>
#include <cstring>

#define NUM_PROCESSES 10
#define RANGE 1000

// Funcție pentru verificarea numerelor prime
bool isPrime(int num) {
    if (num < 2) return false;
    for (int i = 2; i * i <= num; ++i) {
        if (num % i == 0) return false;
    }
    return true;
}

// Proces copil: calculează numerele prime și le scrie într-un pipe
void findPrimesInRange(int start, int end, int writePipe) {
    std::ostringstream resultStream;
    resultStream << "[Process " << getpid() << "] Primes in range [" << start << ", " << end << "]: ";
    for (int i = start; i <= end; ++i) { // Modificat pentru a include capătul final
        if (isPrime(i)) {
            resultStream << i << " ";
        }
    }
    std::string result = resultStream.str();
    std::cout << result << "\n"; // Afișează ce a calculat procesul
    write(writePipe, result.c_str(), result.size());
    close(writePipe); // Închide pipe-ul după scriere
    _exit(0);
}

// Procesul care colectează rezultatele și calculează și numere prime
void processOne(int pipes[NUM_PROCESSES][2], int writeToParent) {
    std::ostringstream aggregatedResults;
    std::cout << "[Process 1] Collecting results and calculating primes...\n";

    // Procesul 1 va calcula și numere prime pe intervalul său
    int start = 0;
    int end = RANGE - 1; // Primul interval [0, 999]
    findPrimesInRange(start, end, pipes[0][1]);

    // Colectează rezultatele de la celelalte procese
    for (int i = 1; i < NUM_PROCESSES; ++i) {
        close(pipes[i][1]); // Închide capătul de scriere al pipe-ului
        char buffer[1024];
        ssize_t bytesRead;
        while ((bytesRead = read(pipes[i][0], buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytesRead] = '\0'; // Null-terminate buffer-ul
            aggregatedResults << buffer;
        }
        close(pipes[i][0]); // Închide capătul de citire după ce ai terminat
    }

    std::string finalOutput = aggregatedResults.str();
    std::cout << "[Process 1] Final primes collected:\n" << finalOutput << "\n";

    write(writeToParent, finalOutput.c_str(), finalOutput.size());
    close(writeToParent); // Închide pipe-ul către părinte
    _exit(0);
}

// Crează procesele și pipe-urile
void createProcesses() {
    int pipes[NUM_PROCESSES][2]; // Pipe-uri pentru fiecare proces
    int pipeToParent[2];         // Pipe pentru procesul 1 -> părinte
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

        if (pids[i] == 0) { // Proces copil
            int start = i * RANGE;
            int end = (i == NUM_PROCESSES - 1) ? 10000 : (i + 1) * RANGE - 1; // Ajustare pentru ultimul interval
            if (i == 0) {
                processOne(pipes, pipeToParent[1]); // Procesul 1 calculează și primele sale
            }
            else {
                close(pipes[i][0]); // Închide capătul de citire
                findPrimesInRange(start, end, pipes[i][1]);
            }
        }
    }

    // Procesul părinte
    close(pipeToParent[1]); // Închide capătul de scriere
    for (int i = 1; i < NUM_PROCESSES; ++i) {
        close(pipes[i][0]); // Închide capetele de citire
        close(pipes[i][1]); // Închide capetele de scriere
    }

    // Citește rezultatele finale de la procesul 1
    char buffer[1024];
    ssize_t bytesRead;
    std::cout << "Final primes collected by Process 1:\n";
    while ((bytesRead = read(pipeToParent[0], buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytesRead] = '\0';
        std::cout << buffer; // Afișează rezultatele
    }
    close(pipeToParent[0]); // Închide capătul de citire

    // Așteaptă procesele să termine
    for (int i = 0; i < NUM_PROCESSES; ++i) {
        waitpid(pids[i], nullptr, 0);
    }
}

int main() {
    createProcesses();
    return 0;
}
