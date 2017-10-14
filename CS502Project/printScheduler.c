#include			"oscreateProcess.h"
#include			"disk.h"
#include			 "lock.h"
#include			 "stdio.h"
#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"
#include             <stdlib.h>
#include             <ctype.h>

extern struct timer_Queue* headTimer;
extern struct Ready_Queue* headReadyQ;
extern struct Disk_Queue* headDisk;
extern struct PCB_Queue* curtProcessPCB;

//timer_Queue length
extern long lenTimerQ;
//readyQ length
extern long lenReadyQ;
extern long lenDiskQ;

//scheduler print
int printFullScheduler = 0;

//For the state printer of scheduler, called by dispatch and set data
void printScheduler(int printFullScheduler, char* targetAction, long targetPID)
{
	if (printFullScheduler == 0) {
		return;
	}
	//variables
	//tarverse for NumberOfReadyProcesses
	int i = 0;
	struct Ready_Queue* p = headReadyQ;
	//tarverse for TimerSuspendedProcessPIDs
	int j = 0;
	struct timer_Queue* q = headTimer;
	//traverse for NumberOfDiskSuspendedProcesses
	int k = 0;
	struct Disk_Queue* r = headDisk;

	SP_INPUT_DATA SPData;
	memset(&SPData, 0, sizeof(SP_INPUT_DATA));
	strcpy(SPData.TargetAction, targetAction);

	SPData.CurrentlyRunningPID = curtProcessPCB->pcb.process_ID;

	/*if (strcmp(targetAction, "create") == 0) {

	}*/
	SPData.TargetPID = targetPID;

	SPData.NumberOfReadyProcesses = lenReadyQ;
	i = 0;
	p = headReadyQ;
	while (p != NULL && i < SPData.NumberOfReadyProcesses) {
		SPData.ReadyProcessPIDs[i] = p->curtPCB->pcb.process_ID;
		i = i + 1;
		p = p->next;
	}

	SPData.NumberOfTimerSuspendedProcesses = lenTimerQ;
	j = 0;
	q = headTimer;
	while (q != NULL && j < SPData.NumberOfTimerSuspendedProcesses)
	{
		SPData.TimerSuspendedProcessPIDs[j] = q->curtPCB->pcb.process_ID;
		j = j + 1;
		q = q->next;
	}

	SPData.NumberOfDiskSuspendedProcesses = lenDiskQ;
	k = 0;
	r = headDisk;
	while (r != NULL && k < SPData.NumberOfDiskSuspendedProcesses) {
		SPData.DiskSuspendedProcessPIDs[k] = r->curtPCB->pcb.process_ID;
		k = k + 1;
		r = r->next;
	}
	
	CALL(SPPrintLine(&SPData));
}