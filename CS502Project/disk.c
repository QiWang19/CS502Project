#include			"disk.h"
#include			"oscreateProcess.h"
#include			 "stdio.h"
#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"
#include             <stdlib.h>
#include             <ctype.h>

#define                  DO_LOCK                     1
#define                  DO_UNLOCK                   0
#define                  SUSPEND_UNTIL_LOCKED        TRUE
#define                  DO_NOT_SUSPEND              FALSE
//Global variables
struct Disk_Queue* headDisk = NULL;
struct Disk_Queue* rearDisk = NULL;
long lenDiskQ = 0;
//in oscreateprocess.c
extern struct PCB_Queue* curtProcessPCB;
extern long exitInterrupt;

//Add the curt PCB to disk queue
int addToDiskQueue() {
	struct Disk_Queue* newDisk;
	newDisk = (struct Disk_Queue*)malloc(sizeof(struct Disk_Queue));
	newDisk->curtPCB = curtProcessPCB;
	newDisk->next = NULL;
	//if (headDisk == NULL && rearDisk == NULL) {
	if (lenDiskQ == 0) {
		headDisk = rearDisk = newDisk;
		lenDiskQ = lenDiskQ + 1;
		return 1;
	}
	else if (rearDisk != NULL){
		rearDisk->next = newDisk;
		rearDisk = newDisk;
		lenDiskQ = lenDiskQ + 1;
		return 1;
	}
	return 0;
}

//delete one element from the head of the disk queue
struct PCB_Queue* delFromDiskQueue() {
	struct Disk_Queue* p = headDisk;
	if (headDisk != NULL) {
		headDisk = headDisk->next;
		lenDiskQ = lenDiskQ - 1;
		/*if (headDisk == NULL) {
			rearDisk = NULL;
		}*/
		if (lenDiskQ == 0) {
			headDisk = NULL;
			rearDisk = NULL;
		}
		return p->curtPCB;
	}
	return NULL;
}

//Used in interrupt handler, delete one element from disk queue and add it to ready queue
void updateDiskQueue(int DiskID) {
	

	exitInterrupt = 0;
	INT32 LockResult;
	//READ_MODIFY(MEMORY_INTERLOCK_BASE + 3 + DiskID, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
	struct PCB_Queue* p = delFromDiskQueue();

	//READ_MODIFY(MEMORY_INTERLOCK_BASE + 3 + DiskID, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
	addToReadyQueue(p);

	exitInterrupt = 1;
}

