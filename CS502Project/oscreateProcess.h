struct Process_PCB
{
	long process_ID;
	long NewContext;
	void *PageTable;
};

struct PCB_Queue
{
	struct Process_PCB pcb;
	struct PCB_Queue* next;
};

struct timer_Queue {
	struct PCB_Queue* curtPCB;
	struct timer_Queue* next;
};
void os_create_process(int argc, char *argv[]);