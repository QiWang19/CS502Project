
struct Disk_Queue{
	struct PCB_Queue* curtPCB;
	long DiskID;
	struct Disk_Queue* next;
};

void addToDiskQueue();
struct PCB_Queue* delFromDiskQueue();
void updateDiskQueue();