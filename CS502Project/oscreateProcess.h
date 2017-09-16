struct Process_PCB
{
	long process_ID;
	long NewContext;
	void *PageTable;
	char* process_Name ;
	long processPriority;
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

void os_create_process(char* ProcessName, long StartingAddress, long InitialPriority, long* ProcessID, long* ErrorReturned);
//TODO: add according to the endTime comparision
void addToTimerQueue();
void createProcesTest3(char* ProcessName, long StartingAddress, long InitialPriority, long* ProcessID, long* ErrorReturned);
void delFromTimerQueue();
//TODO: reayQ
