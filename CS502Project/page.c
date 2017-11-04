#include				"page.h"
#include				"global.h"
#include				"oscreateProcess.h"
#include				"syscalls.h"
#include				"printMemory.h"

static int frameUsed[NUMBER_PHYSICAL_PAGES];
struct FrameTable frameTable;
MP_INPUT_DATA MPData;
extern struct PCB_Queue* curtProcessPCB;

//memset(MPData, 0, sizeof(MP_INPUT_DATA));
void invalidMemoryHandler(UINT16 VirtualPageNumber) {
	if (VirtualPageNumber >= NUMBER_VIRTUAL_PAGES) {
		haltSimulation();
	}
	int emptyFrame = 0;
	int findFrameValid = 0;
	findFrameValid = findEmptyFrame(&emptyFrame);
	
	if (findFrameValid) {
		
		GetPageTableAddress()[(UINT16)VirtualPageNumber] = emptyFrame | PTBL_VALID_BIT;
		frameUsed[emptyFrame] = -1;
		frameTable.frameTable[emptyFrame].isUsed = TRUE;
		frameTable.frameTable[emptyFrame].pageNumber = VirtualPageNumber;
		frameTable.frameTable[emptyFrame].pid = curtProcessPCB->pcb.process_ID;
		frameTable.frameTable[emptyFrame].state = GetPageTableAddress()[(UINT16)VirtualPageNumber];
		printMemory( &MPData);
		//MemoryOutput(&MPData);
	}
	else {
		//can not find valid empty frame, do page replacement

	}
	
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