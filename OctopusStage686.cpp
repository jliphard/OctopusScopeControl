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
#include "OctopusStage686.h"
#include "OctopusCameraDisplay.h"
#include "OctopusCameraDlg.h"
#include "OctopusScope.h"
#include "OctopusLog.h"
#include "Pi_gcs2_dll.h"
#include "OctopusMultifunction.h"
#include "OctopusSamplePiezo.h"
#include "OctopusLasers.h"

extern COctopusGlobals B;

extern COctopusLog*             glob_m_pLog;
extern COctopusScope*           glob_m_pScope;
extern COctopusSamplePiezo*     glob_m_pSamplePiezo;
extern COctopusMultifunction*   glob_m_pNI;
extern COctopusPictureDisplay*  glob_m_pPictureDisplay;
extern COctopusCamera*          glob_m_pCamera;
extern COctopusLasers*          glob_m_pLasers;

DWORD WINAPI MonitorPosition686( LPVOID lpParameter );

UINT ThreadCell(  LPVOID lpParameter );
UINT ThreadManyCells(  LPVOID lpParameter );

COctopusStage686::COctopusStage686(CWnd* pParent)
	: CDialog(COctopusStage686::IDD, pParent)
{    

	B.position_x_686 = 0.0;
	B.position_y_686 = 0.0;

	initialized      = false;
	XY_connected	 = false;
	bIsMoving[0]     = FALSE;
	bIsMoving[1]     = FALSE;

	middle			 = 12.5;
	range			 = 10.0;
	
	save_xy[0]		 = middle;
	save_xy[1]		 = middle;
	
	target_xy[0]	 = middle;
	target_xy[1]	 = middle;

	B.AUTO_CellLabel = 1000;
	
	velocity_nor     = 3.0;

	old_x            = 12.5;
	old_y            = 12.5;
	old_z            = 100.0;
	
	emergencySTOP    = false;

	if( Create(COctopusStage686::IDD, pParent) ) 
		ShowWindow( SW_SHOW );
}

BOOL COctopusStage686::OnInitDialog()
{
    CDialog::OnInitDialog();

	//m_Font.CreatePointFont(80, _T("MS Shell Dlg"));

	Cbox_Laser1.SetCheck( 1 );
	Cbox_Laser2.SetCheck( 1 );
	Cbox_Laser3.SetCheck( 0 );
	Cbox_Laser4.SetCheck( 1 );
	Cbox_Brightfield.SetCheck( 1 );

	CString fw1("1 TIRF 4");
	CString fw2("2 DAPI");
	CString fw3("3 GFP");
	CString fw4("4 Cal590");
	CString fw5("5 Cy5");
	CString fw6("6 IRFP/AI649");

	Las1_FW_Choice.AddString(fw1);
	Las1_FW_Choice.AddString(fw2);
	Las1_FW_Choice.AddString(fw3);
	Las1_FW_Choice.AddString(fw4);
	Las1_FW_Choice.AddString(fw5);
	Las1_FW_Choice.AddString(fw6);

	Las2_FW_Choice.AddString(fw1);
	Las2_FW_Choice.AddString(fw2);
	Las2_FW_Choice.AddString(fw3);
	Las2_FW_Choice.AddString(fw4);
	Las2_FW_Choice.AddString(fw5);
	Las2_FW_Choice.AddString(fw6);

	Las3_FW_Choice.AddString(fw1);
	Las3_FW_Choice.AddString(fw2);
	Las3_FW_Choice.AddString(fw3);
	Las3_FW_Choice.AddString(fw4);
	Las3_FW_Choice.AddString(fw5);
	Las3_FW_Choice.AddString(fw6);

	Las4_FW_Choice.AddString(fw1);
	Las4_FW_Choice.AddString(fw2);
	Las4_FW_Choice.AddString(fw3);
	Las4_FW_Choice.AddString(fw4);
	Las4_FW_Choice.AddString(fw5);
	Las4_FW_Choice.AddString(fw6);

	Las5_FW_Choice.AddString(fw1);
	Las5_FW_Choice.AddString(fw2);
	Las5_FW_Choice.AddString(fw3);
	Las5_FW_Choice.AddString(fw4);
	Las5_FW_Choice.AddString(fw5);
	Las5_FW_Choice.AddString(fw6);

	Las1_FW_Choice.SetCurSel(4); //this is zero indexed!
	Las2_FW_Choice.SetCurSel(3);
	Las3_FW_Choice.SetCurSel(2);
	Las4_FW_Choice.SetCurSel(1);
	Las5_FW_Choice.SetCurSel(0);

	//639
	Las1_ExpTime = 50;
	Las1_Gain    = 50;

	//561
	Las2_ExpTime = 50;
	Las2_Gain    = 200;

	//488
	Las3_ExpTime = 50;
	Las3_Gain    = 200;

	//405
	Las4_ExpTime = 50;
	Las4_Gain    = 50;

	Las5_ExpTime = 50;
	Las5_Gain    =  1;

	AutoStepNum  = 5;

	AutoStepSize = 1000;

	OnStepSize3();

	InitializeStage(); 
	
	StageCenter();

	wait_thread			= NULL;
	thread_count		= 0;
	wait_thread_running = true;
	DWORD wait_thread_id;
	wait_thread			= CreateThread( 0, 4096, MonitorPosition686, (LPVOID)this, 0, &wait_thread_id );

	if ( glob_m_pLog != NULL ) 
		glob_m_pLog->Write(" PI 686 stage initialized ");

	Cell_List.ResetContent();

	//AfxMessageBox("Are you absolutely SURE that the laser is in low power mode?\n1 mistake = all new optics in the beam path!");
	return TRUE;
}

void COctopusStage686::DoDataExchange(CDataExchange* pDX) 
{
	CDialog::DoDataExchange( pDX );

	DDX_Radio(pDX,		IDC_STAGEm_SSIZE_1,	 m_Radio_S);
	DDX_Control(pDX,	IDC_STAGEm_POS,		 m_Pos);
	DDX_Control(pDX,	IDC_STAGEm_POS_SAVE, m_Pos_Save);

	DDX_Text( pDX, IDC_STAGEm_Vel, velocity_nor);
	DDV_MinMaxDouble(pDX, velocity_nor, 0.01, 10.0);

	DDX_Control(pDX, IDC_STAGEm_CELL_LIST, Cell_List);
	
	DDX_Control(pDX, IDC_STAGEm_AUTO_CHECK_LAS1, Cbox_Laser1);
	DDX_Control(pDX, IDC_STAGEm_AUTO_CHECK_LAS2, Cbox_Laser2);
	DDX_Control(pDX, IDC_STAGEm_AUTO_CHECK_LAS3, Cbox_Laser3);
	DDX_Control(pDX, IDC_STAGEm_AUTO_CHECK_LAS4, Cbox_Laser4);
	DDX_Control(pDX, IDC_STAGEm_AUTO_CHECK_LAS5, Cbox_Brightfield);

	DDX_Control(pDX, IDC_IDC_STAGEm_AUTO_FW_LAS1, Las1_FW_Choice);
	DDX_Control(pDX, IDC_IDC_STAGEm_AUTO_FW_LAS2, Las2_FW_Choice);
	DDX_Control(pDX, IDC_IDC_STAGEm_AUTO_FW_LAS3, Las3_FW_Choice);
	DDX_Control(pDX, IDC_IDC_STAGEm_AUTO_FW_LAS4, Las4_FW_Choice);
	DDX_Control(pDX, IDC_IDC_STAGEm_AUTO_FW_LAS5, Las5_FW_Choice);

	DDX_Text( pDX, IDC_STAGEm_AUTO_ExpTime1, Las1_ExpTime);
	DDV_MinMaxInt(pDX, Las1_ExpTime, 1, 1000);
	DDX_Text( pDX, IDC_STAGEm_AUTO_Gain1, Las1_Gain);
	DDV_MinMaxInt(pDX, Las1_Gain, 1, 950);

	DDX_Text( pDX, IDC_STAGEm_AUTO_ExpTime2, Las2_ExpTime);
	DDV_MinMaxInt(pDX, Las2_ExpTime, 1, 1000);
	DDX_Text( pDX, IDC_STAGEm_AUTO_Gain2, Las2_Gain);
	DDV_MinMaxInt(pDX, Las2_Gain, 1, 950);

	DDX_Text( pDX, IDC_STAGEm_AUTO_ExpTime3, Las3_ExpTime);
	DDV_MinMaxInt(pDX, Las3_ExpTime, 1, 1000);
	DDX_Text( pDX, IDC_STAGEm_AUTO_Gain3, Las3_Gain);
	DDV_MinMaxInt(pDX, Las3_Gain, 1, 950);

	DDX_Text( pDX, IDC_STAGEm_AUTO_ExpTime4, Las4_ExpTime);
	DDV_MinMaxInt(pDX, Las4_ExpTime, 1, 1000);
	DDX_Text( pDX, IDC_STAGEm_AUTO_Gain4, Las4_Gain);
	DDV_MinMaxInt(pDX, Las4_Gain, 1, 950);

	DDX_Text( pDX, IDC_STAGEm_AUTO_ExpTime5, Las5_ExpTime);
	DDV_MinMaxInt(pDX, Las5_ExpTime, 1, 1000);
	DDX_Text( pDX, IDC_STAGEm_AUTO_Gain5, Las5_Gain);
	DDV_MinMaxInt(pDX, Las5_Gain, 1, 950);

	DDX_Text( pDX, IDC_STAGEm_AUTO_StepNum, AutoStepNum);
	DDV_MinMaxInt(pDX, AutoStepNum, 1, 20);

	DDX_Text( pDX, IDC_STAGEm_AUTO_StepSize, AutoStepSize);
	DDV_MinMaxInt(pDX, AutoStepSize, 1, 10000);

}  

BEGIN_MESSAGE_MAP(COctopusStage686, CDialog)
	ON_BN_CLICKED(IDC_STAGEm_RIGHT,	      MoveRight)
	ON_BN_CLICKED(IDC_STAGEm_LEFT,		  MoveLeft)
	ON_BN_CLICKED(IDC_STAGEm_BACK,	      MoveBack)
	ON_BN_CLICKED(IDC_STAGEm_FWD,		  MoveFwd)
	ON_BN_CLICKED(IDC_STAGEm_CENTER,	  StageCenter)
	ON_BN_CLICKED(IDC_STAGEm_SAVE,	      OnSave)
	ON_BN_CLICKED(IDC_STAGEm_SAVE_GOTO,   OnSaveGoTo)
	
	ON_BN_CLICKED(IDC_STAGEm_SSIZE_1,     OnStepSize1)
	ON_BN_CLICKED(IDC_STAGEm_SSIZE_2,     OnStepSize2)
	ON_BN_CLICKED(IDC_STAGEm_SSIZE_3,     OnStepSize3)
	ON_BN_CLICKED(IDC_STAGEm_SSIZE_4,     OnStepSize4)
	ON_BN_CLICKED(IDC_STAGEm_SSIZE_5,     OnStepSize5)

	ON_EN_KILLFOCUS(IDC_STAGEm_Vel,       OnKillfocusVelocityNor)

	ON_EN_KILLFOCUS(IDC_STAGEm_AUTO_ExpTime1,    OnKillfocusGeneral)
	ON_EN_KILLFOCUS(IDC_STAGEm_AUTO_Gain1,       OnKillfocusGeneral)
	ON_EN_KILLFOCUS(IDC_STAGEm_AUTO_ExpTime2,    OnKillfocusGeneral)
	ON_EN_KILLFOCUS(IDC_STAGEm_AUTO_Gain2,       OnKillfocusGeneral)
	ON_EN_KILLFOCUS(IDC_STAGEm_AUTO_ExpTime3,    OnKillfocusGeneral)
	ON_EN_KILLFOCUS(IDC_STAGEm_AUTO_Gain3,       OnKillfocusGeneral)
	ON_EN_KILLFOCUS(IDC_STAGEm_AUTO_ExpTime4,    OnKillfocusGeneral)
	ON_EN_KILLFOCUS(IDC_STAGEm_AUTO_Gain4,       OnKillfocusGeneral)
	ON_EN_KILLFOCUS(IDC_STAGEm_AUTO_ExpTime5,    OnKillfocusGeneral)
	ON_EN_KILLFOCUS(IDC_STAGEm_AUTO_Gain5,       OnKillfocusGeneral)

	ON_EN_KILLFOCUS(IDC_STAGEm_AUTO_StepNum,     OnKillfocusGeneral)
	ON_EN_KILLFOCUS(IDC_STAGEm_AUTO_StepSize,    OnKillfocusGeneral)

	ON_BN_CLICKED(IDC_STAGEm_JONOFF,		  OnJoyStickOnOff)
	//ON_BN_CLICKED(IDC_STAGEm_ADD,		  WritePos)
	//ON_BN_CLICKED(IDC_STAGEm_CF,		  Close_The_File)

	ON_BN_CLICKED(IDC_STAGEm_IMAGE_ONE_CELL,        ImageOneCell)
	ON_BN_CLICKED(IDC_STAGEm_IMAGE_MANY_CELLS,      ImageManyCells)
	ON_BN_CLICKED(IDC_STAGEm_IMAGE_STOP,            ImageStop)
	ON_BN_CLICKED(IDC_STAGEm_REMOVE_CELL,           CellRemove)	
END_MESSAGE_MAP()

COctopusStage686::~COctopusStage686() 
{
	Close();

	//if ( pFileHead != NULL ) 
	//{
	//	fclose( pFileHead );
	//	pFileHead = NULL;
	//}
}

void COctopusStage686::Close( void ) 
{ 

	initialized = false; 
	
	if( !wait_thread_running ) return;
	wait_thread_running = false;
	WaitForSingleObject(wait_thread, 500);

	if ( XY_connected )
	{
		PI_CloseConnection( ID_XY );
		XY_connected = false;
	}

	debug.Format(_T(" PI 686 stage closed "));
	if ( glob_m_pLog != NULL ) glob_m_pLog->Write(debug);
}

void COctopusStage686::InitializeStage( void ) 
{

   const char* XY_string = "0111017919"; 
   //const char* XY_string = "0111005018"; //"0111017919"; 
   
   ID_XY = PI_ConnectUSB( XY_string );

   if (ID_XY < 0)
   {
		AfxMessageBox(_T("PI XY stage not connected"));
   } 
   else
   {
		debug.Format(_T(" PI coarse stage XY connected "));
		if ( glob_m_pLog != NULL ) glob_m_pLog->Write(debug);
		XY_connected = true;
   }

   // Get the name of the connected axis 
   if ( !PI_qSAI(ID_XY, axis_xy, 6) )
   {
		AfxMessageBox(_T("PI stage XY could not get name"));
   }

    unsigned int p_arr[2];
	double pd_arr[2];
	char sz[8];


    /*Near parameters, P1*/
	//P-term for Set 1
	p_arr[0]  = 0x411;
	p_arr[1]  = 0x411;
	pd_arr[0] =    50;
	pd_arr[1] =    50;
	PI_SPA(ID_XY, axis_xy, p_arr, pd_arr, sz);
	
	p_arr[0]  = 0x412;
	p_arr[1]  = 0x412;
	pd_arr[0] =   200;
	pd_arr[1] =   200;
	PI_SPA(ID_XY, axis_xy, p_arr, pd_arr, sz);

	p_arr[0]  = 0x413;
	p_arr[1]  = 0x413;
	pd_arr[0] =   500;
	pd_arr[1] =   500;
	PI_SPA(ID_XY, axis_xy, p_arr, pd_arr, sz);
	/*end near range parameters, P1*/

	/*long range parameters, P2*/
	//P-term for Set 2
	p_arr[0]  = 0x421;
	p_arr[1]  = 0x421;
	pd_arr[0] =   300;
	pd_arr[1] =   300;
	PI_SPA(ID_XY, axis_xy, p_arr, pd_arr, sz);
	
	p_arr[0]  = 0x422;
	p_arr[1]  = 0x422;
	pd_arr[0] =    20;
	pd_arr[1] =    40;
	PI_SPA(ID_XY, axis_xy, p_arr, pd_arr, sz);

	p_arr[0]  = 0x423;
	p_arr[1]  = 0x423;
	pd_arr[0] =   450;
	pd_arr[1] =   200;
	PI_SPA(ID_XY, axis_xy, p_arr, pd_arr, sz);
	/*end long range parameters, P2*/

	/*settle 0*/
	p_arr[0]  = 0x406; //enter 0
	p_arr[1]  = 0x406;
	pd_arr[0] =  1;
	pd_arr[1] =  1;
	PI_SPA(ID_XY, axis_xy, p_arr, pd_arr, sz);
	
	p_arr[0]  = 0x407;
	p_arr[1]  = 0x407;
	pd_arr[0] =  3;   //exit 0
	pd_arr[1] =  3;
	PI_SPA(ID_XY, axis_xy, p_arr, pd_arr, sz);

	/*settle 1*/
	p_arr[0]  = 0x416;
	p_arr[1]  = 0x416;
	pd_arr[0] =  50; //enter 1
	pd_arr[1] =  50;
	PI_SPA(ID_XY, axis_xy, p_arr, pd_arr, sz);
	
	p_arr[0]  = 0x417;
	p_arr[1]  = 0x417;
	pd_arr[0] = 100; //exit 1
	pd_arr[1] = 100;
	PI_SPA(ID_XY, axis_xy, p_arr, pd_arr, sz);

	/*minimum motor voltage*/
	p_arr[0]  = 0x33;
	p_arr[1]  = 0x33;
	pd_arr[0] = 2500;
	pd_arr[1] = 2500;
	PI_SPA(ID_XY, axis_xy, p_arr, pd_arr, sz);
	
	p_arr[0]  = 0x34;
	p_arr[1]  = 0x34;
	pd_arr[0] = 2500;
	pd_arr[1] = 2500;
	PI_SPA(ID_XY, axis_xy, p_arr, pd_arr, sz);

   SetVelocity( 2.0 ); //otherwise initialisation takes forever

   // Switch on the Servo
   BOOL bFlag = TRUE;
   if( !PI_SVO(ID_XY, axis_xy, &bFlag ) )
   {
		AfxMessageBox(_T("PI stage XY closed loop could not switch on"));
   }

   // Wait until done.
   bFlag = FALSE;
   
   while( bFlag != TRUE )
   {
	   PI_IsControllerReady(ID_XY, &bFlag);
   }

   //turn off joystick mode in case we are in it!
    if( IsJoyStickOn() ) 
		TurnJoyStickOff();

   // Reference the axis using the reference switch
   if( !PI_FRF(ID_XY, axis_xy) )
   {
		AfxMessageBox(_T("PI stage XY reference problem\n"));
   }
   
   // Wait until done.
   bFlag = FALSE;
   
   while( bFlag != TRUE )
   {
		PI_IsControllerReady(ID_XY, &bFlag);
   }
  
	initialized = true;

	SetVelocity( velocity_nor );
}

void COctopusStage686::SetVelocity( double velocity )
{
	double velocity_arr[2];
	
	if( velocity >= 0.01 && velocity <= 10 )
	{
		velocity_arr[0] = velocity;
		velocity_arr[1] = velocity;
		PI_VEL(ID_XY, axis_xy, velocity_arr);
	}
}

void COctopusStage686::MoveRelX( double dist )
{
	double new_target = target_xy[0] + dist;

	if( new_target > (middle - range) && new_target < (middle + range) ) 
	{
		target_xy[0] = new_target;
		if (XY_connected) PI_MOV(ID_XY, axis_xy, target_xy);
		debug.Format(_T(" PI 686 stage X move to %.3f"), target_xy[0]);
		if ( glob_m_pLog != NULL ) glob_m_pLog->Write(debug);
	}
}

void COctopusStage686::MoveRelY( double dist )
{
	double new_target = target_xy[1] + dist;

	if( new_target > (middle - range) && new_target < (middle + range) ) 
	{
		target_xy[1] = new_target;
	    if (XY_connected) PI_MOV(ID_XY, axis_xy, target_xy);
		debug.Format(_T(" PI 686 stage Y move to %.3f"), target_xy[1]);
		if ( glob_m_pLog != NULL ) glob_m_pLog->Write(debug);
	}
}

void COctopusStage686::MoveToX( double target_mm )
{
	if( IsJoyStickOn() ) TurnJoyStickOff();

	if( target_mm > (middle - range) && target_mm < (middle + range) ) 
	{
		target_xy[0] = target_mm;
		if (XY_connected) PI_MOV(ID_XY, axis_xy, target_xy);
		debug.Format(_T(" PI 686 stage X move to %.3f"), target_xy[0]);
		if ( glob_m_pLog != NULL ) glob_m_pLog->Write(debug);
	}
}

void COctopusStage686::MoveToY( double target_mm )
{
	if( IsJoyStickOn() ) TurnJoyStickOff();

	if( target_mm > (middle - range) && target_mm < (middle + range) ) 
	{
		target_xy[1] = target_mm;
		if (XY_connected) PI_MOV(ID_XY, axis_xy, target_xy);
		debug.Format(_T(" PI 686 stage Y move to %.3f"), target_xy[1]);
		if ( glob_m_pLog != NULL ) glob_m_pLog->Write(debug);
	}
}

void COctopusStage686::MoveToXY( double target_x_mm, double target_y_mm )
{
	if( IsJoyStickOn() ) TurnJoyStickOff();

	if( target_x_mm > (middle - range) && target_x_mm < (middle + range) ) 
	{
		if( target_y_mm > (middle - range) && target_y_mm < (middle + range) ) 
		
		{
			target_xy[0] = target_x_mm;
			target_xy[1] = target_y_mm;
			//do a vector move
			if (XY_connected) PI_MVE(ID_XY, axis_xy, target_xy);

			debug.Format(_T(" PI 686 stage XY move to %.3f %.3f"), target_xy[0], target_xy[1]);
		
			if ( glob_m_pLog != NULL ) glob_m_pLog->Write(debug);
		}
	}
}

void COctopusStage686::MoveLeft(  void ) { MoveRelY( +1 * stepsize ); }
void COctopusStage686::MoveRight( void ) { MoveRelY( -1 * stepsize ); }
void COctopusStage686::MoveBack(  void ) { MoveRelX( +1 * stepsize ); }
void COctopusStage686::MoveFwd(   void ) { MoveRelX( -1 * stepsize ); }

void COctopusStage686::OnStepSize1() { m_Radio_S = 0; stepsize = 0.0005; UpdateData (false); };
void COctopusStage686::OnStepSize2() { m_Radio_S = 1; stepsize = 0.0050; UpdateData (false); };
void COctopusStage686::OnStepSize3() { m_Radio_S = 2; stepsize = 0.0500; UpdateData (false); };
void COctopusStage686::OnStepSize4() { m_Radio_S = 3; stepsize = 0.2000; UpdateData (false); };
void COctopusStage686::OnStepSize5() { m_Radio_S = 4; stepsize = 1.0000; UpdateData (false); };
/*
bool COctopusStage686::Open_A_File( void ) 
{	
	PathFileName.Format("PosMap.seq");

	fopen_s(&pFileHead, PathFileName , _T("wt"));

	if ( pFileHead != NULL )
	{
		CString str;
		str.Format(_T("save off\n"));
		fprintf( pFileHead, str );
		fflush( pFileHead );

		return true;
	}
	else 
		return false;

}

void COctopusStage686::Close_The_File( void )
{

	if( pFileHead == NULL ) 
		return;

	fflush( pFileHead );
	fclose( pFileHead );

	pFileHead = NULL;

	Beep(600,500);
}

void COctopusStage686::WritePos( void ) 
{

	// if the file is closed - due to any number of reasons 
	// => open a new one

	if( pFileHead == NULL ) 
	{ 
		if ( !Open_A_File() ) return; 
	}
	
	GetPosition();

	target_xy[0] = B.position_x_686;
	target_xy[1] = B.position_y_686;

	CString str;

	str.Format(_T("%.3f %.3f %.3f %.3f\n"), target_xy[0], target_xy[1], B.position_z_sample, B.position_z_objective);
	fprintf( pFileHead, str );

	fflush( pFileHead );

	Beep(300,500);
}
*/
void COctopusStage686::StageCenter( void ) 
{	
	SetVelocity( 2.0 );

	target_xy[0] = middle;
	target_xy[1] = middle;

	if (XY_connected) PI_MOV(ID_XY, axis_xy, target_xy);

	SetVelocity( velocity_nor );

	debug.Format(_T(" PI 686 stage centered "));
	if ( glob_m_pLog != NULL ) glob_m_pLog->Write(debug);
}

void COctopusStage686::OnSave( void ) 
{
	save_xy[0] = target_xy[0];
	save_xy[1] = target_xy[1];

	if( IsWindowVisible() ) 
	{
	    CString str;
		str.Format(_T("X:%.5f\nY:%.5f"), save_xy[0], save_xy[1]);
		m_Pos_Save.SetWindowText( str );
	}

	debug.Format(_T(" PI 686 stage save position "));
	if ( glob_m_pLog != NULL ) glob_m_pLog->Write(debug);
}

void COctopusStage686::OnSaveGoTo( void ) 
{	
	target_xy[0] = save_xy[0];
	target_xy[1] = save_xy[1];
	
	if (XY_connected) PI_MOV(ID_XY, axis_xy, target_xy);

	debug.Format(_T(" PI 686 stage goto save "));
	if ( glob_m_pLog != NULL ) glob_m_pLog->Write(debug);
}

bool COctopusStage686::IsJoyStickOn( void )
{
	bJONState[0] = TRUE;
	bJONState[1] = TRUE;

	int J_ID[2];
	
	J_ID[0] = 1;
	J_ID[1] = 2;
	
	PI_qJON(ID_XY, J_ID, bJONState, 2);
		
	if( bJONState[0] == TRUE )
	{
		JoyStickOn = true;
		return true;
	}
	else
	{
		JoyStickOn = false;
		return false;
	}
}

void COctopusStage686::OnJoyStickOnOff( void )
{
	if( IsJoyStickOn() == true )
	{
		TurnJoyStickOff();
	} 
	else
	{
		TurnJoyStickOn();
	}

	Beep(800,200);
}

void COctopusStage686::TurnJoyStickOff( void )
{
	int J_ID[2];
	
	J_ID[0] = 1;
	J_ID[1] = 2;

	if( IsJoyStickOn() == true )
	{
		//turn it off
		bJONState[0] = FALSE;
		bJONState[1] = FALSE;

		PI_JON(ID_XY, J_ID, bJONState, 2);

		GetPosition();

		target_xy[0] = B.position_x_686;
		target_xy[1] = B.position_y_686;

		JoyStickOn = false;

		JoystickModeNo();
	} 
}

void COctopusStage686::TurnJoyStickOn( void )
{
	if( IsJoyStickOn() == false )
	{
		int J_ID[2];
	
		J_ID[0] = 1;
		J_ID[1] = 2;

		bJONState[0] = TRUE;
		bJONState[1] = TRUE;

		PI_JON(ID_XY, J_ID, bJONState, 2);

		JoyStickOn = true;

		JoystickModeYes();
	} 
}

void COctopusStage686::GetPosition( void ) 
{ 

	if ( XY_connected )
	{
		bIsMoving[0] = TRUE;
		bIsMoving[1] = TRUE;

		dPos[0] = B.position_x_686;
		dPos[1] = B.position_y_686;

		PI_IsMoving(ID_XY, axis_xy, bIsMoving);
		
		if (!bIsMoving[0] && !bIsMoving[1])
		{
			PI_qPOS(ID_XY, axis_xy, dPos);
			B.position_x_686 = dPos[0];
			B.position_y_686 = dPos[1];
		}

		ShowPosition();
	}
}

bool COctopusStage686::StageMoving( void )
{
	bIsMoving[0] = TRUE;
	bIsMoving[1] = TRUE;

	PI_IsMoving(ID_XY, axis_xy, bIsMoving);
	
	if ( bIsMoving[0] || bIsMoving[1] )
	{ 
		return true;
	}
	else
	{
		return false;
	}
}

void COctopusStage686::ShowPosition( void )
{
	if( IsWindowVisible() ) 
	{
	    CString str;
		if( JoyStickOn == true )
			str.Format(_T("X:%.5f\nY:%.5f\nJoystick ON"), B.position_x_686, B.position_y_686);
		else
			str.Format(_T("X:%.5f\nY:%.5f\nJoystick OFF"), B.position_x_686, B.position_y_686);
		m_Pos.SetWindowText( str );
	}
}

BOOL COctopusStage686::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	int id = LOWORD(wParam);     // Notification code
	if( id == 2 ) return FALSE;  // Trap ESC key
	if( id == 1 ) return FALSE;  // Trap RTN key
    return CDialog::OnCommand(wParam, lParam);
}

void COctopusStage686::OnKillfocusGeneral( void ) { UpdateData( true ); }

void COctopusStage686::OnKillfocusVelocityNor( void ) 
{
	UpdateData( true );
	SetVelocity( velocity_nor );
}

void COctopusStage686::Laser_On( void ) 
{
	//if (glob_m_pNI == NULL) return;
	//glob_m_pNI->Laser_On();
}

void COctopusStage686::Laser_Off( void ) 
{
	//if (glob_m_pNI == NULL) return;
	//glob_m_pNI->Laser_Off();
}

void COctopusStage686::JoystickModeYes( void ) 
{	
	if( IsWindowVisible() ) 
	{
		GetDlgItem( IDC_STAGEm_RIGHT  )->EnableWindow( false );
		GetDlgItem( IDC_STAGEm_LEFT   )->EnableWindow( false );
		GetDlgItem( IDC_STAGEm_BACK   )->EnableWindow( false );
		GetDlgItem( IDC_STAGEm_FWD    )->EnableWindow( false );
		GetDlgItem( IDC_STAGEm_CENTER )->EnableWindow( false );
	}
}

void COctopusStage686::JoystickModeNo( void ) 
{
	if( IsWindowVisible() ) 
	{
		GetDlgItem( IDC_STAGEm_RIGHT  )->EnableWindow( true );
		GetDlgItem( IDC_STAGEm_LEFT   )->EnableWindow( true );
		GetDlgItem( IDC_STAGEm_BACK   )->EnableWindow( true );
		GetDlgItem( IDC_STAGEm_FWD    )->EnableWindow( true );
		GetDlgItem( IDC_STAGEm_CENTER )->EnableWindow( true );
	}
}

DWORD WINAPI MonitorPosition686( LPVOID lpParameter )
{
	COctopusStage686 * this_ = (COctopusStage686*)lpParameter; 
	this_ -> thread_count++;

	while( this_ -> wait_thread_running )
	{
		this_ -> GetPosition();
		Sleep(500);
	}

	this_ -> thread_count--;
	return 0;
}

void COctopusStage686::ImageOneCell( void )
{

	if ( B.Camera_Thread_running )
	{
		AfxMessageBox(_T("The camera is running in movie mode.\nPlease stop the current aquisition first, and then try again."));
		return;
	}

	int i = Cell_List.GetCount();
	
	if ( i < 1 ) 
	{
		AfxMessageBox("No cells in list! You need at least 1.");
		return;
	}

	if ( JoyStickOn == true )
	{
		AfxMessageBox("Yipes - you are in Joystick mode - please change that so the computer can move the stage!");
		return;
	}

	if ( B.savetofile == false )
	{
		AfxMessageBox("Just a heads up - You have not turned on filesave.\nNothing will be saved to file.");
	}

	GetPosition();

	old_x = B.position_x_686;
	old_y = B.position_y_686;
	old_z = B.position_z_sample;

	if ( glob_m_pScope == NULL ) return;

	bool BFwasOn = false;
	//turn the light off just in case
	if( glob_m_pScope->IsBFOn() ) 
	{
		BFwasOn = true;
		glob_m_pScope->BrightFieldFastOff();
	}

	ImageThisCell( 0 ); //this is the first (or last) in the list of entries

	if( BFwasOn ) {
		glob_m_pScope->BrightFieldFastOn();
	}

	//move to x and y pos
	MoveToXY( old_x, old_y );

	while ( StageMoving() ) { Sleep( 50 ); }
	
}

void COctopusStage686::ImageThisCell( unsigned int CellID )
{

	if ( glob_m_pPictureDisplay == NULL ) return;
	if ( glob_m_pCamera         == NULL ) return;
	if ( glob_m_pLasers         == NULL ) return;

	int i = Cell_List.GetCount();

	if ( CellID >= i ) //the cell ID is ZERO based!
	{
		AfxMessageBox("CellID >= i - desired cell index exceeds number of cells in Cell_List");
		return;
	}

	CString command;
	Cell_List.GetText( CellID, command );

	unsigned int cellnum = 0;
	float x = 0.0;
	float y = 0.0;
	float z = 0.0;

	//box.Format(_T("cell %d x:%.3f y:%.3f z:%.3f")
	sscanf(command, "cell:%d x:%f y:%f z:%f", &cellnum, &x, &y, &z);

	//give each file a nice name and force a new file to be opened
	CString CellStr;
	CellStr.Format(_T("Cell%d"), cellnum);
	B.cellstring = CellStr;
	//force the camera to open a new file by closing the current file
	glob_m_pPictureDisplay->Close_The_File();

	B.AUTO_CellLabel = cellnum; //this is for the file header string

 	float z_bottom = z - float(AutoStepNum * AutoStepSize) * 0.001; //need to convert to microns, hence the 0.001
	float z_top    = z + float(AutoStepNum * AutoStepSize) * 0.001; //need to convert to microns

	if ( z_bottom < 0.0 ) {
		AfxMessageBox("Z axis bottom out of range!");
		return;
	}

	if ( z_top > 200.0 ) {
		AfxMessageBox("Z axis top out of range!");
		return;
	}

	//move to x and y pos
	MoveToXY( x, y );

	while ( StageMoving() ) { Sleep( 50 ); }
	
	Sleep( 100 );

	//all seems good so far

	if ( emergencySTOP == true ) {  emergencySTOP = false; return; };

	//First channel
	if ( Cbox_Laser1.GetCheck() )
	{
		//Let's roll on Laser One
		//move to first Z height
		glob_m_pSamplePiezo->MoveToZ( z_bottom );
		
		Sleep(50);

		//set the filter wheel
		glob_m_pScope->EpiFilterWheelDirect( Las1_FW_Choice.GetCurSel() + 1 );
		
		Sleep(50); //just for safety - proably not needed

		glob_m_pLasers->Laser_639_On();
		
		Sleep(10); //just for safety - proably not needed

		glob_m_pCamera->TakePicture( Las1_ExpTime, Las1_Gain );
		
		for( u16 step = 1; step <= (2*AutoStepNum); step++)
		{
			glob_m_pSamplePiezo->MoveToZ( z_bottom + double( step * AutoStepSize ) * 0.001 );
			Sleep(50);
			glob_m_pCamera->TakePicture( Las1_ExpTime, Las1_Gain );
			Sleep(50);
		}

		glob_m_pLasers->Laser_639_Off();
	}

	if ( emergencySTOP == true ) {  emergencySTOP = false; return; };

	//Second channel
	if ( Cbox_Laser2.GetCheck() )
	{
		//Let's roll on Laser Two
		//move to first Z height
		glob_m_pSamplePiezo->MoveToZ( z_bottom );
		
		Sleep(50);
		
		//set the filter wheel
		glob_m_pScope->EpiFilterWheelDirect( Las2_FW_Choice.GetCurSel() + 1 );
		
		Sleep(50); //just for safety - proably not needed

		glob_m_pLasers->Laser_561_On();

		Sleep(10); //just for safety - proably not needed

		glob_m_pCamera->TakePicture( Las2_ExpTime, Las2_Gain );

		for( u16 step = 1; step <= (2*AutoStepNum); step++)
		{
			glob_m_pSamplePiezo->MoveToZ( z_bottom + double( step * AutoStepSize ) * 0.001 );
			Sleep(50);
			glob_m_pCamera->TakePicture( Las2_ExpTime, Las2_Gain );
			Sleep(50);
		}

		glob_m_pLasers->Laser_561_Off();
	}

	if ( emergencySTOP == true ) {  emergencySTOP = false; return; };

	//Third channel
	if ( Cbox_Laser3.GetCheck() )
	{
		//Let's roll on Laser Two
		//move to first Z height
		glob_m_pSamplePiezo->MoveToZ( z_bottom );
		
		Sleep(50);
		
		//set the filter wheel
		glob_m_pScope->EpiFilterWheelDirect( Las3_FW_Choice.GetCurSel() + 1 );
		
		Sleep(50); //just for safety - proably not needed

		glob_m_pLasers->Laser_488_On();

		Sleep(10); //just for safety - proably not needed

		glob_m_pCamera->TakePicture( Las3_ExpTime, Las3_Gain );

		for( u16 step = 1; step <= (2*AutoStepNum); step++)
		{
			glob_m_pSamplePiezo->MoveToZ( z_bottom + double( step * AutoStepSize ) * 0.001 );
			Sleep(50);
			glob_m_pCamera->TakePicture( Las3_ExpTime, Las3_Gain );
			Sleep(50);
		}

		glob_m_pLasers->Laser_488_Off();
	}

	if ( emergencySTOP == true ) {  emergencySTOP = false; return; };

	//Fourth channel
	if ( Cbox_Laser4.GetCheck() )
	{
		//Let's roll on Laser Two
		//move to first Z height
		glob_m_pSamplePiezo->MoveToZ( z_bottom );
		
		Sleep(50);
		
		//set the filter wheel
		glob_m_pScope->EpiFilterWheelDirect( Las4_FW_Choice.GetCurSel() + 1 );
		
		Sleep(50); //just for safety - proably not needed

		glob_m_pLasers->Laser_405_On();

		Sleep(10); //just for safety - proably not needed

		glob_m_pCamera->TakePicture( Las4_ExpTime, Las4_Gain );

		for( u16 step = 1; step <= (2*AutoStepNum); step++)
		{
			glob_m_pSamplePiezo->MoveToZ( z_bottom + double( step * AutoStepSize ) * 0.001 );
			Sleep(50);
			glob_m_pCamera->TakePicture( Las4_ExpTime, Las4_Gain );
			Sleep(50);
		}

		glob_m_pLasers->Laser_405_Off();
	}

	if ( emergencySTOP == true ) {  emergencySTOP = false; return; };

	//Fifth channel
	if ( Cbox_Brightfield.GetCheck() )
	{
	
		//TURN LAMP ON
		glob_m_pScope->BrightFieldFastOn();
		
		Sleep(1000); //wait for the lamp to come up to temp

		//move to first Z height
		glob_m_pSamplePiezo->MoveToZ( z_bottom );
		
		Sleep(50);
		
		//set the filter wheel
		glob_m_pScope->EpiFilterWheelDirect( Las5_FW_Choice.GetCurSel() + 1 );
		
		Sleep(50); //just for safety - proably not needed

		glob_m_pCamera->TakePicture( Las5_ExpTime, Las5_Gain );

		for( u16 step = 1; step <= (2*AutoStepNum); step++)
		{
			glob_m_pSamplePiezo->MoveToZ( z_bottom + double( step * AutoStepSize ) * 0.001 );
			Sleep(50);
			glob_m_pCamera->TakePicture( Las5_ExpTime, Las5_Gain );
			Sleep(50);
		}

		//TURN LAMP OFF
		glob_m_pScope->BrightFieldFastOff();
		
		Sleep(1000); //wait for the lamp to come up to temp
	}

	//move to original height
	glob_m_pSamplePiezo->MoveToZ( old_z );
	
}

void COctopusStage686::CellAdd( void )
{
	CString box;
	int i = Cell_List.GetCount();
	box.Format(_T("cell:%d x:%.3f y:%.3f z:%.3f"), i, B.CellParams.x, B.CellParams.y, B.CellParams.z );  
	Cell_List.InsertString( 0, box );
}

void COctopusStage686::CellRemove( void )
{
	Cell_List.DeleteString( 0 ); //This is the first entry
}

void COctopusStage686::ImageStop()
{
	emergencySTOP = true;
}

void COctopusStage686::ImageManyCells( void )
{

	if ( B.Camera_Thread_running )
	{
		AfxMessageBox(_T("The camera is running in movie mode.\nPlease stop the current aquisition first, and then try again."));
		return;
	}

	if ( glob_m_pScope == NULL ) return;

	int nc = Cell_List.GetCount();
	
	if ( nc < 2 ) 
	{
		AfxMessageBox("you do not have multiple cells in the list!");
		return;
	}

	if ( JoyStickOn == true )
	{
		AfxMessageBox("Yipes - you are in Joystick mode - please change that so the computer can move the stage!");
		return;
	}

	if ( B.savetofile == false )
	{
		AfxMessageBox("Just a heads up - You have not turned on filesave.\n Nothing will be saved to file.");
	}

	GetPosition();

	old_x = B.position_x_686;
	old_y = B.position_y_686;
	old_z = B.position_z_sample;

	//do we have more than one cells in the list?

	bool BFwasOn = false;
	//turn the light off just in case
	if( glob_m_pScope->IsBFOn() ) 
	{
		BFwasOn = true;
		glob_m_pScope->BrightFieldFastOff();
	}

	for(int cell = 0; cell < nc; cell++)
	{
		ImageThisCell( cell );
		Sleep( 1000 ); //to give people a change to stop the process this might work?
	}

	//reset everything to where is was - lights, position, etc. 
	if( BFwasOn ) {
		glob_m_pScope->BrightFieldFastOn();
	}

	//move to x and y pos
	MoveToXY( old_x, old_y );

	while ( StageMoving() ) { Sleep( 50 ); }

	//move to old z
	glob_m_pSamplePiezo->MoveToZ( old_z );
	
}