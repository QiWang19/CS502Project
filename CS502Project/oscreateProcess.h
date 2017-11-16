#include				"file.h"
#include				"global.h"
struct Process_PCB
{
	long process_ID;
	long NewContext;
	UINT16 *PageTable;
	char* process_Name ;
	long processPriority;
	struct diskHeader curtDir;
	UINT16 *ShadowPageTable;
};

struct PCB_Queue
{
	struct Process_PCB pcb;
	struct PCB_Queue* next;
};

struct timer_Queue {
	struct PCB_Queue* curtPCB;
	long timerEndTime;
	struct timer_Queue* next;
};

struct Ready_Queue {
	long Qlength;
	struct PCB_Queue* curtPCB;
	struct Ready_Queue* next;
};
struct PCB_Queue* addToPCBQueue(struct Process_PCB* pcb);
void os_create_process(char* ProcessName, long StartingAddress, long InitialPriority, long* ProcessID, long* ErrorReturned);
//TODO: add according to the endTime comparision
void addToTimerQueue(long sleepTime);
void createProcesTest3(char* ProcessName, long StartingAddress, long InitialPriority, long* ProcessID, long* ErrorReturned);
struct PCB_Queue* delFromTimerQueue();
//TODO: reayQ
void addToReadyQueue(struct PCB_Queue* curtPCB);
void delFromReadyQueue();
//dispatcher
void dispatcher();
//decide the timer in timerQ
void updateTimerQueue();
//terminate sys call
void endProcess(int type, long* ErrorReturned);
//scheduler printer 
void printScheduler(int printFullScheduler, char* targetAction, long targetPID);
void haltSimulation();
void FindCurtProcessPCB(struct PCB_Queue** runningProcess);