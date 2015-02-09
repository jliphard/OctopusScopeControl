/*****************************************************************************
 Octopus microscope control software
 
 Copyright (C) 2004-2015 Jan Liphardt (jan.liphardt@stanford.edu)
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License along
 with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 ******************************************************************************/

#include "stdafx.h"
#include "ftd2xx.h"
#include "Octopus.h"
#include "OctopusDoc.h"
#include "OctopusView.h"
#include "OctopusGlobals.h"
#include "OctopusShutter.h"

extern COctopusGlobals B;

COctopusShutterAndWheel::COctopusShutterAndWheel(CWnd* pParent)
	: CDialog(COctopusShutterAndWheel::IDD, pParent)
{    

	USB_ready               =  false;
	m_ShutterOpen			=  2; // 2 means that the shutter is closed
	board_present			=  0;
	B.filter_wheel          =  0;
	ND_value                =  0;

	if( Create(COctopusShutterAndWheel::IDD, pParent) ) 
		ShowWindow( SW_SHOW );

}

void COctopusShutterAndWheel::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX,	IDC_SHUTTER_SLIDER,				m_Slider);
	DDX_Control(pDX,	IDC_SHUTTER_SLIDER_SETTING,		m_Slider_Setting);
	DDX_Radio(pDX,		IDC_RADIO_FILTER_0,				m_Filter);
}  

BEGIN_MESSAGE_MAP(COctopusShutterAndWheel, CDialog)
	ON_BN_CLICKED(IDC_RADIO_FILTER_0,			OnRadioFilter0)
	ON_BN_CLICKED(IDC_RADIO_FILTER_1,			OnRadioFilter1)
	ON_BN_CLICKED(IDC_RADIO_FILTER_2,			OnRadioFilter2)
	ON_BN_CLICKED(IDC_RADIO_FILTER_3,			OnRadioFilter3)
	ON_BN_CLICKED(IDC_RADIO_FILTER_4,			OnRadioFilter4)
	ON_BN_CLICKED(IDC_RADIO_FILTER_5,			OnRadioFilter5)
	ON_BN_CLICKED(IDC_RADIO_FILTER_6,			OnRadioFilter6)
	ON_BN_CLICKED(IDC_RADIO_FILTER_7,			OnRadioFilter7)
	ON_BN_CLICKED(IDC_RADIO_FILTER_8,			OnRadioFilter8)
	ON_BN_CLICKED(IDC_RADIO_FILTER_9,			OnRadioFilter9)
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_SHUTTER_SLIDER, OnNMCustomdrawExecute)
END_MESSAGE_MAP()

BOOL COctopusShutterAndWheel::OnInitDialog() 
{
	CDialog::OnInitDialog();	
	
	Loadem();									  // load the wheel
	Filter( 2 );								  // set the filter to the default
	ShutterClose();								  // close the shutter
	  
	SetWindowPos(NULL, 696, 57, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	m_Slider.SetRange(1, 143);
	m_Slider.SetPos( (int) ND_value );
	m_Slider.SetTicFreq(10);
    
	return TRUE;
}

COctopusShutterAndWheel::~COctopusShutterAndWheel() 
{
	FT_Close( m_ftHandle );
}

/***************************************************************************/
/***************************************************************************/

void COctopusShutterAndWheel::Loadem()
{
	unsigned char txbuf[25], rxbuf[25];
	DWORD ret_bytes;
	FT_STATUS status;

	//FT_OPEN_BY_DESCRIPTION
	status = FT_OpenEx(_T("Sutter Instrument Lambda 10B-XL"), FT_OPEN_BY_DESCRIPTION, &m_ftHandle);

	if( status != FT_OK )
	{
		AfxMessageBox(_T("Could not find shutter!"));
	}
	else
	{
		//CString handle;
		//handle.Format("Handle is: %x\nStatus is: %d\n", m_ftHandle, status);
		//AfxMessageBox(handle);
	}

	FT_SetTimeouts( m_ftHandle, 100, 100);
	FT_Close( m_ftHandle );

	status = FT_OpenEx("Sutter Instrument Lambda 10B-XL", FT_OPEN_BY_DESCRIPTION, &m_ftHandle);

	if( status != FT_OK )
	{
		AfxMessageBox(_T("Could not find shutter!"));
	}
	else
	{
		//CString handle;
		//handle.Format("Handle is: %x\nStatus is: %d\n", m_ftHandle, status);
		//AfxMessageBox(handle);
	}

	FT_Purge(m_ftHandle, FT_PURGE_RX || FT_PURGE_TX);
	FT_SetLatencyTimer(m_ftHandle, 2);
	FT_SetBaudRate(m_ftHandle,128000);
	FT_SetUSBParameters(m_ftHandle, 64,0);

	Sleep(150);

	txbuf[0] = 0xEE;

	FT_Write(m_ftHandle,txbuf, 1, &ret_bytes);
	FT_Read(m_ftHandle,rxbuf, 2, &ret_bytes);
	
	Sleep(150);

	if(ret_bytes == 0) FT_Read(m_ftHandle,rxbuf, 2, &ret_bytes);
		
	if(rxbuf[0] != 0xEE)
	{
		USB_ready = false;
		board_present = 0;
		B.load_wheel_failed = true;
		AfxMessageBox("Sorry...");
		FT_Close( m_ftHandle );
	}	
	else
	{
		USB_ready = true;
		B.load_wheel_failed = false;
		board_present = 1;
		FT_SetTimeouts(m_ftHandle, 100, 100);
	}

	UpdateData(FALSE);
}

/***************************************************************************/
/***************************************************************************/

void COctopusShutterAndWheel::ShutterOpen() 
{
	DWORD ret_bytes;
	unsigned char txbuf[25];
	u8 val = 143;

	if( USB_ready )
	{
		txbuf[0] = 0xDE;			// ND mode
		txbuf[1] = val;				// value
		txbuf[2] = 0xAA;			// open
		FT_Write(m_ftHandle,txbuf, 3, &ret_bytes);
		B.nd_setting = val;
		UpdateShutterVal();
	}

	FT_SetTimeouts(m_ftHandle,100, 100);
}

void COctopusShutterAndWheel::ShutterPartial( u8 val ) 
{
	DWORD ret_bytes;
	char txbuf[3];
	
	//this goes from 1 to 144 - no idea why
	if( USB_ready )
	{
		txbuf[0] = 0xDE;			// ND mode
		txbuf[1] = (char) val;		// value
		txbuf[2] = 0xAA;			// open
		FT_Write(m_ftHandle,txbuf, 3, &ret_bytes);
		B.nd_setting = val;
		UpdateShutterVal();
	}

	FT_SetTimeouts(m_ftHandle,100, 100);
}

void COctopusShutterAndWheel::ShutterClose() 
{
	DWORD ret_bytes;
	unsigned char txbuf[25];
	u8 val = 1;

	if( USB_ready )
	{
		txbuf[0] = 0xDE;			// ND mode
		txbuf[1] = val;				// value
		txbuf[2] = 0xAA;			// open
		FT_Write(m_ftHandle,txbuf, 3, &ret_bytes);
		B.nd_setting = val;
		UpdateShutterVal();
	}

	FT_SetTimeouts(m_ftHandle, 100, 100);
}

//		txbuf[0] = 0xDE;			// ND mode
//		txbuf[1] = (char) val;		// value
//		txbuf[2] = 0xAA;			// open
//		txbuf[0] = 0xDC;
//		FT_Write(m_ftHandle,txbuf, 1, &ret_bytes);
//		txbuf[0] = 0xAA;
//		FT_Write(m_ftHandle,txbuf, 1, &ret_bytes);
//		B.nd_setting = 144;
//		UpdateShutterVal();


void COctopusShutterAndWheel::Filter( u8 filter ) 
{
	
	unsigned char txbuf[25];
	DWORD ret_bytes;

	if( USB_ready )
	{
		B.filter_wheel = filter;
		m_Filter = filter;
		u8 speed = 6;
		txbuf[0] = speed * 16 + filter;
		FT_Write(m_ftHandle,txbuf, 1, &ret_bytes);
	}

	FT_SetTimeouts(m_ftHandle,100, 100);
	UpdateData(FALSE);
}

void COctopusShutterAndWheel::OnRadioFilter0() { Filter ( 0 ); }
void COctopusShutterAndWheel::OnRadioFilter1() { Filter ( 1 ); }
void COctopusShutterAndWheel::OnRadioFilter2() { Filter ( 2 ); }
void COctopusShutterAndWheel::OnRadioFilter3() { Filter ( 3 ); }
void COctopusShutterAndWheel::OnRadioFilter4() { Filter ( 4 ); }
void COctopusShutterAndWheel::OnRadioFilter5() { Filter ( 5 ); }
void COctopusShutterAndWheel::OnRadioFilter6() { Filter ( 6 ); }
void COctopusShutterAndWheel::OnRadioFilter7() { Filter ( 7 ); }
void COctopusShutterAndWheel::OnRadioFilter8() { Filter ( 8 ); }
void COctopusShutterAndWheel::OnRadioFilter9() { Filter ( 9 ); }

bool COctopusShutterAndWheel::ShutterReady( void ) 
{
	
	DWORD bytes_in_buf;
	FT_STATUS status;
	
	status = FT_GetQueueStatus(m_ftHandle,&bytes_in_buf);
	
	if( status == FT_OK ) 
		return true;
	else 
		return false;
}

BOOL COctopusShutterAndWheel::OnCommand(WPARAM wParam, LPARAM lParam) {
	int id = LOWORD(wParam);     // Notification code
	if( id == 2 ) return FALSE;  // Trap ESC key
	if( id == 1 ) return FALSE;  // Trap RTN key
    return CDialog::OnCommand(wParam, lParam);
}

void COctopusShutterAndWheel::OnNMCustomdrawExecute( NMHDR *pNMHDR, LRESULT *pResult )
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	
	int CurPos = m_Slider.GetPos();

	ShutterPartial( CurPos );

	*pResult = 0;	
}

void COctopusShutterAndWheel::UpdateShutterVal( void ) 
{

	CString str;
	str.Format(_T("Shutter (0-144): %d"), B.nd_setting);
	
	if( IsWindowVisible() )
	{
		m_Slider_Setting.SetWindowText( str );
		m_Slider.SetPos( (int) B.nd_setting );
		UpdateData(FALSE);
	}
}
