#include			"oscreateProcess.h"
#include			 "stdio.h"
#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"
#include             <stdlib.h>
#include             <ctype.h>
//Global variables
//PCB Queue
struct PCB_Queue* headPCB = NULL;
struct PCB_Queue* rearPCB = NULL;
//Timer Queue
struct timer_Queue* headTimer = NULL;
struct timer_Queue* rearTimer = NULL;
//PID
long PID = 773;

//struct OS_Structures {
//	struct Process_PCB pcb;
//
//};

void os_create_process(int argc, char *argv[]) {
	MEMORY_MAPPED_IO mmio;
	struct Process_PCB pcb;
	struct PCB_Queue* curtPCB;
	void *PageTable = (void*)calloc(2, NUMBER_VIRTUAL_PAGES);
	pcb.PageTable = PageTable;
	pcb.process_ID = PID + 1;
	
	mmio.Mode = Z502InitializeContext;
	mmio.Field1 = 0;
	mmio.Field2 = (long)test2;
	mmio.Field3 = (long)PageTable;
	mmio.Field4 = 0;
	MEM_WRITE(Z502Context, &mmio);
	if (mmio.Field4 != ERR_SUCCESS) {
		printf("Initialize Context has an error\n");
		exit(0);
	}
	pcb.NewContext = mmio.Field1;
	
	//Add curt PCB to PCB queue
	curtPCB = (struct PCB_Queue*)malloc(sizeof(struct PCB_Queue));
	curtPCB->pcb = pcb;
	curtPCB->next = NULL;
	if (headPCB == NULL && rearPCB == NULL) {
		headPCB = rearPCB = curtPCB;
	}
	else {
		rearPCB->next = curtPCB;
	}
	mmio.Mode = Z502StartContext;
	mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
	MEM_WRITE(Z502Context, &mmio);
	if (mmio.Field4 != ERR_SUCCESS) {
		printf("Start Context has an error\n");
		exit(0);
	}
}

void addToTimerQueue() {
	struct timer_Queue* curt;
	curt = (struct timer_Queue*)malloc(sizeof(struct timer_Queue));
	curt->curtPCB = rearPCB;
	curt->next = NULL;

	if (headTimer == NULL && rearTimer == NULL) {
		headTimer = rearTimer = curt;
	}
	else
	{
		rearTimer->next = curt;
	}
	
}

struct timer_Queue* delFromTimerQueue() {
	struct timer_Queue* p = headTimer;
	if (headTimer != NULL) {
		headTimer = headTimer->next;
	}	
	return p;		//p is the PCB has been deleted
}

void addToDiskQueue() {

}