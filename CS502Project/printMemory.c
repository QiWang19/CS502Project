#include				"syscalls.h"
#include				"page.h"
#include				"oscreateProcess.h"
#include				"protos.h"

//extern struct FrameTable frameTable;
extern struct FrameTable FrameTable;
int initMPData = 0;
void printMemory(MP_INPUT_DATA* MPData) {
	//MP_INPUT_DATA MPData;
	if (!initMPData) {
		memset(MPData, 0, sizeof(MP_INPUT_DATA));
		initMPData = 1;
	}
	int i = 0;
	struct PCB_Queue* pcbOfFrame = NULL;
	for (i = 0; i < NUMBER_PHYSICAL_PAGES; i++) {
		MPData->frames[i].InUse = FrameTable.frameTable[i].isUsed;
		MPData->frames[i].LogicalPage = FrameTable.frameTable[i].pageNumber;
		MPData->frames[i].Pid = FrameTable.frameTable[i].pid;
		findPCBbyID(FrameTable.frameTable[i].pid, &pcbOfFrame);
		if (pcbOfFrame != NULL) {
			MPData->frames[i].State = (pcbOfFrame->pcb.PageTable[FrameTable.frameTable[i].pageNumber] & 0xf000) >> 13;
		}
		
	}
	MPPrintLine(MPData);
}

void MemoryOutput(MP_INPUT_DATA* MPData) {
	MPPrintLine(MPData);
}
