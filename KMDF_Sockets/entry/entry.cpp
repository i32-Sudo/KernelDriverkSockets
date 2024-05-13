#include <ntifs.h>

#include "../clean/clean.hpp"	
#include "../kernel/log.h"
#include "../clean/clean.hpp"

extern void NTAPI initiliaze_sys(void*);


void CleanDriverFromName(UNICODE_STRING driver_int, ULONG timeDateStamp) {
	if (clear::clearCache(driver_int, timeDateStamp) == 0) {
		log(skCrypt("PiDDB Cache Cleared!"));
	}
	else {
		log(skCrypt("PiDDB Cache Exception Thrown non-zero return"));
	}
	if (clear::clearHashBucket(driver_int) == 0) {
		log(skCrypt("HashBucket Cleared!"));
	}
	else {
		log(skCrypt("HashBucket Exception Thrown non-zero return"));
	}
	if (clear::CleanMmu(driver_int) == 0) {
		log(skCrypt("MMU/MML Cleaned!"));
	}
	else {
		log(skCrypt("MMU/MML Exception Thrown non-zero return"));
	}
}

extern "C" NTSTATUS DriverEntry(
	PDRIVER_OBJECT  driver_object,
	PUNICODE_STRING registry_path
)
{
	UNREFERENCED_PARAMETER(driver_object);
	UNREFERENCED_PARAMETER(registry_path);

	KeEnterGuardedRegion();

	/* Clear Cheat Driver then Exploit Driver */
	CleanDriverFromName(UNICODE_STRING(RTL_CONSTANT_STRING(L"ksys.sys")), 0x5284EAC3);

	log(skCrypt("Starting WorkItem In Queue"));
	PWORK_QUEUE_ITEM WorkItem = (PWORK_QUEUE_ITEM)ExAllocatePool(NonPagedPool, sizeof(WORK_QUEUE_ITEM));
	if (WorkItem) {
		ExInitializeWorkItem(WorkItem, initiliaze_sys, WorkItem);
		ExQueueWorkItem(WorkItem, DelayedWorkQueue);
	}
	else {
		log(skCrypt("WorkItem is Null"));
		return STATUS_SUCCESS;
	}

	KeLeaveGuardedRegion();
	return STATUS_SUCCESS;
}