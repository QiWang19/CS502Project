#include				"page.h"
#include				"global.h"
#include				"oscreateProcess.h"
#include				"syscalls.h"
#include				"printMemory.h"
#include				"protos.h"
#include				<string.h>


static int frameUsed[NUMBER_PHYSICAL_PAGES];
struct FrameTable frameTable;
struct FrameList* headFrameList = NULL;
struct FrameList* rearFrameList = NULL;
struct FrameList* findVictim;
MP_INPUT_DATA MPData;
extern struct PCB_Queue* curtProcessPCB;
extern struct PCB_Queue* headPCB;
extern struct FrameTable FrameTable;
char fakedisk[2048][PGSIZE];
int vicDiskSectorNum = 19;
INT32 NumPrevSharers = 0;

//memset(MPData, 0, sizeof(MP_INPUT_DATA));
void invalidMemoryHandler(UINT16 VirtualPageNumber) {
	//printMemory(&MPData);
	//printf("Fault_handler: Found vector type %d with value %d\n", INVALID_MEMORY,
	//	VirtualPageNumber);
	if (VirtualPageNumber >= NUMBER_VIRTUAL_PAGES) {
		haltSimulation();
	}
	//char* pageOnDisk = (void*)calloc(1, PGSIZE);
	char pageOnDisk[PGSIZE];
	if (curtProcessPCB->pcb.ShadowPageTable[VirtualPageNumber] != 0) {
		readFromDisk(VICTIMDISK, curtProcessPCB->pcb.ShadowPageTable[VirtualPageNumber], pageOnDisk);
		/*int fakei = 0;
		for (fakei = 0; fakei < PGSIZE; fakei++) {
		pageOnDisk[fakei] = fakedisk[curtProcessPCB->pcb.ShadowPageTable[VirtualPageNumber]][fakei];
		}*/
	}
	int emptyFrame = 0;
	int findFrameValid = 0;
	findFrameValid = findEmptyFrame(&emptyFrame);

	if (findFrameValid) {
		curtProcessPCB->pcb.PageTable[(UINT16)VirtualPageNumber] = emptyFrame | PTBL_VALID_BIT;
		//GetPageTableAddress()[(UINT16)VirtualPageNumber] = emptyFrame | PTBL_VALID_BIT;
		frameUsed[emptyFrame] = -1;
		frameTable.frameTable[emptyFrame].isUsed = TRUE;
		frameTable.frameTable[emptyFrame].pageNumber = VirtualPageNumber;
		frameTable.frameTable[emptyFrame].pid = (UINT16)curtProcessPCB->pcb.process_ID;
		frameTable.frameTable[emptyFrame].state = GetPageTableAddress()[(UINT16)VirtualPageNumber];

		//MemoryOutput(&MPData);
		if (curtProcessPCB->pcb.ShadowPageTable[VirtualPageNumber] != 0) {
			Z502WritePhysicalMemory(emptyFrame, pageOnDisk);
			//curtProcessPCB->pcb.ShadowPageTable[VirtualPageNumber] = 0;
		}
		//printMemory( &MPData);
	}
	else {
		//can not find valid empty frame, do page replacement

		PageReplacement(VirtualPageNumber, pageOnDisk);
		//printMemory(&MPData);
	}
	//printMemory(&MPData);
}

int findEmptyFrame(int* emptyFrameIndex) {
	int i = 0;
	for (i = 0; i < NUMBER_PHYSICAL_PAGES; i++) {
		if (frameUsed[i] == 0) {
			*emptyFrameIndex = i;
			return 1;
		}
	}
	return 0;
}

//Create 64 length list for LRU 
void initFrameList() {
	struct FrameList* p = headFrameList;
	int i = 0;
	for (i = 0; i < NUMBER_PHYSICAL_PAGES; i++) {
		InsertFrameListLast(headFrameList, i);
		headFrameList = headFrameList->next;
	}
	headFrameList = headFrameList->next;
	findVictim = headFrameList;
}

int FrameListIsEmpty() {
	return headFrameList == NULL;
}

//Insert item from last position of the list
void InsertFrameListLast(struct FrameList* element, int frameNum) {
	struct FrameList* link = (struct FrameList*)malloc(sizeof(struct FrameList));
	link->FrameNum = frameNum;
	if (FrameListIsEmpty()) {
		link->next = link;
		headFrameList = link;
		//element = link;
	}
	else {
		link->next = element->next;
		element->next = link;
	}
}

void MoveToFirstFrameList(int frameToMove) {
	if (headFrameList->FrameNum == frameToMove) {
		return;
	}
	struct FrameList* p = headFrameList;
	while (p != NULL) {
		if (p->FrameNum == frameToMove) {
			break;
		}
	}
	if (p->next != NULL) {
		p->prev->next = p->next;
		p->next->prev = p->prev;
		p->prev = NULL;
		p->next = headFrameList;
		headFrameList->prev = p;
		headFrameList = p;
	}
	else if (p->next == NULL) {
		struct FrameList* q = headFrameList;
		headFrameList = rearFrameList;
		rearFrameList = rearFrameList->prev;
		rearFrameList->next = NULL;
		headFrameList->prev = NULL;
		headFrameList->next = q;
		q->prev = headFrameList;
	}

}

void PageReplacement(UINT16 VirtualPageNumber, char* pageOnDisk) {
	//struct FrameList* findVictim = headFrameList;
	int victimFrameNum = selectVictimPage(&findVictim);
	//p points to the freed frame
	//Set shadow page table about victim page
	//printf("victimFrame number %d\n", victimFrameNum);
	//printf("pointer p %d\n", findVictim->FrameNum);
	DiskSecForShadowPageTable(VirtualPageNumber, findVictim);
	findVictim = findVictim->next;
	//give freed frame to new page
	if (curtProcessPCB->pcb.ShadowPageTable[VirtualPageNumber] != 0) {
		Z502WritePhysicalMemory(victimFrameNum, pageOnDisk);
		//curtProcessPCB->pcb.ShadowPageTable[VirtualPageNumber] = 0;
	}

}
//Return the frame num of the victim page
int selectVictimPage(struct FrameList** current) {
	while (getFrameReference(*current) == 1) {
		setFrameRefToZero(*current);
		*current = (*current)->next;
	}
	return (*current)->FrameNum;
}

int getFrameReference(struct FrameList* current) {
	int isRefer = 0;
	struct PCB_Queue* PCBofFrame = NULL;
	findPCBofFrame(current, &PCBofFrame);
	if (PCBofFrame != NULL) {
		isRefer = (PCBofFrame->pcb.PageTable[(UINT16)frameTable.frameTable[current->FrameNum].pageNumber]) & PTBL_REFERENCED_BIT;
		isRefer = isRefer >> 13;
		return isRefer;
	}
	printf("Error !\n");
	return -1;
}

void findPCBofFrame(struct FrameList* current, struct PCB_Queue** PCBofFrame) {
	struct PCB_Queue* p = headPCB;
	while (p != NULL)
	{
		if (p->pcb.process_ID == frameTable.frameTable[current->FrameNum].pid) {
			*PCBofFrame = p;
			break;
		}
		p = p->next;
	}
}

void findPCBbyID(int targetPID, struct PCB_Queue** PCBofFrame) {
	struct PCB_Queue* p = headPCB;
	while (p != NULL)
	{
		if (p->pcb.process_ID == targetPID) {
			*PCBofFrame = p;
			break;
		}
		p = p->next;
	}
}

void setFrameRefToZero(struct FrameList* current) {
	struct PCB_Queue* PCBofFrame = NULL;
	findPCBofFrame(current, &PCBofFrame);
	(PCBofFrame->pcb.PageTable[(UINT16)frameTable.frameTable[current->FrameNum].pageNumber]) &= 0xDFFF;
}

//Select sector to write victim page and put it to disk
//current points to the frame of victim page; 
int DiskSecForShadowPageTable(UINT16 VirtualPageNumber, struct FrameList* current) {
	//find sector num to put page data
	struct PCB_Queue* PCBofFrame = NULL;
	findPCBofFrame(current, &PCBofFrame);
	//TODO:
	//shadowpage record the disk sector num
	//int VictimSector = current->FrameNum + 1;
	int VictimSector = 0;
	int victim_page_num = (UINT16)frameTable.frameTable[current->FrameNum].pageNumber;
	if (PCBofFrame->pcb.ShadowPageTable[victim_page_num] == 0) {
		VictimSector = vicDiskSectorNum;
		vicDiskSectorNum = vicDiskSectorNum + 1;
		PCBofFrame->pcb.ShadowPageTable[victim_page_num] = VictimSector;
	}
	else if (PCBofFrame->pcb.ShadowPageTable[victim_page_num] != 0) {
		VictimSector = PCBofFrame->pcb.ShadowPageTable[victim_page_num];
	}


	//write data to disk
	//char* VictimPageData = (void*)calloc(1, PGSIZE);
	char VictimPageData[PGSIZE];
	INT32 physicalFrameNum = (PCBofFrame->pcb.PageTable[victim_page_num]) & 0x0FFF;
	Z502ReadPhysicalMemory(physicalFrameNum, VictimPageData);//get victim frame data
	writeVictimPageToDisk(VICTIMDISK, VictimSector, VictimPageData);
	/*int fakei = 0;
	for (fakei = 0; fakei < PGSIZE; fakei++) {
	fakedisk[VictimSector][fakei] = VictimPageData[fakei];
	}*/
	PCBofFrame->pcb.PageTable[victim_page_num] = 0;	//make victim pagetable entry invalid
													//Update curt process pagetable
													//give freed frame to new page
													//GetPageTableAddress()[(UINT16)VirtualPageNumber] = physicalFrameNum | PTBL_VALID_BIT;
	curtProcessPCB->pcb.PageTable[(UINT16)VirtualPageNumber] = physicalFrameNum | PTBL_VALID_BIT;
	//Update global frameTable
	frameUsed[physicalFrameNum] = -1;
	frameTable.frameTable[physicalFrameNum].isUsed = TRUE;
	frameTable.frameTable[physicalFrameNum].pageNumber = VirtualPageNumber;
	frameTable.frameTable[physicalFrameNum].pid = (UINT16)curtProcessPCB->pcb.process_ID;
	frameTable.frameTable[physicalFrameNum].state = curtProcessPCB->pcb.PageTable[(UINT16)VirtualPageNumber];


	return VictimSector;
}

//Release the physical mem of the process, called when terminate a process
void ClearProcessPhysicalMem(int pid) {
	int index = 0;
	for (index = 0; index < NUMBER_PHYSICAL_PAGES; index++) {
		if (frameTable.frameTable[index].pid == pid) {
			frameTable.frameTable[index].isUsed = FALSE;
			frameTable.frameTable[index].pageNumber = -1;
			frameTable.frameTable[index].pid = -1;
			frameTable.frameTable[index].state = 0;
			frameUsed[index] = 0;
			break;
		}
	}

}

void DefineSharedArea(INT32 StartingAddress, INT32 PagesInSharedArea, char* AreaTag,
	INT32* NumberPreviousSharers, INT32* ErrorReturned) {
	int i = 0;
	INT32 SharePageID = StartingAddress / PGSIZE;
	int ShareFrameID = 1;
	for (i = 0; i < PagesInSharedArea; i++) {
		curtProcessPCB->pcb.PageTable[SharePageID + i] = ShareFrameID | PTBL_VALID_BIT;
		FrameTable.frameTable[ShareFrameID].isUsed = TRUE;
		FrameTable.frameTable[ShareFrameID].areaTag = AreaTag;
		ShareFrameID = ShareFrameID + 1;
	}
	*NumberPreviousSharers = NumPrevSharers;
	NumPrevSharers = NumPrevSharers + 1;
	*ErrorReturned = ERR_SUCCESS;
}

void SendMessage(INT32 ProcessID, char* MessageBuffer, INT32 MessageSendLength, INT32* ErrorReturned) {
	struct PCB_Queue* FoundPCB;
	findPCBbyID(ProcessID, &FoundPCB);
	strcpy(FoundPCB->pcb.Message, MessageBuffer);
	FoundPCB->pcb.MessageSendLength = MessageSendLength;
	FoundPCB->pcb.MessageSendPid = curtProcessPCB->pcb.process_ID;
	*ErrorReturned = ERR_SUCCESS;
}

void ReceiveMessage(INT32 SourceID, char* MessageBuffer, INT32 MessageReceiveLength, INT32* MessageSendLength,
	INT32* MessageSenderPid, INT32* ErrorReturned) {
	struct PCB_Queue* curtPCB = curtProcessPCB;
	int flag = 1;
	while (flag) {
		if (curtProcessPCB->pcb.MessageSendLength > 0) {
			flag = 0;
		}
		if (curtProcessPCB->pcb.MessageSendLength <= 0) {
			addToReadyQueue(curtPCB);
			dispatcher();
		}
	}
	
	curtPCB = curtProcessPCB;
	*MessageSendLength = curtProcessPCB->pcb.MessageSendLength;
	*MessageSenderPid = curtProcessPCB->pcb.MessageSendPid;
	*ErrorReturned = ERR_SUCCESS;
	addToReadyQueue(curtPCB);
	dispatcher();
}
