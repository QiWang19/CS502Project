#include				"syscalls.h"
#include				"page.h"

extern struct FrameTable frameTable;
int initMPData = 0;
void printMemory(MP_INPUT_DATA* MPData) {
	//MP_INPUT_DATA MPData;
	if (!initMPData) {
		memset(MPData, 0, sizeof(MP_INPUT_DATA));
		initMPData = 1;
	}
	int i = 0;
	for (i = 0; i < NUMBER_PHYSICAL_PAGES; i++) {
		MPData->frames[i].InUse = frameTable.frameTable[i].isUsed;
		MPData->frames[i].LogicalPage = frameTable.frameTable[i].pageNumber;
		MPData->frames[i].Pid = frameTable.frameTable[i].pid;
		MPData->frames[i].State = (GetPageTableAddress()[(UINT16)frameTable.frameTable[i].pageNumber] & 0xf000) >> 13;
	}
	MPPrintLine(MPData);
}

void MemoryOutput(MP_INPUT_DATA* MPData) {
	MPPrintLine(MPData);
}
