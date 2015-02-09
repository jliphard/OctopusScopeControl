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
#include "Octopus.h"
#include "OctopusSamplePiezo.h"
#include "OctopusLog.h"

extern COctopusGlobals B;
extern COctopusLog* glob_m_pLog;

COctopusSamplePiezo::COctopusSamplePiezo(CWnd* pParent)
	: CDialog(COctopusSamplePiezo::IDD, pParent)
{    
	if( Create(COctopusSamplePiezo::IDD, pParent) ) 
		ShowWindow( SW_SHOW );

	B.position_z_sample         =   0.0;
	stepsize_z_microns          =   1.0;
	max_z_microns               = 200.0;

	middle_z_microns            = 100.0;
	target_z_microns			= 100.0;
	save_z_microns				= 100.0;

	first_tick                  = true;
	initialized					= false;
	connected					= false;

	SetTimer( TIMER_PIEZO, 100, NULL );
}

BOOL COctopusSamplePiezo::OnInitDialog()
{
     CDialog::OnInitDialog();
     SetWindowPos(NULL,696, 57, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
     return TRUE;
}

COctopusSamplePiezo::~COctopusSamplePiezo() {}

int COctopusSamplePiezo::Initialize( void ) 
{
	const char* piezo_string = "0111015130";

	//const char* piezo_string = "110010341";
    //used to be PI_

	ID = PI_ConnectUSB( piezo_string );

	if (ID < 0)
	{
		AfxMessageBox(_T(" PI Z stage not connected "));
	} 
	else
	{
		WriteLogString(_T(" PI Z stage connected "));		
		connected = true;
	}

   // Get the name of the connected axis 
   if ( !PI_qSAI(ID, axis, 16) )
   {
		AfxMessageBox(_T(" PI Z stage could not get name "));
   }

	/////////////////////////////////////////
	// close the servo loop (closed-loop). //
	/////////////////////////////////////////

	// Switch on the Servo for all axes
	// Servo on for first axis in the string 'axes'.
	BOOL bFlags[1];	
    bFlags[0] = TRUE; 

	// call the SerVO mode command.
	if(!PI_SVO(ID, axis, bFlags))
	{
		WriteLogString(_T(" PI Z stage SVO failed "));
		initialized = false;
		PI_CloseConnection(ID);
		AfxMessageBox(_T(" PI Z stage SVO failed "));
		return 1;
	}

	initialized = true;
	return 0;

}

void COctopusSamplePiezo::Close( void ) 
{ 
    KillTimer( TIMER_PIEZO );

	Sleep(500);

	initialized = false; 

	if ( connected )
	{
		PI_CloseConnection( ID );
		connected = false;
	}

	WriteLogString(_T(" PI Z stage Close() "));

}

void COctopusSamplePiezo::MoveRelZ( double dist )
{
	BOOL bIsMoving[1];
	bIsMoving[0] = TRUE;
	
	if ( connected )
	{
		PI_IsMoving(ID, NULL, bIsMoving);

		if ( bIsMoving[0] == FALSE ) 
		{
			double dPos[1];

			double new_target = target_z_microns + dist;

			if( new_target > 0.0 && new_target < max_z_microns ) 
			{
				target_z_microns = new_target;
				dPos[0]  = target_z_microns;
			
				if(!PI_MOV(ID, axis, dPos))
				{
					WriteLogString(_T(" PI 736 stage MoveRelZ() failed "));
				}
				else
				{
					debug.Format(_T(" PI 736 stage MoveRelZ() to %.3f "), target_z_microns);
					WriteLogString(debug);
				}
			}
		}
	UpdatePosition();
	}		
}

void COctopusSamplePiezo::MoveToZ( double z )
{
	BOOL bIsMoving[1];
	bIsMoving[0] = TRUE;
	
	if ( connected )
	{
		PI_IsMoving(ID, NULL, bIsMoving);

		if ( bIsMoving[0] == FALSE ) 
		{
			double dPos[1];

			double new_target = z;

			if( new_target > 0.0 && new_target < max_z_microns ) 
			{
				target_z_microns = new_target;
				dPos[0]  = target_z_microns;
			
				if(!PI_MOV(ID, axis, dPos))
				{
					WriteLogString(_T(" PI 736 stage MoveRelZ() failed "));
				}
				else
				{
					debug.Format(_T(" PI 736 stage MoveRelZ() to %.3f "), target_z_microns);
					WriteLogString(debug);
				}
			}
		}
	UpdatePosition();
	}		
}

void COctopusSamplePiezo::MoveUp(   void ) { MoveRelZ( +stepsize_z_microns ); }
void COctopusSamplePiezo::MoveDown( void ) { MoveRelZ( -stepsize_z_microns ); }

void COctopusSamplePiezo::OnStepSize1() { m_Radio_S = 0; stepsize_z_microns = 0.005; UpdateData (false); };
void COctopusSamplePiezo::OnStepSize2() { m_Radio_S = 1; stepsize_z_microns = 0.050; UpdateData (false); };
void COctopusSamplePiezo::OnStepSize3() { m_Radio_S = 2; stepsize_z_microns = 0.500; UpdateData (false); };
void COctopusSamplePiezo::OnStepSize4() { m_Radio_S = 3; stepsize_z_microns = 1.000; UpdateData (false); };
void COctopusSamplePiezo::OnStepSize5() { m_Radio_S = 4; stepsize_z_microns = 5.000; UpdateData (false); };

double COctopusSamplePiezo::GetPosition( void ) 
{ 
	//this function is just for completeness, if you do not like globals!
	UpdatePosition(); 
	return B.position_z_sample;
}

void COctopusSamplePiezo::UpdatePosition( void ) 
{ 
	double dPos[1];

	dPos[0] = 0.0; 
	
	BOOL bIsMoving[1];
	bIsMoving[0] = TRUE;

	if ( connected )
	{
		PI_IsMoving(ID, NULL, bIsMoving);
		
		if ( bIsMoving[0] == FALSE ) 
		{
			PI_qPOS(ID, axis, dPos);
		}
	}

	//this gets saved to file
	B.position_z_sample = dPos[0];
}

void COctopusSamplePiezo::ShowPosition( void )
{
	CString str;

	if( IsWindowVisible() ) 
	{
	    str.Format(_T("Z:%.3f\nSave Z:%.3f"), B.position_z_sample, save_z_microns);
		m_Pos.SetWindowText( str );
	}
}

void COctopusSamplePiezo::Center( void ) 
{	
	target_z_microns = middle_z_microns;

	if ( connected ) 
	{
		double dPos[1];
		dPos[0] = target_z_microns;

		if(!PI_MOV(ID, axis, dPos))
			WriteLogString(_T(" PI 736 stage Center() failed "));
		else
			WriteLogString(_T(" PI 736 stage Center() "));
	}
}

void COctopusSamplePiezo::OnSave( void ) 
{	
	save_z_microns = GetPosition();
	WriteLogString(_T(" COctopusSamplePiezo::OnSave( void ) "));
}

void COctopusSamplePiezo::OnSaveGoTo( void ) 
{	
	target_z_microns = save_z_microns;

	if ( connected ) {

		double dPos[1];
		dPos[0] = target_z_microns;

		if(!PI_MOV(ID, axis, dPos)) 
			WriteLogString(_T(" PI 736 stage OnSaveGoTo() failed "));
		else
			WriteLogString(_T(" PI 736 stage OnSaveGoTo() "));
	}
}

void COctopusSamplePiezo::OnTimer( UINT nIDEvent ) 
{

	if( nIDEvent == TIMER_PIEZO ) 
	{
		if( first_tick ) 
		{
			OnStepSize4();
			first_tick = false;
			Initialize(); 
			MoveToZ( 100.0 );//Center();
			WriteLogString(_T(" PI 736 stage initialized "));
		}
		if( initialized ) 
		{
			UpdatePosition();
			ShowPosition();
		}
	}

	CDialog::OnTimer(nIDEvent);
}

void COctopusSamplePiezo::DoDataExchange(CDataExchange* pDX) 
{
	CDialog::DoDataExchange( pDX );
	DDX_Radio(pDX,	 IDC_PIEZO_SSIZE_1,	m_Radio_S);
	DDX_Control(pDX, IDC_PIEZO_POS,	    m_Pos);
}  

BEGIN_MESSAGE_MAP(COctopusSamplePiezo, CDialog)
	ON_BN_CLICKED(IDC_PIEZO_UP,	         MoveUp)
	ON_BN_CLICKED(IDC_PIEZO_DOWN,        MoveDown)
	ON_BN_CLICKED(IDC_PIEZO_SAVE,	     OnSave)
	ON_BN_CLICKED(IDC_PIEZO_SAVE_GOTO,   OnSaveGoTo)
	ON_BN_CLICKED(IDC_PIEZO_CENTER,		 Center)
	ON_BN_CLICKED(IDC_PIEZO_SSIZE_1,     OnStepSize1)
	ON_BN_CLICKED(IDC_PIEZO_SSIZE_2,     OnStepSize2)
	ON_BN_CLICKED(IDC_PIEZO_SSIZE_3,     OnStepSize3)
	ON_BN_CLICKED(IDC_PIEZO_SSIZE_4,     OnStepSize4)
	ON_BN_CLICKED(IDC_PIEZO_SSIZE_5,     OnStepSize5)
	ON_WM_TIMER()
END_MESSAGE_MAP()

void COctopusSamplePiezo::OnKillfocusGeneral() { UpdateData( true ); }

BOOL COctopusSamplePiezo::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	int id = LOWORD(wParam);     // Notification code
	if( id == 2 ) return FALSE;  // Trap ESC key
	if( id == 1 ) return FALSE;  // Trap RTN key
    return CDialog::OnCommand(wParam, lParam);
}

void COctopusSamplePiezo::WriteLogString( CString logentry )
{
		//debug.Format(_T(" PI 736 stage OnSaveGoTo() "));
		if ( glob_m_pLog != NULL ) glob_m_pLog->Write( logentry );
}