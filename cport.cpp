/*****************************************************************************
 -                                                                           -
 -                    POLONY SEQUENCER ACQUISITION SUITE                     -
 -                                Church Lab                                 -
 -                          Harvard Medical School                           -
 -                                                                           -
 -    Free software for running a Polony Sequencing automated microscope     -
 -                                                                           -
 - ========================================================================= -
 -                                                                           -
 -                                                                           -
 - cport.cpp                                                                 -
 -                                                                           -
 -   Interface class to general serial port objects                          -
 -                                                                           -
 -   Written by Michelle Kuykendal, 08-10-2005                               -
 -                                                                           -
 - Revised                                                                   -
 -                                                                           -
 -                                                                           -
 - This software may be used, modified, and distributed freely, but this     -
 - header may not be modified and must appear at the top of this file.       -
 -                                                                           -
 -                                                                           -
 *****************************************************************************/

/******************************************************************************
*******************************************************************************
**** File: cport.cpp                                                       ****
**** Requirements: cport.h should exist in the same directory as this file.****
****  if the file is being used in the GUI_Devices project, the StdAfx.h   ****
****  and StdAfx.cpp files should also exit in the same directory. if this ****
****  file is used outside of the GUI_Devices project(or any Microsoft     ****
****  Visual project) then the line of code "#include "StdAfx.h" may be    ****
****  commented.                                                           ****
**** Purpose: this file defines functions of the CPort class to be used    ****
****  with a serial communication port. it enables the user to open and    ****
****  close a comm port, to write to and read from the comm port (in       ****
****  strings or one byte at a time), and to check for the completion of a ****
****  write or read (in strings or one byte at a time).                    ****
**** Initial Version: Aug. 10, 2005                                        ****
**** Programmer: Michelle Kuykendal                                        ****
*******************************************************************************
******************************************************************************/

#include "stdafx.h"
#include "cport.h"  //definition of the CPort class

//REQUIREMENTS: none.
//USE: CPort() is the constructor for the port class.
//RETURN VALUE: none.
CPort::CPort()
{
	portIsInit = false;
}

//REQUIREMENTS: none.
//USE: ~CPort() is the destructor for the port class, which will close 
// the comm port handle in case of improper function closure.
//RETURN VALUE: none.
CPort::~CPort()
{
    CloseHandle(mComm);
}

//REQUIREMENT: the comm port name must be set prior to a call of this
// function in the variable mPort.
//USE: OpenCPort() will open the comm port and return the handle to the
// port, mComm, for future use. the comm port will be opened for read 
// and write at 9600 baud, no parity, 8 data bits, and 1 stop bit. during
// normal use, CloseCPort() should be called prior to function termination
// to close the comm port handle.
//RETURN VALUE: the return will be false if the handle returned when
// opening the port is invalid, the device control block is not 
// successfully retrieved, or the comm port and timeout parameters are 
// not properly set. the return will be true if the comm port is opened
// and properly setup for reading and writing.
bool CPort::OpenCPort()
{
	if(!portIsInit)//check to see if the port has been initialized
	{
		//open the comm port assigned by the user to mPort for reading and 
		// writing. no sharing of port permitted and no security implemented.
		// serial port must be created as OPEN_EXISTING, and FILE_FLAG_OVERLAPPED
		// permits program to do other things while asynchronous transfer occurs.
		// returns the handle mComm for reference to comm port
		mComm = CreateFile(mPort,	//pointer to name of the file
						   GENERIC_READ | GENERIC_WRITE, //access mode
						   0,		//share mode
						   0,		//pointer to security attributes
						   OPEN_EXISTING, //how to create file
						   FILE_FLAG_OVERLAPPED,//for asynchronous transfer
						   0);		//handle to file with attributes to copy

		if(mComm == INVALID_HANDLE_VALUE)//unsuccessful opening of comm port
			return false;

		if(!GetCommState(mComm, &mDCB))
			return false;

		//set the device control block for 19200 baud, no parity, 8 data bits, 
		//and 2 stop bits. disable XON/XOFF and hardware flow control
		mDCB.BaudRate     = 19200;
		mDCB.ByteSize     = 8;
		mDCB.Parity       = EVENPARITY;
		mDCB.StopBits     = TWOSTOPBITS;

		mDCB.fBinary      = true;
		mDCB.fParity      = false;

		mDCB.fOutxCtsFlow = false;
		mDCB.fOutxDsrFlow = false;
		mDCB.fDtrControl  = DTR_CONTROL_ENABLE;
		mDCB.fOutX        = false;
		mDCB.fInX         = false;
		mDCB.fRtsControl  = RTS_CONTROL_ENABLE;
		

/*
        case EHandshakeOff:
        dcb.fOutxCtsFlow = false;                    // Disable CTS monitoring
        dcb.fOutxDsrFlow = false;                    // Disable DSR monitoring
        dcb.fDtrControl = DTR_CONTROL_ENABLE;        // Enable DTR line
      // See http://tinyurl.com/8bulcg for explanation
        dcb.fOutX = false;                            // Disable XON/XOFF for transmission
        dcb.fInX = false;                            // Disable XON/XOFF for receiving
        dcb.fRtsControl = RTS_CONTROL_ENABLE;        // Enable RTS line
      // See http://tinyurl.com/8bulcg for explanation
        break;
*/

		//set the timeout structure for both reading and writing. the total
		// time allowed for either read or write is determined by:
		// (Multiplier * # of chars to be read) + timeout const.
		mTimeouts.ReadTotalTimeoutMultiplier  = 100;
		mTimeouts.ReadTotalTimeoutConstant    = 300;
		mTimeouts.WriteTotalTimeoutMultiplier = 100;
		mTimeouts.WriteTotalTimeoutConstant   = 300;
		mTimeouts.ReadIntervalTimeout         = 300;//time allowed between characters

		//set the comm timeout values
		if(!SetCommTimeouts(mComm, &mTimeouts))
			return false;

		//set comm parameters as designated by device control block
		if(!SetCommState(mComm, &mDCB))
			return false;
		
		portIsInit = true;//set the initialization checker to true

		return true;//serial port is properly setup
	}
	else//the port was already initialized
		return false;
}

//REQUIREMENTS: a call to OpenCPort() should be made prior to this call
// for proper execution.
//USE: CloseCPort() will close the handle to the comm port opened using
// the OpenCPort() function.
//RETURN VALUE: the return value will be false if the comm port handle 
// is not closed. it will be true if the comm port handle is closed.
bool CPort::CloseCPort()
{
	if(portIsInit)//check to see if the port has been initialized
	{
		if(CloseHandle(mComm))//close the port handle
		{
			portIsInit = false;//reset the initialization checker to false
			return true;
		}
		else//the handle, mComm, could not be closed
			return false;
	}
	else//the port was not first initialized
		return false;
}


//REQUIREMENT: a call to OpenCPort() should be made prior to this call
// for proper execution. also, the output buffer, mOutBuf, must be 
// assigned a string to write.
//USE: WriteCPort() will write the string, mOutBuf, to the comm port, 
// mComm. if the write is immediately completed, mResWrite will be 
// assigned true. otherwise it will remain false. if it is false, the 
// CheckWrite() function should be called to check for completion of the
// write. 
//RETURN VALUE: return will be false if the overlapped event (used for 
// checking for completion of write by monitoring the changing of the 
// event during a call to CheckWrite()) is not created, if the write 
// command returns false but is not currently pending, or if the comm
// port is not initalized. the return will be true if the write is 
// successfully initiated regardless of the completion of the write.
bool CPort::WriteCPort()
{

	if(portIsInit)//check to see if the port has been initialized
	{
		//create the write overlap structure for use in checking completion
		// of write as the event changes
		mOverlapWrite.Offset     = 0;
		mOverlapWrite.OffsetHigh = 0; 
		mOverlapWrite.hEvent     = CreateEvent(NULL, TRUE, FALSE, NULL); 
		
		if(mOverlapWrite.hEvent == NULL)//check for invalid creation of event
			return false;

		//set the number of bytes to write based on the user assigned string
		DWORD mBytesToWrite = strlen( mOutBuf );//wcslen( mOutBuf );
		
		mResWrite = false;//initially the result is false
		
		// function will write the string in mOutBuf to the comm port mComm
		// and will return false if write is not immediately completed. it will
		// assign the mOverlapWrite event to the status of the write for future
		// checking of completion by the GetOverlappedEvent function.
		if(WriteFile(mComm, &mOutBuf, mBytesToWrite, &mBytesWritten, &mOverlapWrite)) 
		{
			mResWrite = true;//write completed immediately, set result true
			CloseHandle(mOverlapWrite.hEvent);//close write event handle
		}
		else
		{
			//write returned false but not due to delay, so report error
			if(GetLastError() != ERROR_IO_PENDING)
				return false;
		}

		return true;//the write was successfully initiated
	}
	else//the port was not first initialized
		return false;
}


//REQUIREMENT: a call to OpenCPort() should be made prior to this call
// for proper execution. also, the output character, mOutOne, must be 
// assigned a value to write.
//USE: WriteCPortOneByte() will write the character, mOutOne, to the comm
// port, mComm. if the write is immediately completed, mResWrite will be 
// assigned true. otherwise it will remain false. if it is false, the 
// CheckWrite() function should be called to check for completion of the 
// write. 
//RETURN VALUE: return will be false if the overlapped event (used for 
// checking for completion of write by monitoring the changing of the 
// event during a call to CheckWrite()) is not created, if the write 
// command returns false but is not currently pending, or if the comm
// port is not initalized. the return will be true if the write is 
// successfully initiated regardless of the completion of the write.
bool CPort::WriteCPortOneByte()
{
	if(portIsInit)//check to see if the port has been initialized
	{
		//create the write overlap structure for use in checking completion
		// of write as the event changes
		mOverlapWrite.Offset = 0;
		mOverlapWrite.OffsetHigh = 0; 
		mOverlapWrite.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL); 
		
		if(mOverlapWrite.hEvent == NULL)//check for invalid creation of event
			return false;

		mResWrite = false;//initially the result is false
		
		//function will write the string in mOutBuf to the comm port mComm
		// and will return false if write is not immediately completed. it will
		// assign the mOverlapWrite event to the status of the write for future
		// checking of completion by the GetOverlappedEvent function.
		if(WriteFile(mComm, &mOutOne, 1, &mBytesWritten, &mOverlapWrite)) 
		{
			mResWrite = true;//write completed immediately, set result true
			CloseHandle(mOverlapWrite.hEvent);//close write event handle
		}
		else
		{
			//write returned false but not due to delay, so report error
			if(GetLastError() != ERROR_IO_PENDING)
				return false;
		}

		return true;//the write was successfully initiated
	}
	else//the port was not first initialized
		return false;
}


//REQUIREMENT: a call to OpenCPort() and WriteCPort() should be made prior
// to this call for proper execution.
//USE: CheckWrite() will check to see if write was successfully completed.
// if it is completed, mResWrite will be assigned true, otherwise it 
// will remain false.
//RETURN VALUE: the return will be false if the port is not initialized. 
// the return will be true if the check for completion was successful 
// regardless of the status of the write completion.
bool CPort::CheckWrite()
{
	if(portIsInit)//check to see if the port has been initialized
	{
		//function will check to see that write is completed based on the 
		// the overlap event changing. the false parameter means it will return
		// false if not completed and will not wait for completion. should loop
		// to continue checking for completion of write.
		if(GetOverlappedResult(mComm, &mOverlapWrite, &mBytesWritten, false))
		{
			mResWrite = true;//write completed
			CloseHandle(mOverlapWrite.hEvent);//close write event handle
		}

		return true;//the write was checked
	}
	else//the port was not first initialized
		return false;
}


//REQUIREMENT: a call to OpenCPort() should be made prior to this call
// for proper execution. also, mBytesToRaed must be assigned a value 
// exactly equal to the number of bytes that are expected to be returned.
//USE: ReadCPort() will read the comm port, mComm, and assign the string 
// to mInBuf. if the read is immediately completed, mResRead will be 
// assigned true. otherwise it will remain false. if it is false, the 
// CheckRead() function should be called to check for completion of the 
// read.
//RETURN VALUE: return will be false if the overlapped event (used for 
// checking for completion of read by monitoring the changing of the 
// event during a call to CheckRead()) is not created, if the read 
// command returns false but is not currently pending, or if the comm
// port is not initalized. the return will be true if the read is 
// successfully initiated regardless of the completion of the read.
bool CPort::ReadCPort()
{
	if(portIsInit)//check to see if the port has been initialized
	{
		//create the read overlap structure for use in checking completion
		// of read as the event changes
		mOverlapRead.Offset = 0;
		mOverlapRead.OffsetHigh = 0; 
		mOverlapRead.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL); 

		if(mOverlapRead.hEvent == NULL)//check for invalid creation of event
			return false;

		for(int j = 0; j < 500; j++)//initialize the input buffer to null
			mInBuf[j] = NULL;

		mResRead = false;//initialize the read result to false

		//function will read the string into mInBuf from the comm port mComm
		// and will return false if read is not immediately completed. it will
		// assign the mOverlapRead event to the status of the read for future
		// checking of completion by the GetOverlappedEvent function.
		if(ReadFile(mComm, &mInBuf, mBytesToRead, &mBytesRead, &mOverlapRead))
		{
			mResRead = true;//read completed immediately, set result true
			CloseHandle(mOverlapRead.hEvent);//close read event handle
		}
		else
		{
			//read returned false but not due to delay, so report error
			//fprintf(stdout, "%d %d - %d %d %d %d %d %d -\n", mBytesToRead, mBytesRead, mInBuf[0], mInBuf[1], mInBuf[2], mInBuf[3], mInBuf[4], mInBuf[5]);
			if(GetLastError() != ERROR_IO_PENDING)
				return false;
		}

		return true;//the read was successfully initiated
	}
	else//the port was not first initialized
		return false;
}


//REQUIREMENT: a call to OpenCPort() should be made prior to this call
// for proper execution.
//USE: ReadCPortOneByte() will read one byte from the comm port, mComm,
// and assign the character to mInOne. if the read is immediately 
// completed, mResRead will be assigned true. otherwise it will remain 
// false. if it is false, the CheckRead() function should be called to
// check for completion of the read.
//RETURN VALUE: return will be false if the overlapped event (used for 
// checking for completion of read by monitoring the changing of the 
// event during a call to CheckRead()) is not created, if the read 
// command returns false but is not currently pending, or if the comm
// port is not initalized. the return will be true if the read is 
// successfully initiated regardless of the completion of the read.
bool CPort::ReadCPortOneByte()
{
	if(portIsInit)//check to see if the port has been initialized
	{
		//create the read overlap structure for use in checking completion
		// of read as the event changes
		mOverlapRead.Offset = 0;
		mOverlapRead.OffsetHigh = 0; 
		mOverlapRead.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL); 

		if(mOverlapRead.hEvent == NULL)//check for invalid creation of event
			return false;

		mResRead = false;//initialize the read result to false

		//function will read the string into mInBuf from the comm port mComm
		// and will return false if read is not immediately completed. it will
		// assign the mOverlapRead event to the status of the read for future
		// checking of completion by the GetOverlappedEvent function.
		if(ReadFile(mComm, &mInOne, 1, &mBytesRead, &mOverlapRead))
		{
			mResRead = true;//read completed immediately, set result true
			CloseHandle(mOverlapRead.hEvent);//close read event handle
		}
		else
		{
			//read returned false but not due to delay, so report error
			if(GetLastError() != ERROR_IO_PENDING)
				return false;
		}

		return true;//the read was successfully initiated
	}
	else//the port was not first initialized
		return false;
}


//REQUIREMENT: a call to OpenCPort() and ReadCPort() should be made prior
// to this call for proper execution.
//USE: CheckRead() will check to see if read was successfully completed.
// if it is completed, mResRead will be assigned true, otherwise it 
// will remain false.
//RETURN VALUE: the return will be false if the port is not initialized. 
// the return will be true if the check for completion was successful 
// regardless of the status of the read completion.
bool CPort::CheckRead()
{
	if(portIsInit)//check to see if the port has been initialized
	{
		//function will check to see that read is completed based on the 
		// the overlap event changing. the false parameter means it will return
		// false if not completed and will not wait for completion. should loop
		// to continue checking for completion of read.
		if(GetOverlappedResult(mComm, &mOverlapRead, &mBytesRead, false))
		{
			mResRead = true;//ReadFile completed
			CloseHandle(mOverlapRead.hEvent);//close read event handle
		}

		return true;//the read was checked
	}
	else//the port was not first initialized
		return false;
}



// THE FOLLOWING ARE FUNCTION DEFINITIONS USED IN THE CPORT CLASS

/*
HANDLE CreateFile(
  LPCTSTR lpFileName,          //pointer to name of the file
  DWORD dwDesiredAccess,       //access (read-write) mode
  DWORD dwShareMode,           //share mode
  LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                               //pointer to security attributes
  DWORD dwCreationDisposition, //how to create
  DWORD dwFlagsAndAttributes,  //file attributes
  HANDLE hTemplateFile         //handle to file with attributes to 
                               //copy
);
 
BOOL WriteFile(
  HANDLE hFile,                    //handle to file to write to
  LPCVOID lpBuffer,                //pointer to data to write to file
  DWORD nNumberOfBytesToWrite,     //number of bytes to write
  LPDWORD lpNumberOfBytesWritten,  //pointer to number of bytes written
  LPOVERLAPPED lpOverlapped        //pointer to structure for overlapped I/O
);

BOOL ReadFile(
  HANDLE hFile,                //handle of file to read
  LPVOID lpBuffer,             //pointer to buffer that receives data
  DWORD nNumberOfBytesToRead,  //number of bytes to read
  LPDWORD lpNumberOfBytesRead, //pointer to number of bytes read
  LPOVERLAPPED lpOverlapped    //pointer to structure for data
);

typedef struct _OVERLAPPED { 
    DWORD  Internal;     //Reserved for operating system use
    DWORD  InternalHigh; //Reserved for operating system use	
    DWORD  Offset;       //ignored with comm devices
    DWORD  OffsetHigh;   //ignored with comm devices
    HANDLE hEvent;       //Handle to an event set to the signaled state 
			       //when the transfer has been completed. The 
			       //calling process sets this member before calling 
			       //the ReadFile, WriteFile functions
} OVERLAPPED; 
 
HANDLE CreateEvent(
  LPSECURITY_ATTRIBUTES lpEventAttributes,
  BOOL bManualReset,
  BOOL bInitialState,
  LPCTSTR lpName
);

BOOL GetOverlappedResult(
  HANDLE hFile,                       //handle to file, pipe, or comm device
  LPOVERLAPPED lpOverlapped,          //pointer to overlapped structure
  LPDWORD lpNumberOfBytesTransferred, //pointer to actual bytes count
  BOOL bWait                          //wait flag
);
*/