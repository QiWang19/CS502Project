#include				"global.h"
//#include				"oscreateProcess.h"
//#include				"page.h"
struct FrameMage
{
	INT16 isUsed;
	INT16 pid;
	INT16 pageNumber;
	INT16 state;
	char* areaTag;
};

struct FrameTable
{
	struct FrameMage frameTable[NUMBER_PHYSICAL_PAGES];
};

struct FrameList
{
	int FrameNum;
	struct FrameList* next;
	struct FrameList* prev;
};
void InvalidMemoryHandler(UINT16 VirtualPageNumber);
int GetPageTableState(int FrameNum);

void BuildFrameList();
void ClearFrameReferenceBit(int frameNum);

void InitFrameTable();
void ClearProcessFrame(int pid);


