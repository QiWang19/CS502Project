#include				"global.h"

#define					VICTIMDISK				1
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

UINT16 *GetPageTableAddress();
void invalidMemoryHandler(UINT16 VirtualPageNumber);
int findEmptyFrame(int* emptyFrameIndex);
void initFrameList();
int FrameListIsEmpty();
void InsertFrameListLast(struct FrameList *element, int frameNum);
void PageReplacement(UINT16 VirtualPageNumber, char* pageOnDisk);
int selectVictimPage(struct FrameList** current);
int getFrameReference(struct FrameList* current);
void findPCBofFrame(struct FrameList* current, struct PCB_Queue** PCBofFrame);
void setFrameRefToZero(struct FrameList* current);
int DiskSecForShadowPageTable(UINT16 VirtualPageNumber, struct FrameList* current);
void ClearProcessPhysicalMem(int pid);
void findPCBbyID(int targetPID, struct PCB_Queue** PCBofFrame);
void MoveToFirstFrameList(int frameToMove);
void DefineSharedArea(INT32 StartingAddress, INT32 PagesInSharedArea, char* AreaTag,
	INT32* NumberPreviousSharers, INT32* ErrorReturned);
void SendMessage(INT32 ProcessID, char* MessageBuffer, INT32 MessageSendLength, INT32* ErrorReturned);
void ReceiveMessage(INT32 SourceID, char* MessageBuffer, INT32 MessageReceiveLength, INT32* MessageSendLength,
	INT32* MessageSenderPid, INT32* ErrorReturned);