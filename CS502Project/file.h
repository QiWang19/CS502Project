
#define		SECTORSIZE                           (short)16
#define		BITMAPSIZE							 (short)4
#define		ROOTDIRHEADER						 (short)1
#define		SWAPSPACE						     (short)3
#define		ROOTDIRLOCATION						 (short)17
#define		ROOTINDEXLOCATION					 (short)18
#define		SWAPLOCATION						 (short)19
#define		BITMAPLOCATION						 (short)1


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

void initSector0(long DiskID, long* result);
void initBitMap(long DiskID, short bitMapLocation, short bitMapSize);
void writeBitmapToDisk(long DiskID);
void writeToDisk(long DiskID, int sectorToWrite, char* writtenBuffer);
void writeSector0ToDisk(long DiskID, int sectorToWrite, union diskSector0Date* disk_sector0_data);
void updateBitMap(long DiskID, int sectorToWrite);
void initRootDir(long DiskID, short rootDirLocation, short rootHeaderSize);
void writeRootDirToDisk(long DiskID, short rootDirLocation, union diskHeaderData* disk_header_data);
void writeIndexSectorToDisk(long DiskID, short rootIndexLocation, union indexSectorData* index_sector_data);
void formatDisk(long DiskID, long* ErrorReturned);