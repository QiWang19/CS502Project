#include				"global.h"
void InvalidMemoryHandler(UINT16 VirtualPageNumber);
int GetPageTableState(int FrameNum);
int FindPCBbyPID(int pid, struct PCB_Queue** pcb);
void BuildFrameList();
void ClearFrameReferenceBit(int frameNum);
int FindPCBbyID(int targetPID, struct PCB_Queue** PCBofFrame);
void InitFrameTable();
void ClearProcessFrame(int pid);

