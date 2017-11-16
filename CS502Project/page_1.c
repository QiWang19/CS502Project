#include				"page_1.h"
#include				"page.h"
#include				"global.h"
#include				"oscreateProcess.h"
#include				"syscalls.h"
#include				"printMemory.h"
#include				"clist.h"
#include				"protos.h"
#include				"file.h"
struct FrameTable FrameTable;
CList* FrameList;
CListElmt *traverseFrameList;
int VictimCount = 19;
MP_INPUT_DATA MPData;
extern struct PCB_Queue* curtProcessPCB;
extern struct PCB_Queue* headPCB;

void InvalidMemoryHandler(UINT16 VirtualPageNumber) {
	int VICTIM_DISK = 1;
	char PageOnDisk[PGSIZE];
	char VictimPageData[PGSIZE];
	struct PCB_Queue* CurtPCB;
	int i = 0;
	int FindEmptyFrame = 0;
	int FreeFrameNum = -1;
	int isModified = 0;
	int isReferenced = 0;
	int PageTableState = 0;
	int VictimPageNum = 0;
	struct PCB_Queue* VictimPCB = NULL;
	int VictimSectorNum = 0;
	int NoFreeFrame = 0;

	if (VirtualPageNumber >= NUMBER_VIRTUAL_PAGES) {
		haltSimulation();
	}
	//printMemory(&MPData);
	FindCurtProcessPCB(&CurtPCB);
	if (CurtPCB->pcb.ShadowPageTable[VirtualPageNumber] != 0) {
		readFromDisk(VICTIM_DISK, CurtPCB->pcb.ShadowPageTable[VirtualPageNumber], PageOnDisk);
	}
	for (i = 0; i < NUMBER_PHYSICAL_PAGES; i++) {
		if (!FrameTable.frameTable[i].isUsed) {
			FindEmptyFrame = 1;
			break;
		}
		//if (i == NUMBER_PHYSICAL_PAGES - 1) {
		//	NoFreeFrame = 1;
		//}
	}
	
	if (FindEmptyFrame) {		//otherwise the FrameTable is full
		FreeFrameNum = i;
	}
	
	//if (NoFreeFrame == 0) {
	//	FreeFrameNum = i;
	//}
	else {						//find free frame using approximate LRU
/*		for (i = 0; i < NUMBER_PHYSICAL_PAGES; i++) {
			isModified = GetPageTableState(i) & PTBL_MODIFIED_BIT;
			if (isModified == 0) {
				FreeFrameNum = i;
				break;
			}
		}								*/
		if (FreeFrameNum == -1) {
			 //can not find unmodified
			while (FreeFrameNum == -1) {
				PageTableState = GetPageTableState(traverseFrameList->data);
				//isReferenced = GetPageTableState(traverseFrameList->data) & PTBL_REFERENCED_BIT;
				isReferenced = PageTableState & PTBL_REFERENCED_BIT;
				if (isReferenced) {
					ClearFrameReferenceBit((int)(traverseFrameList->data));
					traverseFrameList = traverseFrameList->next;
				}
				else {
					FreeFrameNum = traverseFrameList->data;
					traverseFrameList = traverseFrameList->next;
				}
			}
		}

		VictimPageNum = FrameTable.frameTable[FreeFrameNum].pageNumber;
		FindPCBbyPID(FrameTable.frameTable[FreeFrameNum].pid, &VictimPCB);
		if (VictimPCB->pcb.ShadowPageTable[VictimPageNum] == 0) {
			VictimSectorNum = VictimCount;
			VictimCount = VictimCount + 1;
			//VictimSectorNum = VictimPCB->pcb.ShadowPageTable[VictimPageNum];
		}
		else {
			//VictimSectorNum = VictimCount;
			//VictimCount = VictimCount + 1;
			VictimSectorNum = VictimPCB->pcb.ShadowPageTable[VictimPageNum];
		}
		FreeFrameNum = VictimPCB->pcb.PageTable[VictimPageNum] & 0x0FFF;
		Z502ReadPhysicalMemory(FreeFrameNum, VictimPageData);
		writeVictimPageToDisk(VICTIM_DISK, VictimSectorNum, VictimPageData);
		VictimPCB->pcb.ShadowPageTable[VictimPageNum] = VictimSectorNum;
		VictimPCB->pcb.PageTable[VictimPageNum] = 0;
	}
	if (CurtPCB->pcb.ShadowPageTable[VirtualPageNumber] != 0) {
		Z502WritePhysicalMemory(FreeFrameNum, PageOnDisk);
		//readFromDisk(VICTIM_DISK, CurtPCB->pcb.ShadowPageTable[VirtualPageNumber], PageOnDisk);
	}
	FrameTable.frameTable[FreeFrameNum].pageNumber = VirtualPageNumber;
	FrameTable.frameTable[FreeFrameNum].pid = CurtPCB->pcb.process_ID;
	FrameTable.frameTable[FreeFrameNum].isUsed = TRUE;

	CurtPCB->pcb.PageTable[VirtualPageNumber] = FreeFrameNum | PTBL_VALID_BIT;
}

int GetPageTableState(int FrameNum) {
	struct PCB_Queue* PCBofFrame = NULL;
	FindPCBbyPID(FrameTable.frameTable[FrameNum].pid, &PCBofFrame);
	if (PCBofFrame != NULL) {
		return PCBofFrame->pcb.PageTable[FrameTable.frameTable[FrameNum].pageNumber];
	}
	return -1;			//PCB is null
}

int FindPCBbyPID(int pid, struct PCB_Queue** pcb) {
	struct PCB_Queue* p = headPCB;
	while (p != NULL) {
		if (p->pcb.process_ID == pid) {
			*pcb = p;
			break;
		}
		p = p->next;
	}
	return -1;		//did not find the pcb
}

void BuildFrameList() {
	//clist_init(FrameList, clist_destroy(FrameList));
	FrameList = (CList *)malloc(sizeof(CList));
	clist_init(FrameList, clist_destroy(FrameList));
	CListElmt *element = FrameList->head;
	int i = 0;
	for (i = 0; i < NUMBER_PHYSICAL_PAGES; i++) {
		clist_ins_next(FrameList, element, i);
		if (i == 0) {
			element = FrameList->head;
		}
		element = element->next;
	}
	traverseFrameList = FrameList->head;
}

void InitFrameTable() {
	int i = 0;
	for (i = 0; i < NUMBER_PHYSICAL_PAGES; i++) {
		FrameTable.frameTable[i].isUsed = FALSE;
		FrameTable.frameTable[i].pageNumber = -1;
		FrameTable.frameTable[i].pid = -1;
		FrameTable.frameTable[i].state = 0;
	}
	
}

void ClearFrameReferenceBit(int frameNum) {
	int framePID = FrameTable.frameTable[frameNum].pid;
	struct PCB_Queue* framePCB = NULL;
	int res = FindPCBbyID(framePID, &framePCB);
	if (res != -1) {
		framePCB->pcb.PageTable[FrameTable.frameTable[frameNum].pageNumber] &= 0xDFFF;
	}
}

int FindPCBbyID(int targetPID, struct PCB_Queue** PCBofFrame) {
	struct PCB_Queue* p = headPCB;
	while (p != NULL)
	{
		if (p->pcb.process_ID == targetPID) {
			*PCBofFrame = p;
			return 1;
		}
		p = p->next;
	}
	return -1;
}

void ClearProcessFrame(int pid) {
	int i = 0;
	for (i = 0; i < NUMBER_PHYSICAL_PAGES; i++) {
		if (FrameTable.frameTable[i].pid == pid) {
			FrameTable.frameTable[i].pageNumber = -1;
			FrameTable.frameTable[i].isUsed = FALSE;
			FrameTable.frameTable[i].pid = -1;
			FrameTable.frameTable[i].state = 0;
		}
		
	}
}