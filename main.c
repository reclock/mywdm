#include <ntdef.h>
#include <wdm.h>

#define INITCODE code_seg("INIT")
#define LOCKEDCODE code_seg()
#define PAGEDCODE code_seg("PAGE")

#define INITDATA data_seg("INIT")
#define LOCKEDDATA data_seg()
#define PAGEDDATA data_seg(PAGE)

#define arrarysize(p) (sizeof(p)/sizeof((p)[0]))

typedef struct _DEVICE_EXTENSION
{
	PDEVICE_OBJECT fdo;
	PDEVICE_OBJECT pNextStackDevice;
	UNICODE_STRING ustrDevName;
	UNICODE_STRING ustrLinkName;
}DEVICE_EXTENSION, *PDEVICE_EXTENSION;

NTSTATUS AddDevice(IN PDRIVER_OBJECT pDriverObject, IN PDEVICE_OBJECT pdo);
NTSTATUS DefaultPnp(IN PDEVICE_OBJECT fdo, IN PIRP pIrp);
NTSTATUS DefaultDispatchRoutine(IN PDEVICE_OBJECT fdo, IN PIRP pIrp);
VOID Unload(IN PDRIVER_OBJECT pDriverObject);
NTSTATUS RemovableDevice(IN PDEVICE_EXTENSION pdx, IN PIRP pIrp);
NTSTATUS HandlerPnp(IN PDEVICE_EXTENSION pdx, IN PIRP pIrp);

//HelloWDM.cpp



#pragma INITCODE
DriverEntry(IN PDRIVER_OBJECT pDriverObject, IN PUNICODE_STRING pRegisterPath)
{
	KdPrint(("DriverEntry:Enter\n"));

	pDriverObject->DriverExtension->AddDevice = AddDevice;
	pDriverObject->MajorFunction[IRP_MJ_PNP] = DefaultPnp;
	pDriverObject->MajorFunction[IRP_MJ_CREATE] = DefaultDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_READ] = DefaultDispatchRoutine;
	pDriverObject->MajorFunction[IRP_MJ_WRITE] = DefaultDispatchRoutine;
	pDriverObject->DriverUnload = Unload;

	KdPrint(("DriverEntry:Leave\n"));

	return STATUS_SUCCESS;
}

#pragma PAGEDCODE
NTSTATUS AddDevice(IN PDRIVER_OBJECT pDriverObject, IN PDEVICE_OBJECT pdo)
{
	PAGED_CODE();
	KdPrint(("HelloWDMAddDevice:Enter\n"));
	PDEVICE_OBJECT fdo;
	NTSTATUS status;
	UNICODE_STRING devName;
	UNICODE_STRING linkName;
	PDEVICE_EXTENSION pdx;

	//创建设备
	RtlInitUnicodeString(&devName, L"\\Device\\MyHelloWDM");
	status = IoCreateDevice(pDriverObject,
		sizeof(DEVICE_EXTENSION),
		&devName,
		FILE_DEVICE_UNKNOWN,
		0,
		FALSE,
		&fdo);
	if (!NT_SUCCESS(status))
	{
		return status;
	}
	//创建符号链接
	RtlInitUnicodeString(&linkName, L"\\??\\HelloWDM");
	status = IoCreateSymbolicLink(&linkName, &devName);
	if (!NT_SUCCESS(status))
	{
		IoDeleteDevice(fdo);
		return status;
	}
	//设备扩展
	pdx = (PDEVICE_EXTENSION)fdo->DeviceExtension;
	pdx->fdo = fdo;
	pdx->ustrDevName = devName;
	pdx->ustrLinkName = linkName;
	//挂接到设备栈
	pdx->pNextStackDevice = IoAttachDeviceToDeviceStack(fdo, pdo);
	if (pdx->pNextStackDevice == NULL)
	{
		IoDeleteSymbolicLink(&pdx->ustrLinkName);
		IoDeleteDevice(fdo);
		return STATUS_UNSUCCESSFUL;
	}
	//设备标志
	fdo->Flags |= DO_BUFFERED_IO | DO_POWER_PAGABLE;
	fdo->Flags &= ~DO_DEVICE_INITIALIZING;

	KdPrint(("HelloWDMAddDevice:Leave\n"));
	return status;
}

#pragma PAGEDCODE
VOID Unload(IN PDRIVER_OBJECT pDriverObject)
{
	PAGED_CODE();
	//此卸载例程不做任何事情，交由IRP_MN_REMOVABLE_DEVICE处理
	KdPrint(("HelloWDMUnload:Enter\n"));
	KdPrint(("HelloWDMUnload:Leave\n"));
}

#pragma PAGEDCODE
NTSTATUS DefaultDispatchRoutine(IN PDEVICE_OBJECT fdo, IN PIRP pIrp)
{
	PAGED_CODE();
	//本层驱动直接处理irp
	KdPrint(("HelloWDMDefaultDispatchRoutine:Enter\n"));

	pIrp->IoStatus.Status = STATUS_SUCCESS;
	pIrp->IoStatus.Information = 0;
	IoCompleteRequest(pIrp, IO_NO_INCREMENT);

	KdPrint(("HelloWDMDefaultDispatchRoutine:Leave\n"));
	return STATUS_SUCCESS;
}

#pragma PAGEDCODE
NTSTATUS DefaultPnp(IN PDEVICE_OBJECT fdo, IN PIRP pIrp)
{
	PAGED_CODE();
	KdPrint(("HelloWDMDefaultPnp:Enter\n"));
	NTSTATUS status;
	PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(pIrp);
	PDEVICE_EXTENSION pdx = (PDEVICE_EXTENSION)fdo->DeviceExtension;
	ULONG fcn;

	NTSTATUS(*fcntab[])(PDEVICE_EXTENSION, PIRP) =
	{
	 HandlerPnp,
	 HandlerPnp,
	 RemovableDevice,
	 HandlerPnp,
	 HandlerPnp,
	 HandlerPnp,
	 HandlerPnp,
	 HandlerPnp,
	 HandlerPnp,
	 HandlerPnp,
	 HandlerPnp,
	 HandlerPnp,
	 HandlerPnp,
	 HandlerPnp,

	 HandlerPnp,

	 HandlerPnp,
	 HandlerPnp,
	 HandlerPnp,
	 HandlerPnp,
	 HandlerPnp,
	 HandlerPnp,
	 HandlerPnp,
	 HandlerPnp,
	 HandlerPnp,
	};
	fcn = stack->MinorFunction;
	if (fcn >= arrarysize(fcntab))
	{
		status = HandlerPnp(pdx, pIrp);
		KdPrint(("HelloWDMDefaultPnp:Leave\n"));
		return status;
	}
	char* fcnName[] =
	{
	 "IRP_MN_START_DEVICE",
	 "IRP_MN_QUERY_REMOVE_DEVICE",
	 "IRP_MN_REMOVE_DEVICE",
	 "IRP_MN_CANCEL_REMOVE_DEVICE",
	 "IRP_MN_STOP_DEVICE",
	 "IRP_MN_QUERY_STOP_DEVICE",
	 "IRP_MN_CANCEL_STOP_DEVICE",
	 "IRP_MN_QUERY_DEVICE_RELATIONS",
	 "IRP_MN_QUERY_INTERFACE",
	 "IRP_MN_QUERY_CAPABILITIES",
	 "IRP_MN_QUERY_RESOURCES",
	 "IRP_MN_QUERY_RESOURCE_REQUIREMENTS",
	 "IRP_MN_QUERY_DEVICE_TEXT",
	 "IRP_MN_FILTER_RESOURCE_REQUIREMENTS",
	 "",
	 "IRP_MN_READ_CONFIG",
	 "IRP_MN_WRITE_CONFIG",
	 "IRP_MN_EJECT",
	 "IRP_MN_SET_LOCK",
	 "IRP_MN_QUERY_ID",
	 "IRP_MN_QUERY_PNP_DEVICE_STATE",
	 "IRP_MN_QUERY_BUS_INFORMATION",
	 "IRP_MN_DEVICE_USAGE_NOTIFICATION",
	 "IRP_MN_SURPRISE_REMOVAL",
	};
	KdPrint(("PNP Request (%s)\n", fcnName[fcn]));

	status = (*fcntab[fcn])(pdx, pIrp);

	KdPrint(("HelloWDMDefaultPnp:Leave\n"));
	return status;
}

#pragma PAGEDCODE
NTSTATUS HandlerPnp(IN PDEVICE_EXTENSION pdx, IN PIRP pIrp)
{
	//交由下层处理IRP
	PAGED_CODE();
	NTSTATUS status = STATUS_SUCCESS;
	KdPrint(("HandlerPnp:Enter\n"));

	IoSkipCurrentIrpStackLocation(pIrp);
	status = IoCallDriver(pdx->pNextStackDevice, pIrp);

	KdPrint(("HandlerPnp:Leave\n"));
	return status;
}

#pragma PAGEDCODE
NTSTATUS RemovableDevice(IN PDEVICE_EXTENSION pdx, IN PIRP pIrp)
{
	PAGED_CODE();
	KdPrint(("RemovableDevice:Enter\n"));
	NTSTATUS status = STATUS_SUCCESS;
	//交由下层处理irp
	status = HandlerPnp(pdx, pIrp);
	//解除挂载
	if (pdx->pNextStackDevice)
	{
		IoDetachDevice(pdx->pNextStackDevice);
	}
	//删除符号链接
	IoDeleteSymbolicLink(&pdx->ustrLinkName);
	//删除设备
	IoDeleteDevice(pdx->fdo);

	KdPrint(("RemovableDevice:Leave\n"));
	return status;
}