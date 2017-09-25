#include			"disk.h"
#include			"oscreateProcess.h"
#include			 "stdio.h"
#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"
#include             <stdlib.h>
#include             <ctype.h>

//Global variables
struct Disk_Queue* headDisk = NULL;
struct Disk_Queue* rearDisk = NULL;
extern struct PCB_Queue* curtProcessPCB;
extern long exitInterrupt;

void addToDiskQueue() {
	struct Disk_Queue* newDisk;
	newDisk = (struct Disk_Queue*)malloc(sizeof(struct Disk_Queue));
	newDisk->curtPCB = curtProcessPCB;
	newDisk->next = NULL;
	if (headDisk == NULL && rearDisk == NULL) {
		headDisk = rearDisk = newDisk;
	}
	else {
		rearDisk->next = newDisk;
		rearDisk = newDisk;
	}
}

struct PCB_Queue* delFromDiskQueue() {
	struct Disk_Queue* p = headDisk;
	if (headDisk != NULL) {
		headDisk = headDisk->next;
		if (headDisk == NULL) {
			rearDisk = NULL;
		}
		return p->curtPCB;
	}
	return NULL;
}

void updateDiskQueue() {
	exitInterrupt = 0;
	struct PCB_Queue* p = delFromDiskQueue();
	addToReadyQueue(p);
	exitInterrupt = 1;
}

