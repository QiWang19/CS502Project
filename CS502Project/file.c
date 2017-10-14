#include				"file.h"
#include				"global.h"
#include				"syscalls.h"
#include				"disk.h"
#include				"oscreateProcess.h"


#define                  DO_LOCK                     1
#define                  DO_UNLOCK                   0
#define                  SUSPEND_UNTIL_LOCKED        TRUE
#define                  DO_NOT_SUSPEND              FALSE


extern struct PCB_Queue* curtProcessPCB;
unsigned char diskBitMap[MAX_NUMBER_OF_DISKS][256];		//(4 * Bitmapsize) sectors * 16byte, totally 2048 bits, 2048 sectors
union diskHeaderData diskHeader[MAX_NUMBER_OF_DISKS];	//the root dir of each disk
long curtDiskID = 0;									//defined when create directory
long Inode = 1;											//global variable

//For the format system call, called by osinit
void formatDisk(long DiskID, long* ErrorReturned) {
	if (DiskID >= MAX_NUMBER_OF_DISKS) {
		*ErrorReturned = ERR_BAD_PARAM;
		return;
	}
	initSector0(DiskID, ErrorReturned);
	*ErrorReturned = ERR_SUCCESS;
}

//Give the value to sector0, write sector0, bitmap, root dir, to disk
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
	initSwapSectors(DiskID, SWAPLOCATION, SWAPSPACE * 4);
	writeSector0ToDisk(DiskID, 0, &disk_sector0_data);

	initRootDir(DiskID, ROOTDIRLOCATION, ROOTDIRHEADER);
}

//set the bits of sectors of bitmap in bitmap to 1
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

//set bits of swap sectors in bitmap to 1 and update the disk
void initSwapSectors(long DiskID, short swapLocation, short swapSize) {
	setSwapBitMap(DiskID, swapLocation, swapSize);
	writeBitmapToDisk(DiskID);
}

//set sectors in bitmap for swap to 1
void setSwapBitMap(long DiskID, short swapLocation, short swapSize) {
	int sectorStartIndex = swapLocation;
	int sectorEndIndex = swapLocation + swapSize;
	int index = sectorStartIndex;
	int secIndex, offset;
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
	MEMORY_MAPPED_IO mmio1;

	INT32 LockResult;
	//READ_MODIFY(MEMORY_INTERLOCK_BASE + 2 + DiskID, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
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

	mmio.Mode = Z502DiskWrite;
	mmio.Field1 = (long)DiskID;
	mmio.Field2 = (long)sectorToWrite;
	mmio.Field3 = (long)writtenBuffer;
	mmio.Field4 = 0;
	
	//if (res == 1) {
		MEM_WRITE(Z502Disk, &mmio);
	//}
		int res = addToDiskQueue();
	
	//READ_MODIFY(MEMORY_INTERLOCK_BASE + 2 + DiskID, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
	
	dispatcher();
}

//put sector0 data to disk and update the bitmap for disk
void writeSector0ToDisk(long DiskID, int sectorToWrite, union diskSector0Date* disk_sector0_data) {
	writeToDisk(DiskID, sectorToWrite, disk_sector0_data->char_data);
	updateBitMap(DiskID, sectorToWrite);
}

//Update the bitmap after change some sector in a disk
//change bitmap and write it to disk;
void updateBitMap(long DiskID, int sectorToWrite) {
	int secIndex = sectorToWrite / 8;
	int offset = 7 - sectorToWrite % 8;
	int i = 0;
	diskBitMap[DiskID][secIndex] = diskBitMap[DiskID][secIndex] | (1 << offset);
	writeBitmapToDisk(DiskID);
}

//give initial value to header structure
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

//write header data to some sector
void writeRootDirToDisk(long DiskID, short rootDirLocation, union diskHeaderData* disk_header_data) {			// 1 sector
	writeToDisk(DiskID, rootDirLocation, disk_header_data->char_data);
	updateBitMap(DiskID, rootDirLocation);
}

//write the sector of index of header structure to disk
void writeIndexSectorToDisk(long DiskID, short rootIndexLocation, union indexSectorData* index_sector_data) {
	writeToDisk(DiskID, rootIndexLocation, index_sector_data->char_data);
	updateBitMap(DiskID, rootIndexLocation);
}

//For open dir system call. Get the dir has the dirname, set it as curt dir for the process
void openDirectory(long DiskID, char* dirName, long* ErrorReturned) {
	if (DiskID < -1 || DiskID >= MAX_NUMBER_OF_DISKS) {
		*ErrorReturned = ERR_BAD_PARAM;
		return;
	}
	if (DiskID != -1) {
		curtDiskID = DiskID;
	}

	if (strcmp(dirName, "root") == 0) {			//open the root dir		
		//curtDiskID = DiskID;
		curtProcessPCB->pcb.curtDir = diskHeader[curtDiskID].diskHeader_data;
		*ErrorReturned = ERR_SUCCESS;
		return;
	}
	if (strcmp(curtProcessPCB->pcb.curtDir.Name, dirName) == 0) {
		*ErrorReturned = ERR_SUCCESS;
		return;
	}
	unsigned char curtFileDescription = curtProcessPCB->pcb.curtDir.FileDescription;
	int curtIndexBit = curtFileDescription & 6;
	//printf("curtIndexBit========================= %d\n", curtIndexBit);
	union indexSectorData curtIndexSectorData;
	short curtDirIndexLocation = curtProcessPCB->pcb.curtDir.IndexLocation;
	int i = 0;
	short findSectorNum = 0;
	union diskHeaderData findHeaderData;
	unsigned char findFileDescription;
	if (curtIndexBit != 0) {
		getIndexSectorData(curtDiskID, curtDirIndexLocation, &curtIndexSectorData);
		for (i = 0; i < 8; i++) {
			if ((curtIndexSectorData.index_sector_data[i] | 0) != 0) {
				findSectorNum = curtIndexSectorData.index_sector_data[i];
				getHeaderData(curtDiskID, findSectorNum, &findHeaderData);
				findFileDescription = findHeaderData.diskHeader_data.FileDescription;
				if ((findFileDescription & 1) == 0) {
					continue;
				}
				if (strcmp(findHeaderData.diskHeader_data.Name, dirName) == 0) {
					curtProcessPCB->pcb.curtDir = findHeaderData.diskHeader_data;
					*ErrorReturned = ERR_SUCCESS;
					return;
				}
			}
		}
	}
	*ErrorReturned = ERR_BAD_PARAM;
	return;
}

//For create dir system call, find empty sector for new dir and its index, 
//write the updated data to disk
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
		if ( (index_sector_data.index_sector_data[i] | 0) == 0) {
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

//get the data of some sector in the disk to readbuffer
void readFromDisk(long DiskID, short sectorToRead, char* readBuffer) {

	INT32 LockResult;
	//READ_MODIFY(MEMORY_INTERLOCK_BASE + 1 + DiskID, DO_LOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
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
	
	//if (res == 1) {
	MEM_WRITE(Z502Disk, &mmio);
	//}
	int res = addToDiskQueue();
	
	//READ_MODIFY(MEMORY_INTERLOCK_BASE + 1 + DiskID, DO_UNLOCK, SUSPEND_UNTIL_LOCKED, &LockResult);
	
	
	//For idle
	/*mmio1.Mode = Z502Action;
	mmio1.Field1 = mmio1.Field2 = mmio1.Field3 = mmio1.Field4 = 0;
	MEM_WRITE(Z502Idle, &mmio1);*/
	dispatcher();
	
	
}

//find one empty sector to use, return the number of the found sector 
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

//For create file system call, set fileheader and index data
//return the sector number for file header
void createFile(char* newFileName, long* ErrorReturned, long* fileHeaderSec) {
	short curtDirIndexLocation = curtProcessPCB->pcb.curtDir.IndexLocation;
	union indexSectorData index_sector_data;
	getIndexSectorData(curtDiskID, curtDirIndexLocation, &index_sector_data);
	int emptySectorForFileHeader = findEmptySector(curtDiskID, FREESECTORLOCATION);
	if (emptySectorForFileHeader == -1) {
		*ErrorReturned = ERR_BAD_PARAM;
		return;
	}
	int i = 0;
	for (i = 0; i < 8; i++) {
		if (index_sector_data.index_sector_data[i] == 0) {
			index_sector_data.index_sector_data[i] = emptySectorForFileHeader;
			break;
		}
	}
	int emptySectorForNewIndexSec = findEmptySector(curtDiskID, emptySectorForFileHeader + 1);		//plus 1 since the header has 
	if (emptySectorForNewIndexSec == -1) {															//already occupied 1 sector
		*ErrorReturned = ERR_BAD_PARAM;
		return;
	}
	//Set new File Header
	union diskHeaderData FCB;
	FCB.diskHeader_data.Inode = Inode;
	Inode = Inode + 1;
	memset(FCB.diskHeader_data.Name, 0, 7);
	strcpy(FCB.diskHeader_data.Name, newFileName);

	unsigned char fileDiscription = 0;
	fileDiscription = fileDiscription | (curtProcessPCB->pcb.curtDir.Inode << 3 | 2);
	FCB.diskHeader_data.FileDescription = fileDiscription;

	//get curt time
	MEMORY_MAPPED_IO mmio;
	mmio.Mode = Z502ReturnValue;
	mmio.Field1 = mmio.Field2 = mmio.Field3 = mmio.Field4 = 0;
	MEM_READ(Z502Clock, &mmio);
	memcpy(FCB.diskHeader_data.CreationTime, &mmio.Field1, 3);
	FCB.diskHeader_data.IndexLocation = emptySectorForNewIndexSec;
	FCB.diskHeader_data.FileSize = 0;
	union indexSectorData newIndexSector;
	memset(&newIndexSector, 0, sizeof(union indexSectorData));
	//update curt index sector
	writeIndexSectorToDisk(curtDiskID, curtDirIndexLocation, index_sector_data.char_data);
	//write new file dir to disk
	*fileHeaderSec = emptySectorForFileHeader;
	writeRootDirToDisk(curtDiskID, emptySectorForFileHeader, FCB.char_data);
	//write new index sector to disk
	writeIndexSectorToDisk(curtDiskID, emptySectorForNewIndexSec, newIndexSector.char_data);
	*ErrorReturned = ERR_SUCCESS;
}


//read data of header in the sector from the disk
void getHeaderData(long DiskID, short sectorNum, union diskHeaderData* headerData) {
	readFromDisk(DiskID, sectorNum, headerData->char_data);
}

//open the file has the filename return the sector of the file
void openFile(char* openFileName, long* fileSector, long* ErrorReturned) {
	//short fileSector = 0;
	union indexSectorData curtIndexSectorData;
	short curtDirIndexLocation = curtProcessPCB->pcb.curtDir.IndexLocation;
	getIndexSectorData(curtDiskID, curtDirIndexLocation, &curtIndexSectorData);
	int i = 0;
	short findSectorNum = 0;
	union diskHeaderData findHeaderData;
	unsigned char findFileDescription;
	for (i = 0; i < 8; i++) {
		if (curtIndexSectorData.index_sector_data[i] != 0) {
			findSectorNum = curtIndexSectorData.index_sector_data[i];
			getHeaderData(curtDiskID, findSectorNum, &findHeaderData);
			findFileDescription = findHeaderData.diskHeader_data.FileDescription;
			if ((findFileDescription & 1) == 1) {			//is directory not file
				continue;
			}
			if (strcmp(findHeaderData.diskHeader_data.Name, openFileName) == 0) {
				*fileSector = findSectorNum;
				*ErrorReturned = ERR_SUCCESS;
				return;
			}
		}
	}
	long fileHeaderSec = 0;
	createFile(openFileName, ErrorReturned, &fileHeaderSec);
	*fileSector = fileHeaderSec;
}

//write the data in writtenbuffer to the filesector, set fileLogicalBlock(index data) the filesector
void writeFile(long fileSector, long fileLogicalBlock, char* writtenBuffer, long* ErrorReturned) {
	//get index sector of curt file
	union diskHeaderData curtFileHeaderData;
	getHeaderData(curtDiskID, fileSector, &curtFileHeaderData);
	short curtFileIndexLocation = curtFileHeaderData.diskHeader_data.IndexLocation;
	union indexSectorData curtFileIndexSectorData;
	getIndexSectorData(curtDiskID, curtFileIndexLocation, &curtFileIndexSectorData);

	//set logical block to new free sector, write file to the sector
	short emptySectorNumForFile = findEmptySector(curtDiskID, FREESECTORLOCATION);
	if (emptySectorNumForFile == -1) {
		ErrorReturned = ERR_BAD_PARAM;
		return;
	}
	curtFileIndexSectorData.index_sector_data[fileLogicalBlock] = emptySectorNumForFile;
	writeFileToDisk(curtDiskID, emptySectorNumForFile, writtenBuffer);			//also update bitmap data

	//change file header, size, level, to disk
	//curtFileHeaderData.diskHeader_data.FileSize = 16;
	int i = 0;
	int fileSize = 0;
	for (i = 0; i < 8; i++) {
		if ((curtFileIndexSectorData.index_sector_data[i] | 0) != 0) {
			fileSize = fileSize + 1;
		}
	}
	fileSize = fileSize * 16;
	//printf("filesize ========= %d", fileSize);
	curtFileHeaderData.diskHeader_data.FileSize = fileSize;
	//update curt file index sector
	writeIndexSectorToDisk(curtDiskID, curtFileIndexLocation, curtFileIndexSectorData.char_data);

	//update curt file header data
	writeRootDirToDisk(curtDiskID, fileSector, curtFileHeaderData.char_data);
	*ErrorReturned = ERR_SUCCESS;
}

//write the data in writtenbuffer to the disk
void writeFileToDisk(long DiskID, long fileSecNum, char* writtenBuffer) {
	writeToDisk(DiskID, fileSecNum, writtenBuffer);
	updateBitMap(DiskID, fileSecNum);
}

//Close file system call
void closeFile(long fileSectorNum, long* ErrorReturned) {
	*ErrorReturned = ERR_SUCCESS;
	return;
}

//get data from fileLogicalBlock
void readFile(long fileSector, long fileLogicalBlock, char* readBuffer, long* ErrorReturned) {
	if (fileLogicalBlock < 0 || fileLogicalBlock >= 8) {
		*ErrorReturned = ERR_BAD_PARAM;
		return;
	}
	//get file index sector data
	union diskHeaderData curtFileHeaderData;
	getHeaderData(curtDiskID, fileSector, &curtFileHeaderData);
	short curtFileIndexLocation = curtFileHeaderData.diskHeader_data.IndexLocation;
	union indexSectorData curtFileIndexSectorData;
	getIndexSectorData(curtDiskID, curtFileIndexLocation, &curtFileIndexSectorData);

	//read from index sector data
	getFileDataFromDisk(curtDiskID, curtFileIndexSectorData.index_sector_data[fileLogicalBlock], readBuffer);
	*ErrorReturned = ERR_SUCCESS;
}

//Using diskread to get the content of file from the sector in the disk
void getFileDataFromDisk(long DiskID, long fileSectorNum, char* readBuffer) {
	readFromDisk(DiskID, fileSectorNum, readBuffer);
}

//For dir content system call
void printDirContent(long* ErrorReturned) {
	int isDirectory = (curtProcessPCB->pcb.curtDir.FileDescription & 1);
	if ( isDirectory != 1) {					//not a directory, false;
		*ErrorReturned = ERR_BAD_PARAM;
		return;
	}
	//short curtDirIndexLocation = curtProcessPCB->pcb.curtDir.IndexLocation;
	//short curtHeaderLocation = ROOTDIRLOCATION;
	//union indexSectorData index_sector_data;
	//union diskHeaderData headerData;
	//getIndexSectorData(curtDiskID, curtDirIndexLocation, &index_sector_data);
	//getHeaderData(curtDiskID, curtHeaderLocation, &headerData);
	printf("\tInode\t\tFilename\tD/F\t\tCreationTime\tFileSize\n");
	printDirContentHelper( &(curtProcessPCB->pcb.curtDir));
}

//recursively print the dir content, called by printDirContent
void printDirContentHelper(struct diskHeader* dirHeader) {
	long time = 0;
	memcpy(&time, dirHeader->CreationTime, 3);
	printf("\t%d\t\t%s\t\tD\t\t%d\t\t--\n", dirHeader->Inode, dirHeader->Name, time);
	//get curt dir index sector data
	short curtDirIndexLocation = dirHeader->IndexLocation;
	union indexSectorData index_sector_data;
	getIndexSectorData(curtDiskID, curtDirIndexLocation, &index_sector_data);

	short findSectorNum = 0;
	union diskHeaderData findHeaderData;
	unsigned char findFileDescription;
	int i = 0;
	for (i = 0; i < 8; i++) {
		if (index_sector_data.index_sector_data[i] == 0) {
			continue;
		}
		findSectorNum = index_sector_data.index_sector_data[i];
		getHeaderData(curtDiskID, findSectorNum, &findHeaderData);
		findFileDescription = findHeaderData.diskHeader_data.FileDescription;
		if ((findFileDescription & 1) == 0) {			//the sector is a file
			time = 0;
			memcpy(&time, findHeaderData.diskHeader_data.CreationTime, 3);
			printf("\t%d\t\t%s\t\tF\t\t%d\t\t%hd\n", findHeaderData.diskHeader_data.Inode, 
				findHeaderData.diskHeader_data.Name, time, findHeaderData.diskHeader_data.FileSize);
		}
		else if ((findFileDescription & 1) == 1) {		//is a directory
			/*printf("%hd\n", findSectorNum);
			printf("%d\n", i);*/
			printDirContentHelper(&(findHeaderData.diskHeader_data));
		}
	}
}