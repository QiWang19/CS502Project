#include				"file.h"
#include				"global.h"
#include				"syscalls.h"
#include				"disk.h"
#include				"oscreateProcess.h"

extern struct PCB_Queue* curtProcessPCB;
unsigned char diskBitMap[MAX_NUMBER_OF_DISKS][256];		//(4 * Bitmapsize) sectors * 16byte, totally 2048 bits, 2048 sectors
union diskHeaderData diskHeader[MAX_NUMBER_OF_DISKS];	//the root dir of each disk
long curtDiskID = 0;
long Inode = 1;

void formatDisk(long DiskID, long* ErrorReturned) {
	if (DiskID >= MAX_NUMBER_OF_DISKS) {
		*ErrorReturned = ERR_BAD_PARAM;
		return;
	}
	initSector0(DiskID, ErrorReturned);
	*ErrorReturned = ERR_SUCCESS;
}

void initSector0(long DiskID, long* result) {
	union diskSector0Date disk_sector0_data;
	disk_sector0_data.sector0_data.DiskID = DiskID;
	disk_sector0_data.sector0_data.BitmapSize = BITMAPSIZE;
	disk_sector0_data.sector0_data.RootDirSize = ROOTDIRHEADER;
	disk_sector0_data.sector0_data.SwapSize = SWAPSPACE;
	disk_sector0_data.sector0_data.DiskLength = NUMBER_LOGICAL_SECTORS;			//defined in global.h
	disk_sector0_data.sector0_data.BitmapLocation = BITMAPLOCATION;
	disk_sector0_data.sector0_data.RootDirLocation = ROOTDIRLOCATION;
	disk_sector0_data.sector0_data.SwapLocation = SWAPLOCATION;
	memset(disk_sector0_data.sector0_data.RESERVED, '\0', 4);
	
	initBitMap(DiskID, BITMAPLOCATION, BITMAPSIZE * 4);
	writeBitmapToDisk(DiskID);
	writeSector0ToDisk(DiskID, 0, &disk_sector0_data);

	initRootDir(DiskID, ROOTDIRLOCATION, ROOTDIRHEADER);
}

void initBitMap(long DiskID, short bitMapLocation, short bitMapSize) {
	int sectorStartIndex = bitMapLocation;
	int sectorEndIndex = bitMapLocation + bitMapSize;
	int index = sectorStartIndex;
	int secIndex, offset;
	//set the bitmap to 0
	int i = 0;
	for (i = 0; i < MAX_NUMBER_OF_DISKS; i++) {
		memset(diskBitMap[i], 0, sizeof(char) * 256);		//256 = NUMBER_LOGICAL_SECTORS / 8
	}
	for (index = sectorStartIndex; index < sectorEndIndex; index++) {
		secIndex = index / 8;
		offset = 7 - index % 8;
		diskBitMap[DiskID][secIndex] = diskBitMap[DiskID][secIndex] | (1 << offset);
	}
}

//write the content of bitmap to the bitmap sectors in the disk
void writeBitmapToDisk(long DiskID) {
	int bitmapSectors = 16;			//16 sectors for bitmap
	int i = 0;
	int k = 0;
	int ONESECTOR = 16;
	for (i = BITMAPLOCATION; i < BITMAPLOCATION + bitmapSectors; i++) {
		writeToDisk(DiskID, i, (char*)diskBitMap[DiskID] + k * ONESECTOR);
		k = k + 1;
	}
}

//write the buffer to the sector in the disk
void writeToDisk(long DiskID, int sectorToWrite, char* writtenBuffer) {
	MEMORY_MAPPED_IO mmio;
	mmio.Mode = Z502DiskWrite;
	mmio.Field1 = (long)DiskID;
	mmio.Field2 = (long)sectorToWrite;
	mmio.Field3 = (long)writtenBuffer;
	mmio.Field4 = 0;
	MEM_WRITE(Z502Disk, &mmio);
	addToDiskQueue();
	dispatcher();
}

void writeSector0ToDisk(long DiskID, int sectorToWrite, union diskSector0Date* disk_sector0_data) {
	writeToDisk(DiskID, sectorToWrite, disk_sector0_data->char_data);
	updateBitMap(DiskID, sectorToWrite);
}

//TODO: Update the bitmap after change some sector in a disk
//change bitmap and write it to disk;
void updateBitMap(long DiskID, int sectorToWrite) {
	int secIndex = sectorToWrite / 8;
	int offset = 7 - sectorToWrite % 8;
	int i = 0;
	diskBitMap[DiskID][secIndex] = diskBitMap[DiskID][secIndex] | (1 << offset);
	writeBitmapToDisk(DiskID);
}

void initRootDir(long DiskID, short rootDirLocation, short rootHeaderSize) {
	union indexSectorData index_sector_data;
	union diskHeaderData disk_header_data;
	disk_header_data.diskHeader_data.Inode = 0;
	strcpy(disk_header_data.diskHeader_data.Name, "root");
	disk_header_data.diskHeader_data.FileDescription = (31 << 3) | 3;		//fb

	//get current time
	MEMORY_MAPPED_IO mmio;
	mmio.Mode = Z502ReturnValue;
	mmio.Field1 = mmio.Field2 = mmio.Field3 = mmio.Field4 = 0;
	MEM_READ(Z502Clock, &mmio);
	memcpy(disk_header_data.diskHeader_data.CreationTime, &mmio.Field1, 3);

	disk_header_data.diskHeader_data.IndexLocation = ROOTINDEXLOCATION;
	disk_header_data.diskHeader_data.FileSize = 0;

	writeRootDirToDisk(DiskID, ROOTDIRLOCATION, &disk_header_data);
	memset(&index_sector_data, 0, sizeof(union indexSectorData));

	writeIndexSectorToDisk(DiskID, ROOTINDEXLOCATION, &index_sector_data);

	diskHeader[DiskID] = disk_header_data;
	curtProcessPCB->pcb.curtDir = disk_header_data.diskHeader_data;
}

void writeRootDirToDisk(long DiskID, short rootDirLocation, union diskHeaderData* disk_header_data) {			// 1 sector
	writeToDisk(DiskID, rootDirLocation, disk_header_data->char_data);
	updateBitMap(DiskID, rootDirLocation);
}

void writeIndexSectorToDisk(long DiskID, short rootIndexLocation, union indexSectorData* index_sector_data) {
	writeToDisk(DiskID, rootIndexLocation, index_sector_data->char_data);
	updateBitMap(DiskID, rootIndexLocation);
}

void openDirectory(long DiskID, char* dirName, long* ErrorReturned) {
	if (DiskID < 0 && DiskID >= MAX_NUMBER_OF_DISKS) {
		*ErrorReturned = ERR_BAD_PARAM;
		return;
	}

	if (strcmp(dirName, "root") == 0) {			//open the root dir
		curtDiskID = DiskID;
		curtProcessPCB->pcb.curtDir = diskHeader[DiskID].diskHeader_data;
		*ErrorReturned = ERR_SUCCESS;
	}

}

void createDirectory(char* newDirName, long* ErrorReturned) {
	short curtDirIndexLocation = curtProcessPCB->pcb.curtDir.IndexLocation;
	union indexSectorData index_sector_data;
	getIndexSectorData(curtDiskID, curtDirIndexLocation, &index_sector_data);
	short emptySector = findEmptySector(curtDiskID, FREESECTORLOCATION);
	if (emptySector == -1) {
		*ErrorReturned = ERR_BAD_PARAM;
		return;
	}
	int i = 0;
	for (i = 0; i < 8; i++) {
		if (index_sector_data.index_sector_data[i] | 0 == 0) {
			index_sector_data.index_sector_data[i] = emptySector;
			break;
		}
	}
	//set Dir header
	union diskHeaderData fileHarderData;
	fileHarderData.diskHeader_data.Inode = Inode;
	Inode = Inode + 1;
	memset(fileHarderData.diskHeader_data.Name, 0, 7);
	strcpy(fileHarderData.diskHeader_data.Name, newDirName);

	unsigned char fileDiscription = 0;
	fileDiscription = fileDiscription | (curtProcessPCB->pcb.curtDir.Inode << 3 | 3);
	fileHarderData.diskHeader_data.FileDescription = fileDiscription;
	//get curt time
	MEMORY_MAPPED_IO mmio;
	mmio.Mode = Z502ReturnValue;
	mmio.Field1 = mmio.Field2 = mmio.Field3 = mmio.Field4 = 0;
	MEM_READ(Z502Clock, &mmio);
	memcpy(fileHarderData.diskHeader_data.CreationTime, &mmio.Field1, 3);
	//find sector for the index sec for the new dir
	short emptyForIndexSec = findEmptySector(curtDiskID, emptySector + 1);
	fileHarderData.diskHeader_data.IndexLocation = emptyForIndexSec;
	fileHarderData.diskHeader_data.FileSize = 0;

	union indexSectorData newIndexSector;
	memset(&newIndexSector, 0, sizeof(union indexSectorData));
	//update bitmap, emptySector for the new dir
	updateBitMap(curtDiskID, emptySector);
	//update index sector
	writeIndexSectorToDisk(curtDiskID, curtDirIndexLocation, index_sector_data.char_data);
	//weitr new dir to disk
	writeRootDirToDisk(curtDiskID, emptySector, fileHarderData.char_data);
	//write new index sector to disk
	writeIndexSectorToDisk(curtDiskID, emptyForIndexSec, newIndexSector.char_data);
	*ErrorReturned = ERR_SUCCESS;
}

//inedxsector size only 1 sector
void getIndexSectorData(long curtDiskID, short indexLocation, union indexSectorData* indexSecData) {
	readFromDisk(curtDiskID, indexLocation, indexSecData->char_data);
}

void readFromDisk(long DiskID, short sectorToRead, char* readBuffer) {
	MEMORY_MAPPED_IO mmio;
	MEMORY_MAPPED_IO mmio1;
	/*mmio.Mode = Z502DiskRead;
	mmio.Field1 = (long)DiskID;
	mmio.Field2 = (long)sectorToRead;
	mmio.Field3 = (long)readBuffer;
	mmio.Field4 = 0;
	addToDiskQueue();
	MEM_WRITE(Z502DiskRead, &mmio);*/

	//Make sure the disk is free
	mmio1.Mode = Z502Status;
	mmio1.Field1 = (long)DiskID;
	mmio1.Field2 = mmio1.Field3 = 0;
	MEM_READ(Z502Disk, &mmio1);
	if (mmio1.Field2 == DEVICE_IN_USE) {
		while (mmio1.Field2 != DEVICE_FREE)
		{
			mmio1.Mode = Z502Status;
			mmio1.Field1 = (long)DiskID;
			mmio1.Field2 = mmio1.Field3 = 0;
			MEM_READ(Z502Disk, &mmio1);
		}
	}
	mmio.Mode = Z502DiskRead;
	mmio.Field1 = (long)DiskID;
	mmio.Field2 = (long)sectorToRead;
	mmio.Field3 = (char*)readBuffer;
	mmio.Field4 = 0;
	MEM_WRITE(Z502Disk, &mmio);
	addToDiskQueue();
	//For idle
	/*mmio1.Mode = Z502Action;
	mmio1.Field1 = mmio1.Field2 = mmio1.Field3 = mmio1.Field4 = 0;
	MEM_WRITE(Z502Idle, &mmio1);*/
	dispatcher();
	
	
}

//find one empty sector to use
short findEmptySector(long DiskID, short startSectortoFind) {
	short freeSectorNum;
	int i = startSectortoFind;
	int sectorIndex = 0;
	int offset = 0;

	for (i = startSectortoFind; i <= NUMBER_LOGICAL_SECTORS; i++) {
		sectorIndex = i / 8;
		offset = 7 - i % 8;
		if ((diskBitMap[DiskID][sectorIndex] & (1 << offset)) == 0) {
			freeSectorNum = i;
			return freeSectorNum;
		}
	}	
	return -1;
}

void createFile()