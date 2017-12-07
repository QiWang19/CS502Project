/************************************************************************

This code forms the base of the operating system you will
build.  It has only the barest rudiments of what you will
eventually construct; yet it contains the interfaces that
allow test.c and z502.c to be successfully built together.

Revision History:
1.0 August 1990
1.1 December 1990: Portability attempted.
1.3 July     1992: More Portability enhancements.
Add call to SampleCode.
1.4 December 1992: Limit (temporarily) printout in
interrupt handler.  More portability.
2.0 January  2000: A number of small changes.
2.1 May      2001: Bug fixes and clear STAT_VECTOR
2.2 July     2002: Make code appropriate for undergrads.
Default program start is in test0.
3.0 August   2004: Modified to support memory mapped IO
3.1 August   2004: hardware interrupt runs on separate thread
3.11 August  2004: Support for OS level locking
4.0  July    2013: Major portions rewritten to support multiple threads
4.20 Jan     2015: Thread safe code - prepare for multiprocessors
************************************************************************/

#include			 "oscreateProcess.h"
#include			 "page_1.h"
#include			 "page.h"
#include			 "disk.h"
#include			 "file.h"
#include             "global.h"
#include             "syscalls.h"
#include             "protos.h"
#include             "string.h"
#include             <stdlib.h>
#include             <ctype.h>


//  Allows the OS and the hardware to agree on where faults occur
extern void *TO_VECTOR[];

//Defined in oscreateProcess.c
extern struct PCB_Queue* headPCB;
extern struct timer_Queue* headTimer;
extern long lenPCBQ;
extern long exitInterrupt;
//print.c
extern int printFullScheduler;
extern int printFullMemory;

char *call_names[] = { "mem_read ", "mem_write", "read_mod ", "get_time ",
"sleep    ", "get_pid  ", "create   ", "term_proc", "suspend  ",
"resume   ", "ch_prior ", "send     ", "receive  ", "PhyDskRd ",
"PhyDskWrt", "def_sh_ar", "Format   ", "CheckDisk", "Open_Dir ",
"OpenFile ", "Crea_Dir ", "Crea_File", "ReadFile ", "WriteFile",
"CloseFile", "DirContnt", "Del_Dir  ", "Del_File " };

/************************************************************************
INTERRUPT_HANDLER
When the Z502 gets a hardware interrupt, it transfers control to
this routine in the OS.
************************************************************************/
void InterruptHandler(void) {
	INT32 DeviceID;
	INT32 Status;
	
	MEMORY_MAPPED_IO mmio;       // Enables communication with hardware

	static BOOL remove_this_in_your_code = TRUE; /** TEMP **/
	static INT32 how_many_interrupt_entries = 0; /** TEMP **/
	
	//variables
	struct PCB_Queue* deletedTimerPCB;			//the pointer to the pcb just del from timerQ
	exitInterrupt = 1;
												 // Get cause of interrupt
	mmio.Mode = Z502GetInterruptInfo;
	mmio.Field1 = mmio.Field2 = mmio.Field3 = mmio.Field4 = 0;
	MEM_READ(Z502InterruptDevice, &mmio);
	DeviceID = mmio.Field1;
	Status = mmio.Field2;
	

	if (mmio.Field4 != ERR_SUCCESS) {
		printf(
			"The InterruptDevice call in the InterruptHandler has failed.\n");
		printf("The DeviceId and Status that were returned are not valid.\n");
	}

	if (DeviceID >= DISK_INTERRUPT_DISK0 && DeviceID <= DISK_INTERRUPT_DISK7) {
		updateDiskQueue(DeviceID - 5);
	}
	
	switch (DeviceID)
	{
	case TIMER_INTERRUPT:
		
		updateTimerQueue();
		break;
	
	default:
		
		break;
	}
	exitInterrupt = 0;
	
	
	/** REMOVE THE NEXT SIX LINES **/
	/**how_many_interrupt_entries++;**/ /** TEMP **/
	/**
	if (remove_this_in_your_code && (how_many_interrupt_entries < 10)) {
		printf("Interrupt_handler: Found device ID %d with status %d\n",
			(int)mmio.Field1, (int)mmio.Field2);
	}
	**/

}           // End of InterruptHandler

			/************************************************************************
			FAULT_HANDLER
			The beginning of the OS502.  Used to receive hardware faults.
			************************************************************************/

void FaultHandler(void) {
	INT32 DeviceID;

	MEMORY_MAPPED_IO mmio;       // Enables communication with hardware

								 // Get cause of interrupt
	mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
	mmio.Mode = Z502GetInterruptInfo;
	MEM_READ(Z502InterruptDevice, &mmio);
	DeviceID = mmio.Field1;

	

	INT32 Status;
	Status = mmio.Field2;
	switch (DeviceID)
	{
	case INVALID_MEMORY:
		//invalidMemoryHandler(Status);
		InvalidMemoryHandler(Status);
		break;
	default:
		printf("Fault_handler: Found vector type %d with value %d\n", DeviceID,
				Status);
		break;
	}
	//printf("Fault_handler: Found vector type %d with value %d\n", DeviceID,
	//	Status);
} // End of FaultHandler

  /************************************************************************
  SVC
  The beginning of the OS502.  Used to receive software interrupts.
  All system calls come to this point in the code and are to be
  handled by the student written code here.
  The variable do_print is designed to print out the data for the
  incoming calls, but does so only for the first ten calls.  This
  allows the user to see what's happening, but doesn't overwhelm
  with the amount of data.
  ************************************************************************/

void svc(SYSTEM_CALL_DATA *SystemCallData) {
	short call_type;
	static short do_print = 10;
	short i;
	struct PCB_Queue* p = headPCB;
	INT32 Status;
	MEMORY_MAPPED_IO mmio;
	MEMORY_MAPPED_IO mmio1;
	//other variable
	int flag = 0;		//flag is 1 the PCB is deleted from timmer queue
	char* pName;		//The name of the process to be created, getID
	long pID = 0;			//The ID of the process to be created, getID
	long pPriority;		//The priority of the process to be created
	BOOL duplicateName = FALSE;			//The process of the same name has been created
	long res;			//the result of create process
	long sleepTime;		//sleep time in sleep system call
	long fileHeaderSector;// created file header sector num
	INT32 StartingAddress;	//define shared area system call
	INT32 PagesInSharedArea;
	INT32 NumberPreviousSharers;

	call_type = (short)SystemCallData->SystemCallNumber;
	if (do_print > 0) {
		printf("SVC handler: %s\n", call_names[call_type]);
		for (i = 0; i < SystemCallData->NumberOfArguments - 1; i++) {
			//Value = (long)*SystemCallData->Argument[i];
			printf("Arg %d: Contents = (Decimal) %8ld,  (Hex) %8lX\n", i,
				(unsigned long)SystemCallData->Argument[i],
				(unsigned long)SystemCallData->Argument[i]);
		}
		do_print--;
	}
	// Write code here
	switch (call_type) {
		//Get time service call
		case SYSNUM_GET_TIME_OF_DAY:
			mmio.Mode = Z502ReturnValue;
			mmio.Field1 = mmio.Field2 = mmio.Field3 = 0;
			MEM_READ(Z502Clock, &mmio);
			*(long*)SystemCallData->Argument[0] = mmio.Field1;
			break;
		//terminate system call
		case SYSNUM_TERMINATE_PROCESS:
			endProcess((int)SystemCallData->Argument[0], SystemCallData->Argument[1]);
			
			break;
		case SYSNUM_SLEEP:
			//Start the timer
			mmio.Mode = Z502Start;
			mmio.Field1 = (long)SystemCallData->Argument[0];
			mmio.Field2 = mmio.Field3 = 0;
			sleepTime = (long)SystemCallData->Argument[0];
			MEM_WRITE(Z502Timer, &mmio);
			//enqueues the PCB of the running process onto the timer_queue
			addToTimerQueue(sleepTime);
						
			//For idle
			mmio1.Mode = Z502Action;
			mmio1.Field1 = mmio1.Field2 = mmio1.Field3 = mmio1.Field4 = 0;
			MEM_WRITE(Z502Idle, &mmio1);			
			dispatcher();
			break;
		case SYSNUM_GET_PROCESS_ID:
			
			if (strcmp((char*)SystemCallData->Argument[0], "") == 0) {
				mmio.Mode = Z502GetCurrentContext;
				mmio.Field1 = mmio.Field2 = mmio.Field3 = mmio.Field4 = 0;
				MEM_READ(Z502Context, &mmio);
				
				while (p!= NULL && p->pcb.NewContext != mmio.Field1) {
					
					p = p->next;
				}
				if (p == NULL) {
					printf("ERROR! Process does not exist\n");

				}
				else if (p != NULL){
					*(long*)SystemCallData->Argument[1] = p->pcb.process_ID;
					*(long*)SystemCallData->Argument[2] = ERR_SUCCESS;
				}
			}
			else {
				pName = (char*)SystemCallData->Argument[0];
				pID = *(long*)SystemCallData->Argument[1];
				while (p != NULL) {
					if (strcmp(p->pcb.process_Name, pName) == 0 || p->pcb.process_ID == pID) {
						break;
					}
					else {
						p = p->next;
					}
				}
				if (p == NULL) {
					*(long*)SystemCallData->Argument[2] = ERR_BAD_PARAM;
					

				}
				else if (strcmp(p->pcb.process_Name, pName) == 0 ) {		//find the process
					*(long*)SystemCallData->Argument[1] = p->pcb.process_ID;
					if (p->pcb.NewContext == -1) {
						*(long*)SystemCallData->Argument[2] = ERR_BAD_PARAM;
					}
					else {
						*(long*)SystemCallData->Argument[2] = ERR_SUCCESS;
					}
					
				}
				else if (strcmp(p->pcb.process_Name, pName) != 0) {
					*(long*)SystemCallData->Argument[2] = ERR_BAD_PARAM;
					//printf("Illegal ProcessName\n");
				}
				else if (p->pcb.process_ID != pID) {
					*(long*)SystemCallData->Argument[2] = ERR_BAD_PARAM;
					printf("Process does not exist\n");
				}
			}
			break;
		case SYSNUM_PHYSICAL_DISK_WRITE:
			

			mmio.Mode = Z502DiskWrite;
			mmio.Field1 = (long)SystemCallData->Argument[0];
			mmio.Field2 = (long)SystemCallData->Argument[1];
			mmio.Field3 = (char*)SystemCallData->Argument[2];
			mmio.Field4 = 0;
			MEM_WRITE(Z502Disk, &mmio);
			addToDiskQueue();
			//For idle
			mmio1.Mode = Z502Action;
			mmio1.Field1 = mmio1.Field2 = mmio1.Field3 = mmio1.Field4 = 0;
			MEM_WRITE(Z502Idle, &mmio1);
			dispatcher();
			//Make sure that disk is running
			
			
			break;
		case SYSNUM_PHYSICAL_DISK_READ:
			//Make sure the disk is free
			mmio1.Mode = Z502Status;
			mmio1.Field1 = (long)SystemCallData->Argument[0];
			mmio1.Field2 = mmio1.Field3 = 0;
			MEM_READ(Z502Disk, &mmio1);
			if (mmio1.Field2 == DEVICE_IN_USE) {
				while (mmio1.Field2 != DEVICE_FREE)
				{
					mmio1.Mode = Z502Status;
					mmio1.Field1 = (long)SystemCallData->Argument[0];
					mmio1.Field2 = mmio1.Field3 = 0;
					MEM_READ(Z502Disk, &mmio1);
				}
			}
			mmio.Mode = Z502DiskRead;
			mmio.Field1 = (long)SystemCallData->Argument[0];
			mmio.Field2 = (long)SystemCallData->Argument[1];
			mmio.Field3 = (char*)SystemCallData->Argument[2];
			mmio.Field4 = 0;
			MEM_WRITE(Z502Disk, &mmio);
			addToDiskQueue();
			//For idle
			mmio1.Mode = Z502Action;
			mmio1.Field1 = mmio1.Field2 = mmio1.Field3 = mmio1.Field4 = 0;
			MEM_WRITE(Z502Idle, &mmio1);
			dispatcher();
			//Wait for the disk action to complete
			
			break;
		case SYSNUM_CHECK_DISK:
			mmio.Mode = Z502CheckDisk;
			mmio.Field1 = (long)SystemCallData->Argument[0];
			mmio.Field2 = mmio.Field3 = mmio.Field4 = 0;
			MEM_READ(Z502Disk, &mmio);
			break;
		case SYSNUM_CREATE_PROCESS:
			pName = (char*)SystemCallData->Argument[0];			
			pPriority = (long)SystemCallData->Argument[2];
			
			//Get number of process
			if (lenPCBQ + 1 > 15) {
				*(long*)SystemCallData->Argument[3] = pID;
				*(long*)SystemCallData->Argument[4] = ERR_BAD_PARAM;
				break;
			}		
			//cannot create if illegal priority
			if (pPriority < 0) {
				
				*(long*)SystemCallData->Argument[3] = pID;
				*(long*)SystemCallData->Argument[4] = ERR_BAD_PARAM;
				break;
			}
			//If name is duplicate
			while (p != NULL) {				
				if (strcmp(p->pcb.process_Name, pName) == 0) {
					break;
				}
				else {
					p = p->next;
				}
			}
			if (p != NULL) {										//duplicate name
				*(long*)SystemCallData->Argument[4] = ERR_BAD_PARAM;
				break;
			}
			createProcesTest3(pName, (long)SystemCallData->Argument[1], pPriority, &pID, &res);
			*(long*)SystemCallData->Argument[3] = pID;
			*(long*)SystemCallData->Argument[4] = res;
			break;
		case SYSNUM_FORMAT:
			//diskID to be format (long)SystemCallData->Argument[0]
			//error SystemCallData->Argument[1]
			formatDisk((long)SystemCallData->Argument[0], SystemCallData->Argument[1]);
			break;
		case SYSNUM_OPEN_DIR:
			//diskID to open (long)SystemCallData->Argument[0]
			//directory name (char*)SystemCallData->Argument[1]
			//error SystemCallData->Argument[2]
			openDirectory((long)SystemCallData->Argument[0], (char*)SystemCallData->Argument[1], SystemCallData->Argument[2]);
			break;
		case SYSNUM_CREATE_DIR:
			//Dir name to create (char*)SystemCallData->Argument[0]
			//Error message SystemCallData->Argument[1]
			createDirectory((char*)SystemCallData->Argument[0], SystemCallData->Argument[1]);
			break;
		case SYSNUM_CREATE_FILE:
			//file name to create (char*)SystemCallData->Argument[0]
			//ErrorReturned SystemCallData->Argument[1]
			
			createFile((char*)SystemCallData->Argument[0], SystemCallData->Argument[1], &fileHeaderSector);
			break;
		case SYSNUM_OPEN_FILE:
			//(char*)SystemCallData->Argument[0] open file name
			//inode SystemCallData->Argument[1]
			//errorreturned SystemCallData->Argument[2]
			openFile((char*)SystemCallData->Argument[0], SystemCallData->Argument[1], SystemCallData->Argument[2]);
			break;
		case SYSNUM_WRITE_FILE:
			//file sector (long)SystemCallData->Argument[0]
			//file logical sector SystemCallData->Argument[1]
			//written buffer (char*)SystemCallData->Argument[2]
			//errorreturned SystemCallData->Argument[3]
			writeFile((long)SystemCallData->Argument[0], (long)SystemCallData->Argument[1], (char*)SystemCallData->Argument[2], SystemCallData->Argument[3]);
			break;
		case SYSNUM_CLOSE_FILE:
			//file sector (long)SystemCallData->Argument[0]
			//errorreturned SystemCallData->Argument[1]
			closeFile((long)SystemCallData->Argument[0], SystemCallData->Argument[1]);
			break;
		case SYSNUM_READ_FILE:
			//file sector (long)SystemCallData->Argument[0]
			//file logical sector SystemCallData->Argument[1]
			//read buffer (char*)SystemCallData->Argument[2]
			//errorreturned SystemCallData->Argument[3]
			readFile((long)SystemCallData->Argument[0], SystemCallData->Argument[1], (char*)SystemCallData->Argument[2], SystemCallData->Argument[3]);
			break;
		case SYSNUM_DIR_CONTENTS:
			printDirContent(SystemCallData->Argument[0]);
			break;
		case SYSNUM_DEFINE_SHARED_AREA:
			//starting address(INT32)	SystemCallData->Argument[0]
			//pages in shared area(INT32)
			//area tag char[] 
			//num previous sharers INT32
			//error INT32
			StartingAddress = (INT32)SystemCallData->Argument[0];
			PagesInSharedArea = (INT32)SystemCallData->Argument[1];
			NumberPreviousSharers = (INT32*)SystemCallData->Argument[3];
			DefineSharedArea(StartingAddress, PagesInSharedArea, (char*)SystemCallData->Argument[2],
				(INT32*)SystemCallData->Argument[3], (INT32*)SystemCallData->Argument[4]);
			break;
		case SYSNUM_SEND_MESSAGE:
			SendMessage((INT32)(SystemCallData->Argument[0]), (char*)(SystemCallData->Argument[1]), 
				(INT32)(SystemCallData->Argument[2]), (INT32*)(SystemCallData->Argument[3]));
			break;
		case SYSNUM_RECEIVE_MESSAGE:
			
			ReceiveMessage((INT32)(SystemCallData->Argument[0]), (char*)(SystemCallData->Argument[1]), 
				(INT32)(SystemCallData->Argument[2]), (INT32*)(SystemCallData->Argument[3]), 
				(INT32*)(SystemCallData->Argument[4]), (INT32*)(SystemCallData->Argument[5]));
			break;
		default:
			printf("ERROR! call_type not recognized!\n");
			printf("Call_type is - %i\n", call_type);
			break;
	}
}                                               // End of svc

												/************************************************************************
												osInit
												This is the first routine called after the simulation begins.  This
												is equivalent to boot code.  All the initial OS components can be
												defined and initialized here.
												************************************************************************/

void osInit(int argc, char *argv[]) {
	void *PageTable = (void *)calloc(2, NUMBER_VIRTUAL_PAGES);
	INT32 i;
	MEMORY_MAPPED_IO mmio;

	// Demonstrates how calling arguments are passed thru to here

	printf("Program called with %d arguments:", argc);
	for (i = 0; i < argc; i++)
		printf(" %s", argv[i]);
	printf("\n");
	printf("Calling with argument 'sample' executes the sample program.\n");

	// Here we check if a second argument is present on the command line.
	// If so, run in multiprocessor mode.  Note - sometimes people change
	// around where the "M" should go.  Allow for both possibilities
	if (argc > 2) {
		if ((strcmp(argv[1], "M") == 0) || (strcmp(argv[1], "m") == 0)) {
			strcpy(argv[1], argv[2]);
			strcpy(argv[2], "M\0");
		}
		if ((strcmp(argv[2], "M") == 0) || (strcmp(argv[2], "m") == 0)) {
			printf("Simulation is running as a MultProcessor\n\n");
			mmio.Mode = Z502SetProcessorNumber;
			mmio.Field1 = MAX_NUMBER_OF_PROCESSORS;
			mmio.Field2 = (long)0;
			mmio.Field3 = (long)0;
			mmio.Field4 = (long)0;
			MEM_WRITE(Z502Processor, &mmio);   // Set the number of processors
		}
	}
	else {
		printf("Simulation is running as a UniProcessor\n");
		printf(
			"Add an 'M' to the command line to invoke multiprocessor operation.\n\n");
	}

	//          Setup so handlers will come to code in base.c

	TO_VECTOR[TO_VECTOR_INT_HANDLER_ADDR] = (void *)InterruptHandler;
	TO_VECTOR[TO_VECTOR_FAULT_HANDLER_ADDR] = (void *)FaultHandler;
	TO_VECTOR[TO_VECTOR_TRAP_HANDLER_ADDR] = (void *)svc;

	//  Determine if the switch was set, and if so go to demo routine.

	PageTable = (void *)calloc(2, NUMBER_VIRTUAL_PAGES);
	if ((argc > 1) && (strcmp(argv[1], "sample") == 0)) {
		mmio.Mode = Z502InitializeContext;
		mmio.Field1 = 0;
		mmio.Field2 = (long)SampleCode;
		mmio.Field3 = (long)PageTable;

		MEM_WRITE(Z502Context, &mmio);   // Start of Make Context Sequence
		mmio.Mode = Z502StartContext;
		// Field1 contains the value of the context returned in the last call
		mmio.Field2 = START_NEW_CONTEXT_AND_SUSPEND;
		MEM_WRITE(Z502Context, &mmio);     // Start up the context

	} // End of handler for sample code - This routine should never return here
	  //  By default test0 runs if no arguments are given on the command line
	  //  Creation and Switching of contexts should be done in a separate routine.
	  //  This should be done by a "OsMakeProcess" routine, so that
	  //  test0 runs on a process recognized by the operating system.
	
	//use command to create process
	long testAddress = (long)test0;
	char* testName = (char*)malloc(256);
	long newProcessID = 0;
	long ErrorReturned = 0;
	//default test is test0, change test here for testing
	if (argv[1] == NULL) {
		testAddress = (long)test26;
		testName = "test26";
		printFullScheduler = 0;
	}
	else if (strcmp(argv[1], "test1") == 0) {
		testAddress = (long)test1;
		strcpy(testName, argv[1]);
	}
	else if (strcmp(argv[1], "test2") == 0) {
		testAddress = (long)test2;
		strcpy(testName, argv[1]);
	}
	else if (strcmp(argv[1], "test3") == 0) {
		testAddress = (long)test3;
		strcpy(testName, argv[1]);
		printFullScheduler = 1;
	}
	else if (strcmp(argv[1], "test4") == 0) {
		testAddress = (long)test4;
		strcpy(testName, argv[1]);
		printFullScheduler = 1;
	}
	else if (strcmp(argv[1], "test5") == 0) {
		testAddress = (long)test5;
		strcpy(testName, argv[1]);
		printFullScheduler = 1;
	}
	else if (strcmp(argv[1], "test6") == 0) {
		testAddress = (long)test6;
		strcpy(testName, argv[1]);
		printFullScheduler = 1;
	}
	else if (strcmp(argv[1], "test7") == 0) {
		testAddress = (long)test7;
		strcpy(testName, argv[1]);
		printFullScheduler = 1;
	}
	else if (strcmp(argv[1], "test8") == 0) {
		testAddress = (long)test8;
		strcpy(testName, argv[1]);
		printFullScheduler = 1;
	}
	else if (strcmp(argv[1], "test9") == 0) {
		testAddress = (long)test9;
		strcpy(testName, argv[1]);
		printFullScheduler = 0;
	}
	else if (strcmp(argv[1], "test10") == 0) {
		testAddress = (long)test10;
		strcpy(testName, argv[1]);
		printFullScheduler = 0;
	}
	else if (strcmp(argv[1], "test11") == 0) {
		testAddress = (long)test11;
		strcpy(testName, argv[1]);
	}
	else if (strcmp(argv[1], "test12") == 0) {
		testAddress = (long)test12;
		strcpy(testName, argv[1]);
		printFullScheduler = 1;
	}
	else if (strcmp(argv[1], "test13") == 0) {
		testAddress = (long)test13;
		strcpy(testName, argv[1]);
		printFullScheduler = 0;
	}
	else if (strcmp(argv[1], "test14") == 0) {
		testAddress = (long)test14;
		strcpy(testName, argv[1]);
	}
	else if (strcmp(argv[1], "test15") == 0) {
		testAddress = (long)test15;
		strcpy(testName, argv[1]);
	}
	else if (strcmp(argv[1], "test16") == 0) {
		testAddress = (long)test16;
		strcpy(testName, argv[1]);
	}
	else if (strcmp(argv[1], "test21") == 0) {
		testAddress = (long)test21;
		printFullMemory = 1;
	}
	else if (strcmp(argv[1], "test22") == 0) {
		testAddress = (long)test22;
		printFullMemory = 1;
	}
	else if (strcmp(argv[1], "test23") == 0) {
		testAddress = (long)test23;
		printFullMemory = 1;
		printFullScheduler = 1;
	}
	else if (strcmp(argv[1], "test24") == 0) {
		testAddress = (long)test24;
		printFullMemory = -1;
		printFullScheduler = -1;
	}
	else if (strcmp(argv[1], "test25") == 0) {
		testAddress = (long)test25;
		printFullMemory = -1;
		printFullScheduler = -1;
	}
	else if (strcmp(argv[1], "test26") == 0) {
		testAddress = (long)test26;
		printFullMemory = -1;
		printFullScheduler = 1;
	}
	else if (strcmp(argv[1], "test27") == 0) {
		testAddress = (long)test27;
	}
	else if (strcmp(argv[1], "test28") == 0) {
		testAddress = (long)test28;
		printFullMemory = -1;
	}
		
	
	//initFrameList();
	InitFrameTable();
	BuildFrameList();
	os_create_process(testName, testAddress, 10, &newProcessID, &ErrorReturned);


}                                               // End of osInit