//Define the structure of the disk queue
struct Disk_Queue{
	struct PCB_Queue* curtPCB;
	long DiskID;
	struct Disk_Queue* next;
};

int addToDiskQueue();
struct PCB_Queue* delFromDiskQueue();
void updateDiskQueue(int DiskID);