#include <windows.h>
#include <stdio.h>

#define DEVICE_PATH "\\\\.\\EchoDevice"

int main() {
    HANDLE hDevice;
    char writeBuffer[] = "Hello from user space!";
    char readBuffer[256] = {0};
    DWORD bytesWritten = 0;
    DWORD bytesRead = 0;
    
    printf("=== Echo Driver Test Application ===\n\n");
    
    printf("[1] Opening device: %s\n", DEVICE_PATH);
    hDevice = CreateFile(DEVICE_PATH, GENERIC_READ | GENERIC_WRITE, 0, NULL, 
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    
    if (hDevice == INVALID_HANDLE_VALUE) {
        printf("    ERROR: Failed to open device (Error: %d)\n", GetLastError());
        printf("    Make sure the driver is loaded!\n");
        return 1;
    }
    printf("    SUCCESS: Device opened (Handle: 0x%p)\n\n", hDevice);
    
    printf("[2] Writing data to driver: \"%s\"\n", writeBuffer);
    BOOL writeResult = WriteFile(hDevice, writeBuffer, (DWORD)strlen(writeBuffer), 
                                 &bytesWritten, NULL);
    
    if (!writeResult) {
        printf("    ERROR: WriteFile failed (Error: %d)\n", GetLastError());
        CloseHandle(hDevice);
        return 1;
    }
    printf("    SUCCESS: Wrote %d bytes\n\n", bytesWritten);
    
    printf("[3] Reading data back from driver\n");
    BOOL readResult = ReadFile(hDevice, readBuffer, sizeof(readBuffer) - 1, 
                               &bytesRead, NULL);
    
    if (!readResult) {
        printf("    ERROR: ReadFile failed (Error: %d)\n", GetLastError());
        CloseHandle(hDevice);
        return 1;
    }
    printf("    SUCCESS: Read %d bytes\n", bytesRead);
    printf("    Data: \"%s\"\n\n", readBuffer);
    
    if (strcmp(writeBuffer, readBuffer) == 0) {
        printf("[PASS] Echo verified! Data matches.\n");
    } else {
        printf("[FAIL] Data mismatch!\n");
    }
    
    CloseHandle(hDevice);
    printf("\nTest completed.\n");
    
    return 0;
}
