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
//Ready Queue
struct Ready_Queue* headReadyQ = NULL;
struct Ready_Queue* rearReadyQ = NULL;
//PID
long PID = 7;
//PCB_Queue length
long lenPCBQ = 0;
//TODO:
struct PCB_Queue* curtProcessPCB = NULL;

//struct OS_Structures {
//	struct Process_PCB pcb;
//
//};
void addToPCBQueue(struct Process_PCB* pcb) {
	struct PCB_Queue* curtPCB;
	curtPCB = (struct PCB_Queue*)malloc(sizeof(struct PCB_Queue));
	curtPCB->pcb = *pcb;
	curtPCB->next = NULL;
	if (headPCB == NULL && rearPCB == NULL) {
		headPCB = rearPCB = curtPCB;
	}
	else {
		rearPCB->next = curtPCB;
		rearPCB = curtPCB;
	}
	lenPCBQ = lenPCBQ + 1;
}

void os_create_process(char* ProcessName, long StartingAddress, long InitialPriority, long* ProcessID, long* ErrorReturned) {
	MEMORY_MAPPED_IO mmio;
	struct Process_PCB pcb;
	struct PCB_Queue* curtPCB;
	void *PageTable = (void*)calloc(2, NUMBER_VIRTUAL_PAGES);
	pcb.PageTable = PageTable;
	pcb.process_ID = PID + 1;
	//Set pcb name
	pcb.process_Name = (char*)malloc(256);
	strcpy(pcb.process_Name, ProcessName);
	//char* ProcessName = "test3";

	mmio.Mode = Z502InitializeContext;
	mmio.Field1 = 0;
	mmio.Field2 = (long)StartingAddress;
	mmio.Field3 = (long)PageTable;
	mmio.Field4 = 0;
	MEM_WRITE(Z502Context, &mmio);
	if (mmio.Field4 != ERR_SUCCESS) {
		printf("Initialize Context has an error\n");
		exit(0);
	}
	pcb.NewContext = mmio.Field1;
	pcb.processPriority = InitialPriority;

	//Add curt PCB to PCB queue
	curtPCB = (struct PCB_Queue*)malloc(sizeof(struct PCB_Queue));
	curtPCB->pcb = pcb;
	curtPCB->next = NULL;
	if (headPCB == NULL && rearPCB == NULL) {
		headPCB = rearPCB = curtPCB;
	}
	else {
		rearPCB->next = curtPCB;
		rearPCB = curtPCB;
	}
	lenPCBQ = lenPCBQ + 1;
	curtProcessPCB = rearPCB;

	mmio.Mode = Z502StartContext;
	mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
	MEM_WRITE(Z502Context, &mmio);
	if (mmio.Field4 != ERR_SUCCESS) {
		printf("Start Context has an error\n");
		exit(0);
	}
		
}

void addToTimerQueue(long sleepTime) {
	struct timer_Queue* curt;
	struct timer_Queue* q;
	struct timer_Queue* p = (struct timer_Queue*)malloc(sizeof(struct timer_Queue));
	p->next = headTimer;

	curt = (struct timer_Queue*)malloc(sizeof(struct timer_Queue));
	//curt->curtPCB = rearPCB;
	curt->curtPCB = curtProcessPCB;
	curt->next = NULL;
	MEMORY_MAPPED_IO mmio;
	mmio.Mode = Z502ReturnValue;
	mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
	MEM_READ(Z502Clock, &mmio);
	curt->timerEndTime = (long)(sleepTime + mmio.Field1);

	if (headTimer == NULL && rearTimer == NULL) {
		headTimer = rearTimer = curt;
	}
	else
	{
		//rearTimer->next = curt;
		//rearTimer = curt;
		while (p->next != NULL && p->next->timerEndTime <= curt->timerEndTime)
		{
			p = p->next;
		}
	
		curt->next = p->next;
		p->next = curt;
		if (curt->next == headTimer) {
			headTimer = curt;
		}
		else if (curt->next == NULL) {
			rearTimer = curt;
		}
		free(p);
	}
	
}

int isDelFromTimerQueue() {
	//int ans = 0;
	struct timer_Queue* p = headTimer;
	struct PCB_Queue* q = rearPCB;
	while (p != NULL) {
		if (p != NULL && p->curtPCB->pcb.process_ID == q->pcb.process_ID) {
			return 0;
		}
		else {
			p = p->next;
		}
	}
	return 1;
}

void delFromTimerQueue() {
	struct timer_Queue* p = headTimer;
	if (headTimer != NULL) {
		headTimer = headTimer->next;
	}
	free(p);	//p is the PCB has been deleted
}

void addToReadyQueue(struct PCB_Queue* curtPCB)
{
	struct Ready_Queue* newPCB = (struct Ready_Queue*)malloc(sizeof(struct Ready_Queue));
	newPCB->curtPCB = curtPCB;
	newPCB->next = NULL;
	if (headReadyQ == NULL && rearReadyQ == NULL) {
		headReadyQ = rearReadyQ = newPCB;
	}
	else {
		rearReadyQ->next = newPCB;
		rearReadyQ = newPCB;
	}
}

void delFromReadyQueue()
{
}

void createProcesTest3(char* ProcessName, long StartingAddress, long InitialPriority, long* ProcessID, long* ErrorReturned) {
	MEMORY_MAPPED_IO mmio;
	struct Process_PCB pcb;
	struct PCB_Queue* curtPCB;
	void *PageTable = (void*)calloc(2, NUMBER_VIRTUAL_PAGES);
	pcb.PageTable = PageTable;
	pcb.process_ID = PID + 1;

	mmio.Mode = Z502InitializeContext;
	mmio.Field1 = 0;
	mmio.Field2 = (long)StartingAddress;
	mmio.Field3 = (long)PageTable;
	mmio.Field4 = 0;
	MEM_WRITE(Z502Context, &mmio);
	if (mmio.Field4 != ERR_SUCCESS) {
		printf("Initialize Context has an error\n");
		exit(0);
	}
	*ErrorReturned = mmio.Field4;
	pcb.NewContext = mmio.Field1;
	pcb.process_Name = (char *)malloc(256);
	strcpy(pcb.process_Name, ProcessName);
	pcb.processPriority = InitialPriority;
	*ProcessID = pcb.process_ID;

	//start context no need here
	/*mmio.Mode = Z502StartContext;
	mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
	MEM_WRITE(Z502Context, &mmio);
	if (mmio.Field4 != ERR_SUCCESS) {
		printf("Start Context has an error\n");
		exit(0);
	}*/
	//Add curt PCB to PCB queue
	/*curtPCB = (struct PCB_Queue*)malloc(sizeof(struct PCB_Queue));
	curtPCB->pcb = pcb;
	curtPCB->next = NULL;
	if (headPCB == NULL && rearPCB == NULL) {
		headPCB = rearPCB = curtPCB;
	}
	else {
		rearPCB->next = curtPCB;
		rearPCB = curtPCB;
	}
	lenPCBQ = lenPCBQ + 1;*/
	addToPCBQueue(&pcb);
}

