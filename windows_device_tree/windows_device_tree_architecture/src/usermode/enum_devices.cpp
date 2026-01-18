/*
 * Device Tree Enumerator - User Mode Tool
 * Uses SetupAPI to walk Windows device tree and query properties
 * 
 * Compile: cl /W4 /EHsc enum_devices.cpp /link setupapi.lib
 */

#include <windows.h>
#include <setupapi.h>
#include <devpkey.h>
#include <cfgmgr32.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "cfgmgr32.lib")

// Performance timing structure
typedef struct _PERF_STATS {
    ULONGLONG totalQueries;
    ULONGLONG totalTime;  // in microseconds
    ULONGLONG minTime;
    ULONGLONG maxTime;
} PERF_STATS;

void PrintDeviceProperty(HDEVINFO deviceInfoSet, PSP_DEVINFO_DATA deviceInfoData, 
                         const DEVPROPKEY *propertyKey, const char *propertyName,
                         PERF_STATS *stats)
{
    DEVPROPTYPE propertyType;
    BYTE buffer[4096];
    DWORD requiredSize = 0;
    LARGE_INTEGER startTime, endTime, frequency;

    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&startTime);

    BOOL result = SetupDiGetDevicePropertyW(
        deviceInfoSet,
        deviceInfoData,
        propertyKey,
        &propertyType,
        buffer,
        sizeof(buffer),
        &requiredSize,
        0
    );

    QueryPerformanceCounter(&endTime);

    ULONGLONG queryTimeMicros = ((endTime.QuadPart - startTime.QuadPart) * 1000000) / frequency.QuadPart;

    if (stats) {
        stats->totalQueries++;
        stats->totalTime += queryTimeMicros;
        if (queryTimeMicros < stats->minTime) stats->minTime = queryTimeMicros;
        if (queryTimeMicros > stats->maxTime) stats->maxTime = queryTimeMicros;
    }

    if (result) {
        wprintf(L"  %S: ", propertyName);
        
        if (propertyType == DEVPROP_TYPE_STRING) {
            wprintf(L"%s\n", (WCHAR*)buffer);
        } else if (propertyType == DEVPROP_TYPE_GUID) {
            GUID *guid = (GUID*)buffer;
            wprintf(L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}\n",
                   guid->Data1, guid->Data2, guid->Data3,
                   guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
                   guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);
        } else if (propertyType == DEVPROP_TYPE_STRINGLIST) {
            WCHAR *str = (WCHAR*)buffer;
            while (*str) {
                wprintf(L"%s ", str);
                str += wcslen(str) + 1;
            }
            wprintf(L"\n");
        } else {
            wprintf(L"(Type: 0x%X)\n", propertyType);
        }
        
        printf("    [Query time: %llu microseconds]\n", queryTimeMicros);
    } else {
        DWORD error = GetLastError();
        if (error != ERROR_NOT_FOUND) {
            printf("  %s: Error %lu\n", propertyName, error);
        }
    }
}

void EnumerateDevices(void)
{
    HDEVINFO deviceInfoSet;
    SP_DEVINFO_DATA deviceInfoData;
    DWORD deviceIndex = 0;
    PERF_STATS stats = {0};
    stats.minTime = ULONGLONG_MAX;

    printf("\n");
    printf("=================================================\n");
    printf("Windows Device Tree Enumeration\n");
    printf("=================================================\n\n");

    // Get all devices in the system
    deviceInfoSet = SetupDiGetClassDevsW(
        NULL,
        NULL,
        NULL,
        DIGCF_PRESENT | DIGCF_ALLCLASSES
    );

    if (deviceInfoSet == INVALID_HANDLE_VALUE) {
        printf("SetupDiGetClassDevs failed: %lu\n", GetLastError());
        return;
    }

    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    while (SetupDiEnumDeviceInfo(deviceInfoSet, deviceIndex, &deviceInfoData)) {
        printf("\n[Device %lu]\n", deviceIndex);
        printf("---------------------------------------------------\n");

        // Query various properties
        PrintDeviceProperty(deviceInfoSet, &deviceInfoData, &DEVPKEY_Device_DeviceDesc, 
                           "Description", &stats);
        PrintDeviceProperty(deviceInfoSet, &deviceInfoData, &DEVPKEY_Device_InstanceId, 
                           "Instance ID", &stats);
        PrintDeviceProperty(deviceInfoSet, &deviceInfoData, &DEVPKEY_Device_HardwareIds, 
                           "Hardware IDs", &stats);
        PrintDeviceProperty(deviceInfoSet, &deviceInfoData, &DEVPKEY_Device_Class, 
                           "Class", &stats);
        PrintDeviceProperty(deviceInfoSet, &deviceInfoData, &DEVPKEY_Device_ClassGuid, 
                           "Class GUID", &stats);
        PrintDeviceProperty(deviceInfoSet, &deviceInfoData, &DEVPKEY_Device_Driver, 
                           "Driver", &stats);
        PrintDeviceProperty(deviceInfoSet, &deviceInfoData, &DEVPKEY_Device_Manufacturer, 
                           "Manufacturer", &stats);
        
        deviceIndex++;

        // Limit output for demo
        if (deviceIndex >= 10) {
            printf("\n... (showing first 10 devices, %lu more available)\n", 
                   deviceIndex);
            break;
        }
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);

    // Print performance statistics
    printf("\n");
    printf("=================================================\n");
    printf("Performance Statistics\n");
    printf("=================================================\n");
    printf("Total property queries: %llu\n", stats.totalQueries);
    printf("Total time: %llu microseconds\n", stats.totalTime);
    printf("Average time per query: %llu microseconds\n", 
           stats.totalQueries > 0 ? stats.totalTime / stats.totalQueries : 0);
    printf("Minimum query time: %llu microseconds\n", stats.minTime);
    printf("Maximum query time: %llu microseconds\n", stats.maxTime);
    printf("=================================================\n\n");
}

int main(void)
{
    printf("\n");
    printf("  ____             _            _____                \n");
    printf(" |  _ \\  _____   _(_) ___ ___  |_   _| __ ___  ___  \n");
    printf(" | | | |/ _ \\ \\ / / |/ __/ _ \\   | || '__/ _ \\/ _ \\ \n");
    printf(" | |_| |  __/\\ V /| | (_|  __/   | || | |  __/  __/ \n");
    printf(" |____/ \\___| \\_/ |_|\\___\\___|   |_||_|  \\___|\\___| \n");
    printf("                                                      \n");
    printf("  _____                                        _             \n");
    printf(" | ____|_ __  _   _ _ __ ___   ___ _ __ __ _| |_ ___  _ __ \n");
    printf(" |  _| | '_ \\| | | | '_ ` _ \\ / _ \\ '__/ _` | __/ _ \\| '__|\n");
    printf(" | |___| | | | |_| | | | | | |  __/ | | (_| | || (_) | |   \n");
    printf(" |_____|_| |_|\\__,_|_| |_| |_|\\___|_|  \\__,_|\\__\\___/|_|   \n\n");

    EnumerateDevices();

    printf("\nPress Enter to exit...\n");
    getchar();

    return 0;
}
