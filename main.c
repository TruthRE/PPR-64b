#include <windows.h>
#include <tlhelp32.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include <stdlib.h>

volatile int keepRunning = 1;  // Flag to control the loop

// Function to handle Ctrl-C (SIGINT) and perform a graceful exit
void handle_signal(int signal) {
    if (signal == SIGINT) {
        printf("\nCtrl-C pressed. Exiting...\n");
        keepRunning = 0;  // Set flag to stop the loop
    }
}

// Function to clear the console screen
void ClearScreen() {
    system("cls");
}

// Function to get the PID of a process by its name
DWORD GetProcessIdByName(const char *processName) {
    PROCESSENTRY32 processEntry;
    processEntry.dwSize = sizeof(PROCESSENTRY32);

    // Create a snapshot of all running processes
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        printf("Error: Failed to create snapshot of processes. Exiting.\n");
        return 0;
    }

    // Start the process loop
    if (Process32First(hSnapshot, &processEntry)) {
        do {
            // Compare process name
            if (strcmp(processEntry.szExeFile, processName) == 0) {
                DWORD pid = processEntry.th32ProcessID;
                CloseHandle(hSnapshot);
                return pid; // Return the PID when found
            }
        } while (Process32Next(hSnapshot, &processEntry));
    }

    // Clean up and return if process is not found
    CloseHandle(hSnapshot);
    printf("Error: Process %s not found.\n", processName);
    return 0;
}

// Function to check if a string is a valid number (used for PID input)
int isNumber(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isdigit(str[i])) {
            return 0;
        }
    }
    return 1;
}

// Function to check if the process is 64-bit
BOOL IsProcess64Bit(HANDLE hProcess) {
    BOOL isWow64 = FALSE;
    if (IsWow64Process(hProcess, &isWow64)) {
        return !isWow64;  // If not WOW64, it's a 64-bit process
    }
    return FALSE;  // Unable to determine (assume 32-bit)
}

// Function to retrieve the main thread of a process
DWORD GetMainThreadId(DWORD pid) {
    THREADENTRY32 te32;
    te32.dwSize = sizeof(THREADENTRY32);

    HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hThreadSnap == INVALID_HANDLE_VALUE) {
        printf("Error: Failed to create thread snapshot. Exiting.\n");
        return 0;
    }

    DWORD mainThreadId = 0;

    if (Thread32First(hThreadSnap, &te32)) {
        do {
            if (te32.th32OwnerProcessID == pid) {
                mainThreadId = te32.th32ThreadID;
                break;
            }
        } while (Thread32Next(hThreadSnap, &te32));
    }

    CloseHandle(hThreadSnap);
    return mainThreadId;
}

// Function to get the current timestamp
void PrintTimestamp() {
    time_t now;
    time(&now);

    char timeStr[26];  // Buffer to store the time string (ctime_s requires at least 26 bytes)
    errno_t err = ctime_s(timeStr, sizeof(timeStr), &now);  // Use ctime_s for safer time handling

    if (err == 0) {
        printf("Last updated: %s", timeStr);  // Print the current time
    } else {
        printf("Failed to get the current time.\n");
    }
}

// Structure to track previous register values and counts
typedef struct {
    DWORD64 value;
    double unchangedTime;  // Time in seconds the value has not changed
} RegisterTrack;

// Function to display 64-bit registers with formatting and change tracking
void Print64bitRegisters(CONTEXT *context, RegisterTrack *previousRegisters, double sleepSeconds) {
    printf("+-------------------------------------------------+\n");
    printf("|                  Register State                 |\n");
    printf("+----------------------+--------------------------+------------------------+\n");

    const char* registerNames[] = {
        "RAX", "RBX", "RCX", "RDX", "RSI", "RDI", "RBP", "RSP", "RIP", "R8", "R9", "R10", "R11", "R12", "R13", "R14", "R15"
    };

    DWORD64 currentValues[] = {
        context->Rax, context->Rbx, context->Rcx, context->Rdx, context->Rsi, context->Rdi, context->Rbp, context->Rsp, context->Rip,
        context->R8, context->R9, context->R10, context->R11, context->R12, context->R13, context->R14, context->R15
    };

    for (int i = 0; i < 16; i++) {
        if (currentValues[i] == previousRegisters[i].value) {
            previousRegisters[i].unchangedTime += sleepSeconds;
            printf("| %-20s | 0x%016llx       | Unchanged for %.0f sec    |\n", registerNames[i], currentValues[i], previousRegisters[i].unchangedTime);
        } else {
            previousRegisters[i].value = currentValues[i];
            previousRegisters[i].unchangedTime = 0;
            printf("| %-20s | 0x%016llx       | CHANGED                |\n", registerNames[i], currentValues[i]);
        }
    }

    printf("+----------------------+--------------------------+------------------------+\n");
    printf("\nPress Ctrl+C to safely stop the program.\n");
}

// Function to prompt the user for a "Y/N" response
int PromptUser(const char *message) {
    int response;  // Use int to handle the return value of getchar
    printf("%s (Y/N): ", message);
    
    while (1) {
        response = getchar();  // Store getchar return value in an int

        // Ignore newline and other control characters
        if (response == '\n' || response == '\r') {
            continue;
        }

        if (response == 'Y' || response == 'y') {
            return 1;  // Return 1 for "Yes"
        } else if (response == 'N' || response == 'n') {
            return 0;  // Return 0 for "No"
        } else {
            printf("Invalid response. Please enter Y or N: ");
        }

        // Clear the buffer after receiving input
        while (getchar() != '\n') {}  // Discard any extra input (e.g., multiple characters)
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <process name or PID> <sleep seconds>\n", argv[0]);
        return 1;
    }

    // Handle Ctrl-C signal
    signal(SIGINT, handle_signal);

    DWORD pid = 0;

    // Check if the first argument is a number (PID) or a process name
    if (isNumber(argv[1])) {
        pid = (DWORD)atoi(argv[1]);
    } else {
        pid = GetProcessIdByName(argv[1]);
        if (pid == 0) {
            return 1;  // Process not found
        }
    }

    // Get the sleep time (as a decimal value) from the command-line arguments
    double sleepSeconds = atof(argv[2]);
    if (sleepSeconds < 0) {
        printf("Invalid sleep time. Please provide a positive number for seconds.\n");
        return 1;
    }

    // If sleep is 0, prompt the user for confirmation
    if (sleepSeconds == 0) {
        printf("Warning: Update interval set to 0. The program will refresh continuously and may be resource intensive.\n");
        if (!PromptUser("Do you understand the performance impact?")) {
            printf("Exiting program.\n");
            return 1;
        }
    }

    // Open the process
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (hProcess == NULL) {
        printf("Error: Failed to open process with PID: %lu. Error: %lu\n", pid, GetLastError());
        return 1;
    }

    // Check if the process is 64-bit, and prompt the user if it's not
    if (!IsProcess64Bit(hProcess)) {
        printf("Warning: The process with PID %lu is not 64-bit. The register values and labels may be inaccurate.\n", pid);
        if (!PromptUser("Do you want to proceed anyway?")) {
            printf("Exiting program.\n");
            CloseHandle(hProcess);
            return 1;
        }
    }

    // Get the main thread ID
    DWORD threadId = GetMainThreadId(pid);
    if (threadId == 0) {
        printf("Error: Failed to find main thread of process.\n");
        CloseHandle(hProcess);
        return 1;
    }

    // Open the main thread
    HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, threadId);
    if (hThread == NULL) {
        printf("Error: Failed to open thread. Error: %lu\n", GetLastError());
        CloseHandle(hProcess);
        return 1;
    }

    CONTEXT context;
    context.ContextFlags = CONTEXT_FULL;

    // Initialize tracking for previous register values
    RegisterTrack previousRegisters[16] = {0};

    // Continuous loop to check the registers until Ctrl-C
    while (keepRunning) {
        ClearScreen();

        // Display process info and update interval on their own lines
        printf("Process: %s (%lu)\n", argv[1], pid);
        printf("Update interval: %.2f seconds\n", sleepSeconds);

        // Suspend the thread to get a stable context
        SuspendThread(hThread);

        // Get the thread context (registers)
        if (!GetThreadContext(hThread, &context)) {
            printf("Error: Failed to get thread context. Error: %lu\n", GetLastError());
            ResumeThread(hThread);
            break;
        }

        // Print timestamp and add two blank lines before register info
        PrintTimestamp();
        printf("\n\n");

        // Print the registers with change tracking
        Print64bitRegisters(&context, previousRegisters, sleepSeconds);

        // Resume the thread after getting the context
        ResumeThread(hThread);

        // Convert seconds to milliseconds and cast to DWORD
        DWORD sleepMillis = (DWORD)(sleepSeconds * 1000);

        // Sleep for the specified duration
        Sleep(sleepMillis);
    }

    // Close the handles
    CloseHandle(hThread);
    CloseHandle(hProcess);

    return 0;
}
