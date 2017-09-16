#include "oscreateProcess.h"
struct Disk_Queue{
	struct Process_PCB pcb;
	long DiskID;
	struct Disk_Queue* next;
};

void addToDiskQueue();
struct DiskQueue* delFromDiskQueue();