/*
 * Device Tree Enumerator - KMDF Driver
 * Demonstrates proper device property queries in Windows kernel mode
 * 
 * Build with: WDK + Visual Studio
 * Target: Windows 10/11 x64
 */

#include <ntddk.h>
#include <wdf.h>
#include <devpkey.h>

#define DEVICE_ENUM_POOL_TAG 'enuD'

// Device context structure
typedef struct _DEVICE_CONTEXT {
    WDFDEVICE Device;
    ULONG PropertyQueryCount;
    LARGE_INTEGER TotalQueryTime;
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, GetDeviceContext)

// Function prototypes
DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD DeviceEnumEvtDeviceAdd;
EVT_WDF_DEVICE_D0_ENTRY DeviceEnumEvtDeviceD0Entry;

NTSTATUS QueryDeviceProperty(
    _In_ WDFDEVICE Device,
    _In_ const DEVPROPKEY *PropertyKey,
    _Out_ PVOID *PropertyData,
    _Out_ PULONG PropertyDataSize
);

NTSTATUS EnumerateDeviceTree(
    _In_ WDFDEVICE Device
);

/*
 * DriverEntry - Required entry point
 */
_Use_decl_annotations_
NTSTATUS DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
)
{
    WDF_DRIVER_CONFIG config;
    NTSTATUS status;

    KdPrint(("DeviceEnum: DriverEntry\n"));

    // Initialize driver configuration
    WDF_DRIVER_CONFIG_INIT(&config, DeviceEnumEvtDeviceAdd);

    status = WdfDriverCreate(
        DriverObject,
        RegistryPath,
        WDF_NO_OBJECT_ATTRIBUTES,
        &config,
        WDF_NO_HANDLE
    );

    if (!NT_SUCCESS(status)) {
        KdPrint(("DeviceEnum: WdfDriverCreate failed: 0x%X\n", status));
        return status;
    }

    KdPrint(("DeviceEnum: Driver initialized successfully\n"));
    return STATUS_SUCCESS;
}

/*
 * DeviceAdd - Called when PnP manager detects new device
 */
_Use_decl_annotations_
NTSTATUS DeviceEnumEvtDeviceAdd(
    WDFDRIVER Driver,
    PWDFDEVICE_INIT DeviceInit
)
{
    NTSTATUS status;
    WDFDEVICE device;
    PDEVICE_CONTEXT deviceContext;
    WDF_OBJECT_ATTRIBUTES deviceAttributes;
    WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;

    UNREFERENCED_PARAMETER(Driver);

    KdPrint(("DeviceEnum: DeviceAdd called\n"));

    // Set PnP callbacks
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
    pnpPowerCallbacks.EvtDeviceD0Entry = DeviceEnumEvtDeviceD0Entry;
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

    // Set device context
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, DEVICE_CONTEXT);

    status = WdfDeviceCreate(&DeviceInit, &deviceAttributes, &device);
    if (!NT_SUCCESS(status)) {
        KdPrint(("DeviceEnum: WdfDeviceCreate failed: 0x%X\n", status));
        return status;
    }

    // Initialize device context
    deviceContext = GetDeviceContext(device);
    deviceContext->Device = device;
    deviceContext->PropertyQueryCount = 0;
    deviceContext->TotalQueryTime.QuadPart = 0;

    KdPrint(("DeviceEnum: Device created successfully\n"));
    return STATUS_SUCCESS;
}

/*
 * D0Entry - Device entering D0 (fully powered) state
 * This is where we enumerate the device tree
 */
_Use_decl_annotations_
NTSTATUS DeviceEnumEvtDeviceD0Entry(
    WDFDEVICE Device,
    WDF_POWER_DEVICE_STATE PreviousState
)
{
    PDEVICE_CONTEXT deviceContext;
    NTSTATUS status;

    UNREFERENCED_PARAMETER(PreviousState);

    deviceContext = GetDeviceContext(Device);
    KdPrint(("DeviceEnum: Entering D0 state\n"));

    // Enumerate device tree
    status = EnumerateDeviceTree(Device);
    if (!NT_SUCCESS(status)) {
        KdPrint(("DeviceEnum: EnumerateDeviceTree failed: 0x%X\n", status));
    }

    return STATUS_SUCCESS;
}

/*
 * QueryDeviceProperty - Query a single device property
 * Uses modern IoGetDevicePropertyData API (not legacy IoGetDeviceProperty)
 */
_Use_decl_annotations_
NTSTATUS QueryDeviceProperty(
    WDFDEVICE Device,
    const DEVPROPKEY *PropertyKey,
    PVOID *PropertyData,
    PULONG PropertyDataSize
)
{
    NTSTATUS status;
    PDEVICE_OBJECT pdo;
    DEVPROPTYPE propertyType;
    ULONG requiredSize = 0;
    PVOID buffer = NULL;
    LARGE_INTEGER startTime, endTime;
    PDEVICE_CONTEXT deviceContext;

    // Must be at PASSIVE_LEVEL for registry access
    if (KeGetCurrentIrql() != PASSIVE_LEVEL) {
        KdPrint(("DeviceEnum: QueryDeviceProperty called at wrong IRQL!\n"));
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    deviceContext = GetDeviceContext(Device);
    pdo = WdfDeviceWdmGetPhysicalDevice(Device);

    // Start timing
    KeQuerySystemTime(&startTime);

    // First call to get required size
    status = IoGetDevicePropertyData(
        pdo,
        PropertyKey,
        LOCALE_NEUTRAL,
        0,
        0,
        NULL,
        &requiredSize,
        &propertyType
    );

    if (status != STATUS_BUFFER_TOO_SMALL && status != STATUS_SUCCESS) {
        KdPrint(("DeviceEnum: IoGetDevicePropertyData query failed: 0x%X\n", status));
        return status;
    }

    // Allocate buffer
    buffer = ExAllocatePoolWithTag(PagedPool, requiredSize, DEVICE_ENUM_POOL_TAG);
    if (buffer == NULL) {
        KdPrint(("DeviceEnum: Failed to allocate %lu bytes\n", requiredSize));
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Second call to get actual data
    status = IoGetDevicePropertyData(
        pdo,
        PropertyKey,
        LOCALE_NEUTRAL,
        0,
        requiredSize,
        buffer,
        &requiredSize,
        &propertyType
    );

    // End timing
    KeQuerySystemTime(&endTime);

    if (NT_SUCCESS(status)) {
        *PropertyData = buffer;
        *PropertyDataSize = requiredSize;
        
        deviceContext->PropertyQueryCount++;
        deviceContext->TotalQueryTime.QuadPart += (endTime.QuadPart - startTime.QuadPart);

        // Log timing (100ns units to microseconds)
        LONGLONG queryTimeMicros = (endTime.QuadPart - startTime.QuadPart) / 10;
        KdPrint(("DeviceEnum: Property query took %lld microseconds\n", queryTimeMicros));
    } else {
        ExFreePoolWithTag(buffer, DEVICE_ENUM_POOL_TAG);
        KdPrint(("DeviceEnum: IoGetDevicePropertyData failed: 0x%X\n", status));
    }

    return status;
}

/*
 * EnumerateDeviceTree - Walk device tree and query properties
 */
_Use_decl_annotations_
NTSTATUS EnumerateDeviceTree(
    WDFDEVICE Device
)
{
    NTSTATUS status;
    PVOID propertyData = NULL;
    ULONG propertySize = 0;

    KdPrint(("DeviceEnum: Starting device tree enumeration\n"));

    // Query Device Description
    status = QueryDeviceProperty(
        Device,
        &DEVPKEY_Device_DeviceDesc,
        &propertyData,
        &propertySize
    );

    if (NT_SUCCESS(status)) {
        KdPrint(("DeviceEnum: Device Description: %ws\n", (PWSTR)propertyData));
        ExFreePoolWithTag(propertyData, DEVICE_ENUM_POOL_TAG);
        propertyData = NULL;
    }

    // Query Hardware IDs
    status = QueryDeviceProperty(
        Device,
        &DEVPKEY_Device_HardwareIds,
        &propertyData,
        &propertySize
    );

    if (NT_SUCCESS(status)) {
        // Multi-string, print first ID
        KdPrint(("DeviceEnum: Hardware ID: %ws\n", (PWSTR)propertyData));
        ExFreePoolWithTag(propertyData, DEVICE_ENUM_POOL_TAG);
        propertyData = NULL;
    }

    // Query Instance ID
    status = QueryDeviceProperty(
        Device,
        &DEVPKEY_Device_InstanceId,
        &propertyData,
        &propertySize
    );

    if (NT_SUCCESS(status)) {
        KdPrint(("DeviceEnum: Instance ID: %ws\n", (PWSTR)propertyData));
        ExFreePoolWithTag(propertyData, DEVICE_ENUM_POOL_TAG);
        propertyData = NULL;
    }

    // Query Device Class GUID
    status = QueryDeviceProperty(
        Device,
        &DEVPKEY_Device_ClassGuid,
        &propertyData,
        &propertySize
    );

    if (NT_SUCCESS(status)) {
        KdPrint(("DeviceEnum: Class GUID: %ws\n", (PWSTR)propertyData));
        ExFreePoolWithTag(propertyData, DEVICE_ENUM_POOL_TAG);
        propertyData = NULL;
    }

    KdPrint(("DeviceEnum: Device tree enumeration complete\n"));
    return STATUS_SUCCESS;
}
