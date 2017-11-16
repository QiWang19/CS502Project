

#define		SECTORSIZE                           (short)16
#define		BITMAPSIZE							 (short)4
#define		ROOTDIRHEADER						 (short)1
//#define		SWAPSPACE						     (short)3
#define		SWAPSPACE						     (short)500
#define		ROOTDIRLOCATION						 (short)17
#define		ROOTINDEXLOCATION					 (short)18
#define		SWAPLOCATION						 (short)19
#define		BITMAPLOCATION						 (short)1
//#define		FREESECTORLOCATION					 (short)31
#define		FREESECTORLOCATION					 (short)2019

#ifndef _COMMON
#define _COMMON

//Define the structure of the sector0, the file/disk header, index sector
struct sector0 {
	char DiskID;
	char BitmapSize;
	char RootDirSize;
	char SwapSize;
	short DiskLength;
	short BitmapLocation;
	short RootDirLocation;
	short SwapLocation;
	char RESERVED[4];
};

union diskSector0Date {
	char char_data[SECTORSIZE];
	struct sector0 sector0_data;
};

struct diskHeader {
	unsigned char Inode;
	char Name[7];
	unsigned char FileDescription;
	char CreationTime[3];
	short IndexLocation;
	short FileSize;
};

union diskHeaderData {
	char char_data[SECTORSIZE];
	struct diskHeader diskHeader_data;
};

union indexSectorData {
	char char_data[SECTORSIZE];
	unsigned short index_sector_data[8];
};

#endif

//The functions of operations defined in file.c
void initSector0(long DiskID, long* result);
void initBitMap(long DiskID, short bitMapLocation, short bitMapSize);
void initSwapSectors(long DiskID, short swapLocation, short swapSize);
void setSwapBitMap(long DiskID, short swapLocation, short swapSize);
void writeBitmapToDisk(long DiskID);
void writeToDisk(long DiskID, int sectorToWrite, char* writtenBuffer);
void writeSector0ToDisk(long DiskID, int sectorToWrite, union diskSector0Date* disk_sector0_data);
void updateBitMap(long DiskID, int sectorToWrite);
void initRootDir(long DiskID, short rootDirLocation, short rootHeaderSize);
void writeRootDirToDisk(long DiskID, short rootDirLocation, union diskHeaderData* disk_header_data);
void writeIndexSectorToDisk(long DiskID, short rootIndexLocation, union indexSectorData* index_sector_data);
void formatDisk(long DiskID, long* ErrorReturned);
void openDirectory(long DiskID, char* dirName, long* ErrorReturned);
void getIndexSectorData(long curtDiskID, short indexLocation, union indexSectorData* indexSecData);
void createDirectory(char* newDirName, long* ErrorReturned);
void readFromDisk(long DiskID, short sectorToRead, char* readBuffer);
short findEmptySector(long DiskID, short startSectortoFind);
void createFile(char* newFileName, long* RrrorReturned, long* fileHeaderSec);
void getHeaderData(long DiskID, short sectorNum, union diskHeaderData* headerData);
void openFile(char* openFileName, long* fileLogicSector, long* ErrorReturned);
void writeFileToDisk(long DiskID, long fileSecNum, char* writtenBuffer);
void writeFile(long fileSector, long fileLogicalBlock, char* writtenBuffer, long* ErrorReturned);
void closeFile(long fileSectorNum, long* ErrorReturned);
void readFile(long fileSector, long fileLogicalBlock, char* readBuffer, long* ErrorReturned);
void getFileDataFromDisk(long DiskID, long fileSectorNum, char* readBuffer);
void printDirContent(long* ErrorReturned);
void printDirContentHelper(struct diskHeader* dirHeader);
void writeVictimPageToDisk(long DiskID, int sectorToWrite, char* writtenBuffer);