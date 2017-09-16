#include			"disk.h"
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

void addToDiskQueue() {
	if (headDisk == rearDisk == NULL) {
		
	}
}

struct DiskQueue* delFromDiskQueue() {

}

