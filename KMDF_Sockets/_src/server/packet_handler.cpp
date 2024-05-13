#include "ntifs.h"
#include "server_shared.h"
#include "sockets.h"
#include "../kernel/imports.h"
#include "../kernel/log.h"
#include <psapi.h>

uint64_t RDrvGetModuleEntry(PEPROCESS Process, UNICODE_STRING
	module_name)
{
	if (!Process) return STATUS_INVALID_PARAMETER_1;
	PPEB peb = PsGetProcessPeb(Process);

	if (!peb) {
		DbgPrintEx(0, 0, "Error pPeb not found \n");
		return 0;
	}
	KAPC_STATE state;
	KeStackAttachProcess(Process, &state);
	PPEB_LDR_DATA ldr = peb->Ldr;

	if (!ldr) 
	{
		DbgPrintEx(0, 0, "Error pLdr not found \n");
		KeUnstackDetachProcess(&state);
		return 0; // failed
	}

	for (PLIST_ENTRY listEntry = (PLIST_ENTRY)ldr->ModuleListLoadOrder.Flink;
		listEntry != &ldr->ModuleListLoadOrder;
		listEntry = (PLIST_ENTRY)listEntry->Flink)
	{

		PLDR_DATA_TABLE_ENTRY ldrEntry = CONTAINING_RECORD(listEntry, LDR_DATA_TABLE_ENTRY, InLoadOrderModuleList);
		
		log("DllName: %wZ, Length: %d\n", ldrEntry->BaseDllName, ldrEntry->BaseDllName.Length);
		
		if (RtlCompareUnicodeString(&ldrEntry->BaseDllName, &module_name, FALSE) == 0) {
			log("Found!");
			ULONG64 baseAddr = (ULONG64)ldrEntry->DllBase;
			KeUnstackDetachProcess(&state);
			return baseAddr;
		}

	}

	DbgPrintEx(0, 0, "Error exiting funcion nothing was found found \n");
	KeUnstackDetachProcess(&state);

	return 0;
}


static uint64_t handle_copy_memory(const PacketCopyMemory& packet)
{
	PEPROCESS dest_process = nullptr;
	PEPROCESS src_process = nullptr;

	if (!NT_SUCCESS(PsLookupProcessByProcessId(HANDLE(packet.dest_process_id), &dest_process)))
	{
		return uint64_t(STATUS_INVALID_CID);
	}

	if (!NT_SUCCESS(PsLookupProcessByProcessId(HANDLE(packet.src_process_id), &src_process)))
	{
		ObDereferenceObject(dest_process);
		return uint64_t(STATUS_INVALID_CID);
	}


	SIZE_T   return_size = 0;
	NTSTATUS status = MmCopyVirtualMemory(
		src_process,
		(void*)packet.src_address,
		dest_process,
		(void*)packet.dest_address,
		packet.size,
		UserMode,
		&return_size
	);


	ObDereferenceObject(dest_process);
	ObDereferenceObject(src_process);

	return uint64_t(status);
}

static uint64_t handle_get_base_address(const PacketGetBaseAddress& packet)
{
	PEPROCESS process = nullptr;
	NTSTATUS  status = PsLookupProcessByProcessId(HANDLE(packet.process_id), &process);

	if (!NT_SUCCESS(status))
		return 0;

	const auto base_address = uint64_t(PsGetProcessSectionBaseAddress(process));
	ObDereferenceObject(process);

	return base_address;
}

static uint64_t handle_get_peb(const PacketGetBasePeb& packet)
{
	PEPROCESS process = nullptr;
	NTSTATUS  status = PsLookupProcessByProcessId(HANDLE(packet.process_id), &process);

	if (!NT_SUCCESS(status))
		return false;

	PVOID pebAddress = PsGetProcessPeb(process);
	const uint64_t base_address = (uint64_t)pebAddress;

	//RtlInitUnicodeString(&DLLName, L"UnityPlayer.dll");
	//const auto base_address = RDrvGetModuleEntry(process, DLLName);
	//

	ObDereferenceObject(process);
	return base_address;
}

static uint64_t handle_get_module(const PacketGetModule& packet)
{
	PEPROCESS process = nullptr;
	NTSTATUS  status = PsLookupProcessByProcessId(HANDLE(packet.process_id), &process);

	if (!NT_SUCCESS(status))
		return false;

	UNICODE_STRING DLLName;
	RtlInitUnicodeString(&DLLName, (PCWSTR)packet.moduleName);
	const auto base_address = RDrvGetModuleEntry(process, DLLName);

	ObDereferenceObject(process);

	log((char*)base_address);

	return base_address;
}

// Send completion packet.
bool complete_request(const SOCKET client_connection, const uint64_t result)
{
	Packet packet{ };

	packet.header.magic = packet_magic;
	packet.header.type = PacketType::packet_completed;
	packet.data.completed.result = result;

	return send(client_connection, &packet, sizeof(packet), 0) != SOCKET_ERROR;
}

static uintptr_t get_kernel_address(const char* name, size_t& size) {
	NTSTATUS status = STATUS_SUCCESS;
	ULONG neededSize = 0;

	ZwQuerySystemInformation(
		SystemModuleInformation,
		&neededSize,
		0,
		&neededSize
	);

	PSYSTEM_MODULE_INFORMATION pModuleList;

	pModuleList = (PSYSTEM_MODULE_INFORMATION)ExAllocatePool(NonPagedPool, neededSize);

	if (!pModuleList) {
		log("ExAllocatePoolWithTag failed(kernel addr)\n");
		return 0;
	}

	status = ZwQuerySystemInformation(SystemModuleInformation,
		pModuleList,
		neededSize,
		0
	);

	ULONG i = 0;
	uintptr_t address = 0;

	for (i = 0; i < pModuleList->ulModuleCount; i++)
	{
		SYSTEM_MODULE mod = pModuleList->Modules[i];

		address = uintptr_t(pModuleList->Modules[i].Base);
		size = uintptr_t(pModuleList->Modules[i].Size);
		if (strstr(mod.ImageName, name) != NULL)
			break;
	}

	ExFreePool(pModuleList);

	return address;
}

uint64_t handle_incoming_packet(const Packet& packet)
{
	switch (packet.header.type)
	{
	case PacketType::packet_copy_memory:
		return handle_copy_memory(packet.data.copy_memory);

	case PacketType::packet_get_base_address:
		return handle_get_base_address(packet.data.get_base_address);

	case PacketType::packet_get_peb:
		return handle_get_peb(packet.data.get_base_peb);
	
	case PacketType::packet_get_module:
		return handle_get_module(packet.data.get_module_base);

	default:
		break;
	}

	return uint64_t(STATUS_NOT_IMPLEMENTED);
}
