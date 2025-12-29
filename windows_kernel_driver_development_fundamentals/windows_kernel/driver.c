#include <ntddk.h>
#include <wdf.h>

#define DEVICE_NAME L"\\Device\\EchoDevice"
#define SYMBOLIC_LINK L"\\DosDevices\\EchoDevice"

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD EchoEvtDeviceAdd;
EVT_WDF_IO_QUEUE_IO_READ EchoEvtIoRead;
EVT_WDF_IO_QUEUE_IO_WRITE EchoEvtIoWrite;

typedef struct _DEVICE_CONTEXT {
    WDFQUEUE DefaultQueue;
    UCHAR Buffer[4096];
    SIZE_T BufferLength;
    WDFSPINLOCK BufferLock;
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, DeviceGetContext)

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
    NTSTATUS status;
    WDF_DRIVER_CONFIG config;
    
    KdPrint(("EchoSample: DriverEntry called\n"));
    
    WDF_DRIVER_CONFIG_INIT(&config, EchoEvtDeviceAdd);
    
    status = WdfDriverCreate(DriverObject, RegistryPath, WDF_NO_OBJECT_ATTRIBUTES, &config, WDF_NO_HANDLE);
    
    if (!NT_SUCCESS(status)) {
        KdPrint(("EchoSample: WdfDriverCreate failed: 0x%X\n", status));
        return status;
    }
    
    KdPrint(("EchoSample: Driver initialized successfully\n"));
    return STATUS_SUCCESS;
}

NTSTATUS EchoEvtDeviceAdd(_In_ WDFDRIVER Driver, _Inout_ PWDFDEVICE_INIT DeviceInit)
{
    NTSTATUS status;
    WDFDEVICE device;
    WDF_OBJECT_ATTRIBUTES deviceAttributes;
    WDF_IO_QUEUE_CONFIG queueConfig;
    PDEVICE_CONTEXT deviceContext;
    UNICODE_STRING deviceName, symbolicLink;
    WDF_OBJECT_ATTRIBUTES lockAttributes;
    
    UNREFERENCED_PARAMETER(Driver);
    
    RtlInitUnicodeString(&deviceName, DEVICE_NAME);
    status = WdfDeviceInitAssignName(DeviceInit, &deviceName);
    if (!NT_SUCCESS(status)) return status;
    
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, DEVICE_CONTEXT);
    status = WdfDeviceCreate(&DeviceInit, &deviceAttributes, &device);
    if (!NT_SUCCESS(status)) return status;
    
    deviceContext = DeviceGetContext(device);
    deviceContext->BufferLength = 0;
    
    WDF_OBJECT_ATTRIBUTES_INIT(&lockAttributes);
    lockAttributes.ParentObject = device;
    status = WdfSpinLockCreate(&lockAttributes, &deviceContext->BufferLock);
    if (!NT_SUCCESS(status)) return status;
    
    RtlInitUnicodeString(&symbolicLink, SYMBOLIC_LINK);
    status = WdfDeviceCreateSymbolicLink(device, &symbolicLink);
    if (!NT_SUCCESS(status)) return status;
    
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchSequential);
    queueConfig.EvtIoRead = EchoEvtIoRead;
    queueConfig.EvtIoWrite = EchoEvtIoWrite;
    
    status = WdfIoQueueCreate(device, &queueConfig, WDF_NO_OBJECT_ATTRIBUTES, &deviceContext->DefaultQueue);
    if (!NT_SUCCESS(status)) return status;
    
    KdPrint(("EchoSample: Device created successfully\n"));
    return STATUS_SUCCESS;
}

VOID EchoEvtIoWrite(_In_ WDFQUEUE Queue, _In_ WDFREQUEST Request, _In_ size_t Length)
{
    NTSTATUS status;
    PVOID buffer;
    size_t bufferLength;
    WDFDEVICE device = WdfIoQueueGetDevice(Queue);
    PDEVICE_CONTEXT deviceContext = DeviceGetContext(device);
    
    KdPrint(("EchoSample: EchoEvtIoWrite called, Length=%llu\n", (ULONGLONG)Length));
    
    status = WdfRequestRetrieveInputBuffer(Request, 1, &buffer, &bufferLength);
    if (!NT_SUCCESS(status)) {
        WdfRequestComplete(Request, status);
        return;
    }
    
    if (bufferLength > sizeof(deviceContext->Buffer)) {
        bufferLength = sizeof(deviceContext->Buffer);
    }
    
    WdfSpinLockAcquire(deviceContext->BufferLock);
    RtlCopyMemory(deviceContext->Buffer, buffer, bufferLength);
    deviceContext->BufferLength = bufferLength;
    WdfSpinLockRelease(deviceContext->BufferLock);
    
    WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, bufferLength);
}

VOID EchoEvtIoRead(_In_ WDFQUEUE Queue, _In_ WDFREQUEST Request, _In_ size_t Length)
{
    NTSTATUS status;
    PVOID buffer;
    size_t bufferLength;
    size_t bytesToCopy;
    WDFDEVICE device = WdfIoQueueGetDevice(Queue);
    PDEVICE_CONTEXT deviceContext = DeviceGetContext(device);
    
    KdPrint(("EchoSample: EchoEvtIoRead called, Length=%llu\n", (ULONGLONG)Length));
    
    status = WdfRequestRetrieveOutputBuffer(Request, 1, &buffer, &bufferLength);
    if (!NT_SUCCESS(status)) {
        WdfRequestComplete(Request, status);
        return;
    }
    
    WdfSpinLockAcquire(deviceContext->BufferLock);
    bytesToCopy = bufferLength < deviceContext->BufferLength ? bufferLength : deviceContext->BufferLength;
    
    if (bytesToCopy > 0) {
        RtlCopyMemory(buffer, deviceContext->Buffer, bytesToCopy);
    }
    WdfSpinLockRelease(deviceContext->BufferLock);
    
    WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, bytesToCopy);
}
