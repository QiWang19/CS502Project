#include				"global.h"

struct FrameMage
{
	INT16 isUsed;
	INT16 pid;
	INT16 pageNumber;
	INT16 state;
};

struct FrameTable 
{
	struct FrameMage frameTable[NUMBER_PHYSICAL_PAGES];
};

UINT16 *GetPageTableAddress();
void invalidMemoryHandler(UINT16 VirtualPageNumber);
int findEmptyFrame(int* emptyFrameIndex);