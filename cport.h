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
 - cport.h                                                                   -
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
**** File: cport.h                                                         ****
**** Requirements: none.                                                   ****
**** Purpose: this file defines the CPort class to be used with a serial   ****
****  communication port. it enables the user to open and close a comm     ****
****  port, to write to and read from the comm port (in strings or one byte****
****  at a time), and to check for the completion of a write or read (in   ****
****  strings or one byte at a time).                                      ****
**** Initial Version: Aug. 10, 2005                                        ****
**** Programmer: Michelle Kuykendal                                        ****
*******************************************************************************
******************************************************************************/


#if !defined(AFX_H_CPORT_H)
#define AFX_H_CPORT_H

#include "stdafx.h"
#include "afxwin.h"
#include <process.h>

class CPort
{
public:
	
//	 wchar_t buf[100];
//   int len = swprintf( buf, 100, L"%s", L"Hello world" );
	
	
	char mOutBuf[100];     //output buffer used in WriteCPort
	char mInBuf[500];      //input buffer used in ReadCPort
	CString mPort; //[5];     //comm port name (i.e. "COM8")
	bool mResWrite;        //write result checker
	bool mResRead;         //read result checker
	DWORD mBytesToRead;    //user defined number of bytes to read
	DWORD mBytesWritten;   //number of bytes returned as written to port
	DWORD mBytesRead;      //number of bytes returned as read from port
	unsigned char mOutOne; //output byte used in WriteCPortOneByte
	unsigned char mInOne;  //input byte used in ReadCPortOneByte

	//REQUIREMENTS: none.
	//USE: CPort() is the constructor for the port class.
	//RETURN VALUE: none.
	CPort();

	//REQUIREMENTS: none.
	//USE: ~CPort() is the destructor for the port class, which will close 
	// the comm port handle in case of improper function closure.
	//RETURN VALUE: none.
	~CPort();
	
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
	bool OpenCPort();
	
	//REQUIREMENTS: a call to OpenCPort() should be made prior to this call
	// for proper execution.
	//USE: CloseCPort() will close the handle to the comm port opened using
	// the OpenCPort() function.
	//RETURN VALUE: the return value will be false if the comm port handle 
	// is not closed. it will be true if the comm port handle is closed.
	bool CloseCPort();
	
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
	bool WriteCPort();

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
	bool WriteCPortOneByte();

	//REQUIREMENT: a call to OpenCPort() and WriteCPort() should be made prior
	// to this call for proper execution.
	//USE: CheckWrite() will check to see if write was successfully completed.
	// if it is completed, mResWrite will be assigned true, otherwise it 
	// will remain false.
	//RETURN VALUE: the return will be false if the port is not initialized. 
	// the return will be true if the check for completion was successful 
	// regardless of the status of the write completion.
	bool CheckWrite();

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
	bool ReadCPort();

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
	bool ReadCPortOneByte();

	//REQUIREMENT: a call to OpenCPort() and ReadCPort() should be made prior
	// to this call for proper execution.
	//USE: CheckRead() will check to see if read was successfully completed.
	// if it is completed, mResRead will be assigned true, otherwise it 
	// will remain false.
	//RETURN VALUE: the return will be false if the port is not initialized. 
	// the return will be true if the check for completion was successful 
	// regardless of the status of the read completion.
	bool CheckRead();

private:
	HANDLE mComm;             //handle to comm port
	DCB mDCB;                 //device control block for comm params
	OVERLAPPED mOverlapWrite; //structure for write completion monitoring
	OVERLAPPED mOverlapRead;  //structure for read completion monitoring
	COMMTIMEOUTS mTimeouts;   //structure for declaring timeout values
	bool portIsInit;          //initialization checker for port
};


#endif
