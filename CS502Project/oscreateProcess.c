#include			"oscreateProcess.h"
#include			 "lock.h"
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
//timer_Queue length
long lenTimerQ = 0;
//readyQ length
long lenReadyQ = 0;
//current running process 
struct PCB_Queue* curtProcessPCB = NULL;
//
long exitInterrupt = 0;

//scheduler print in printScheduler.c
extern int printFullScheduler;

//struct OS_Structures {
//	struct Process_PCB pcb;
//
//};
//return the pointer to PCBQ element of the pcb just be added
struct PCB_Queue* addToPCBQueue(struct Process_PCB* pcb) {
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
	return curtPCB;
}

void os_create_process(char* ProcessName, long StartingAddress, long InitialPriority, long* ProcessID, long* ErrorReturned) {
	MEMORY_MAPPED_IO mmio;
	struct Process_PCB pcb;
	struct PCB_Queue* curtPCB;
	void *PageTable = (void*)calloc(2, NUMBER_VIRTUAL_PAGES);
	pcb.PageTable = PageTable;
	pcb.process_ID = PID + 1;
	PID = PID + 1;
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
	struct timer_Queue* curt = NULL;
	struct timer_Queue* q = NULL;
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
		lenTimerQ = lenTimerQ + 1;
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
		lenTimerQ = lenTimerQ + 1;
		if (curt->next == headTimer) {
			headTimer = curt;
		}
		else if (curt->next == NULL) {
			rearTimer = curt;
		}
		//free(p);
	}
	//==============print=====================
	/*q = headTimer;
	printf("after adding \n");
	while (q != NULL) {
		printf("====================a timer %ld=====", q->timerEndTime);
		q = q->next;
	}
	printf("\n");*/
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

struct PCB_Queue* delFromTimerQueue() {
	struct timer_Queue* p = headTimer;
	//struct PCB_Queue* q = p->curtPCB;
	struct PCB_Queue* q = NULL;
	if (headTimer != NULL) {
		headTimer = headTimer->next;
		if (headTimer == NULL) {
			rearTimer = NULL;
		}
		q = p->curtPCB;
		lenTimerQ = lenTimerQ - 1;
		//free(p);	//p is the PCB has been deleted
	}		
	return q;
}

void addToReadyQueue(struct PCB_Queue* curtPCB)
{
	if (curtPCB == NULL) {
		return;
	}
	struct Ready_Queue* newPCB = (struct Ready_Queue*)malloc(sizeof(struct Ready_Queue));
	newPCB->curtPCB = curtPCB;
	newPCB->next = NULL;
	if (headReadyQ == NULL && rearReadyQ == NULL) {
		headReadyQ = rearReadyQ = newPCB;
		lenReadyQ = lenReadyQ + 1;
		return;
	}
	else if (rearReadyQ != NULL){
		rearReadyQ->next = newPCB;
		rearReadyQ = newPCB;
		lenReadyQ = lenReadyQ + 1;
	}
	//print
	/*if (rearReadyQ == NULL) {
		printf("add null   ===================== \n");
	}
	else {
		printf("=========== add ready Q ============== %ld\n", rearReadyQ->curtPCB->pcb.process_ID);
	}*/
	
}

void delFromReadyQueue()
{
	struct Ready_Queue* p = headReadyQ;
	if (headReadyQ != NULL) {
		headReadyQ = headReadyQ->next;
		
		if (headReadyQ == NULL) {
			rearReadyQ = NULL;
		}
		//free(p);
		lenReadyQ = lenReadyQ - 1;
	}	
	
}
//del from readyQ, start context and suspend curt process
void dispatcher() {
	
	struct Ready_Queue* p = headReadyQ;
	MEMORY_MAPPED_IO mmio;
	char fff = 'a';

	//syscalls.h line 88
	while (TRUE) {
		if (exitInterrupt == 0) {
			CALL(fff);
			if (headReadyQ != NULL) {
				break;
			}
		}

	}
	
	/*if (headReadyQ == NULL ) {
		while (headReadyQ == NULL) {
			CALL(fff);
		}
	}*/
	//test ready
	long testQlen = lenReadyQ;

	p = headReadyQ;
	printScheduler(printFullScheduler, "Dispatch", p->curtPCB->pcb.process_ID);	
	if (headReadyQ != NULL && p != NULL) {
		delFromReadyQueue();
		mmio.Mode = Z502StartContext;
		mmio.Field1 = p->curtPCB->pcb.NewContext;
		mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
		curtProcessPCB = p->curtPCB;
		
		MEM_WRITE(Z502Context, &mmio);
		if (mmio.Field4 != ERR_SUCCESS) {
			printf("Start Context has an error\n");
			exit(0);
		}
		//curtProcessPCB = p->curtPCB;
		//delFromReadyQueue();
	}
}

void updateTimerQueue() {
	
	INT32 LockResult;
	READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_LOCK, DO_NOT_SUSPEND, &LockResult);
	exitInterrupt = 0;
	long curtTime = 0L;
	struct timer_Queue* p = headTimer;
	struct PCB_Queue* q;
	long delay = 0L;
	//Get current time from clock
	MEMORY_MAPPED_IO mmio;
	mmio.Mode = Z502ReturnValue;
	mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
	MEM_READ(Z502Clock, &mmio);
	curtTime = mmio.Field1;
	//traverse the timer queue
	if (headTimer != NULL) {
		while (p != NULL && curtTime >= p->timerEndTime) {		//del p from timerQ, add to readyQ
			p = p->next;
			q = delFromTimerQueue();								//del 1st element in timerQ, head changes
			addToReadyQueue(q);										//add deleted element to readyQ
			//printf("================== update del timerQ ========== %ld\n", q->pcb.process_ID);
		}
		//leave it, start timer
		//p = headTimer;
		if (p != NULL &&  curtTime < p->timerEndTime) {
			delay = p->timerEndTime - curtTime;
			mmio.Mode = Z502Start;
			mmio.Field1 = delay;   // You pick the time units
			mmio.Field2 = mmio.Field3 = mmio.Field4 = 0;
			MEM_WRITE(Z502Timer, &mmio);
			//printf("===============================start the timer\n");
		}
	}
	
	//print
	/*struct timer_Queue* timerq = headTimer;
	printf("========================== after updating ========================= \n");
	if (timerq == NULL) {
		printf("=========== timer Q null ============================\n");
	}
	while (timerq != NULL) {
		printf("====================a timer %ld===id %ld==", timerq->timerEndTime, timerq->curtPCB->pcb.process_ID);
		timerq = timerq->next;
	}
	printf("\n");
	*/
	/*struct Ready_Queue* readyp = headReadyQ;
	if (readyp == NULL) {
		printf("===================== readyQ null ========================\n");
	}
	while (readyp != NULL) {
		printf("readys =============================== %ld", readyp->curtPCB->pcb.process_ID);
		readyp = readyp->next;
	}
	printf("\n");*/
	exitInterrupt = 1;
	READ_MODIFY(MEMORY_INTERLOCK_BASE, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
}

void endProcess(int type, long * ErrorReturned)
{
	MEMORY_MAPPED_IO mmio;
	struct PCB_Queue* p = curtProcessPCB;
	struct PCB_Queue* q = headPCB;
	if (type == -1) {				//stop the current process and dispatch
		curtProcessPCB->pcb.NewContext = -1;
		//lenPCBQ = lenPCBQ - 1;
		if (lenPCBQ >= 2) {
			lenPCBQ = lenPCBQ - 1;
			*ErrorReturned = ERR_SUCCESS;
			dispatcher();
		}
		else if (lenPCBQ == 1) {
			//test readyQ
			struct Ready_Queue* testQ = headReadyQ;
			lenPCBQ = lenPCBQ - 1;
			headPCB = NULL;
			rearPCB = NULL;
			mmio.Mode = Z502Action;
			mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
			MEM_WRITE(Z502Halt, &mmio);
		}
	}
	else if (type == -2) {						//set all NewContext to -1,
												
		lenPCBQ = 0;
		q = headPCB;
		while (q != NULL) {
			q->pcb.NewContext = -1;
			q = q->next;
		}
		headPCB = NULL;
		rearPCB = NULL;
		mmio.Mode = Z502Action;
		mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
		MEM_WRITE(Z502Halt, &mmio);
	}
	else {										//stop the process with the PID
		q = headPCB;
		while (q != NULL && q->pcb.process_ID != type) {
			q = q->next;
		}
		if (q != NULL) {
			if (q->pcb.NewContext == -1) {
				*ErrorReturned = ERR_BAD_PARAM;
			}
			else {
				lenPCBQ = lenPCBQ - 1;
				*ErrorReturned = ERR_SUCCESS;
			}
		}
		else if (q == NULL) {
			*ErrorReturned = ERR_BAD_PARAM;
		}
	}

}



//void printScheduler(int printFullScheduler, char* targetAction, long targetPID)
//{
//	if (printFullScheduler == 0) {
//		return;
//	}
//	//variables
//	//tarverse for NumberOfReadyProcesses
//	int i = 0;
//	struct Ready_Queue* p = headReadyQ;
//	//tarverse for TimerSuspendedProcessPIDs
//	int j = 0;
//	struct timer_Queue* q = headTimer;
//
//	SP_INPUT_DATA SPData;
//	memset(&SPData, 0, sizeof(SP_INPUT_DATA));
//	strcpy(SPData.TargetAction, targetAction);
//
//	SPData.CurrentlyRunningPID = curtProcessPCB->pcb.process_ID;
//	
//	/*if (strcmp(targetAction, "create") == 0) {
//
//	}*/
//	SPData.TargetPID = targetPID;
//
//	SPData.NumberOfReadyProcesses = lenReadyQ;
//	i = 0;
//	p = headReadyQ;
//	while (p != NULL && i < SPData.NumberOfReadyProcesses) {
//		SPData.ReadyProcessPIDs[i] = p->curtPCB->pcb.process_ID;
//		i = i + 1;
//		p = p->next;
//	}
//
//	SPData.NumberOfTimerSuspendedProcesses = lenTimerQ;
//	j = 0;
//	q = headTimer;
//	while (q != NULL && j < SPData.NumberOfTimerSuspendedProcesses)
//	{
//		SPData.TimerSuspendedProcessPIDs[j] = q->curtPCB->pcb.process_ID;
//		j = j + 1;
//		q = q->next;
//	}
//	CALL(SPPrintLine(&SPData));
//}

void createProcesTest3(char* ProcessName, long StartingAddress, long InitialPriority, long* ProcessID, long* ErrorReturned) {
	MEMORY_MAPPED_IO mmio;
	struct Process_PCB pcb;
	struct PCB_Queue* curtPCB;			//return value after add to pcb queue
	void *PageTable = (void*)calloc(2, NUMBER_VIRTUAL_PAGES);
	pcb.PageTable = PageTable;
	pcb.process_ID = PID + 1;
	PID = PID + 1;
	
	//scheduler printer
	printScheduler(printFullScheduler, "create", PID);


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
	curtPCB = addToPCBQueue(&pcb);
	//add the pcb in pcb queue to readyQ
	addToReadyQueue(curtPCB);
}

