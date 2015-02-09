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

/**********************************************************************************
Note that the Andor firmware and SDK have known eccentricities. They change over time.

1) The parameters that need to be passed the physical shutter changed at some point.

2) For some reason, the are problems with passing a ‘0’ to SetEMCCDGain when you are in EMCCD mode.
This makes sense, sort of (why would you be in EMCCD mode in the first place if you
did not want any gain) but technically you should be able to pass any value from 0-255 if you control 
the gain via an 8-bit unsigned int. In any case, the new cameras go from 0-1000 (or 1-1000) - this has never been
clear from the documentation. 

3) GetNumberNewImages(&first, &last) is slightly schizophrenic. When there _is_ one new image, this function (basically) returns 0.
Guys @ Andor - how about setting up functions called GetNumberNewImages to return 1 when there is 1 new image? 
The key issue is how you are defining ‘first’ and ‘last’.
If I start an acquisition, and take one picture, last == first, and therefore (last - first) = 0
I think you want something like GetNumberNewImages( &u16_NumberNewImages ) and just want u16_NumberNewImages to be equal to the number of new images.
Why should the user have to worry about your internal circular buffering setup?  		
**********************************************************************************/


#include "stdafx.h"
#include "atmcd32d.h"
#include "Octopus.h"
#include "OctopusDoc.h"
#include "OctopusView.h"
#include "OctopusClock.h"
#include "OctopusCameraDlg.h"
#include "OctopusCameraDisplay.h"
#include "OctopusGlobals.h"
#include "OctopusLog.h"
#include "OctopusMultifunction.h"

#include <direct.h>
#include <stdlib.h>
#include <stdio.h>
#include <Mmsystem.h>

DWORD WINAPI FocusThread(LPVOID lpParameter);

extern COctopusGlobals    B;
extern COctopusLog*       glob_m_pLog;
COctopusPictureDisplay*   glob_m_pPictureDisplay = NULL;
extern COctopusGoodClock* glob_m_pGoodClock;
extern COctopusMultifunction*    glob_m_pNI;

COctopusCamera::COctopusCamera(CWnd* pParent)
	: CDialog(COctopusCamera::IDD, pParent)
{    
	B.files_written		= 0;
	B.savetofile		= false;
	B.W                     = 0;
	B.H                     = 0;
	B.automatic_gain	= true;
	B.manual_gain		= 0.05;
	B.pics_per_file		= 200;
	B.Camera_Thread_running = false;
	B.bin                   = 1;
	B.memory                = NULL;
	B.savetime              = 0;
	B.nd_setting            = 0;
	B.expt_frame            = 0;
	B.expt_time             = 0;
	B.ROI_changed           = true;
	B.SetFocus              = false;
	FrameTransfer           = true;

	//allocate lots of memory and be done with it...
	//this is way more than we should ever need.
	B.memory = new u16 [201 * 512 * 512];

	u32_Exposure_Time_Single_ms 	= 50;
	u32_Exposure_Time_Movie_ms  	= 50;
	B.CameraExpTime_ms          	= u32_Exposure_Time_Single_ms;
	u16_Gain_Mult_Factor_Single 	= 1;
	u16_Gain_Mult_Factor_Movie  	= 1;
	m_IDC_MANUAL_GAIN           	= B.manual_gain;
	m_DISPLAY_GAIN_CHOICE       	= 1;
	m_DISPLAY_VSSPEED           	= 3;
	m_DISPLAY_GL                	= 0;
	m_DISPLAY_BIN_CHOICE        	= B.bin - 1;
	m_IDC_PICTURES_PER_FILE     	= B.pics_per_file;
	m_CCD_target_temp		= 10;
	m_CCD_current_temp          	= 0;
	
	//default the converter to 1 MHZ conventional
	m_DISPLAY_HSSPEED           	= 0;

	//does not really matter. will be set correctly by OnHSSPEED_X() anyway
	B.Ampl_Conv			= false; 

	VERIFY(m_bmp_no.LoadBitmap(IDB_NO));
	VERIFY(m_bmp_yes.LoadBitmap(IDB_YES));
	
    	CString currentDir;
    	GetCurrentDirectory( MAX_PATH, currentDir.GetBufferSetLength(MAX_PATH) );
    	currentDir.ReleaseBuffer();

	currentDir.Append(_T("\\fallback"));

	pathname_display = currentDir;
	B.pathname       = pathname_display;

	if( Create(COctopusCamera::IDD, pParent) ) 
		ShowWindow( SW_SHOW );

	if ( glob_m_pLog != NULL ) 
		glob_m_pLog->Write(_T(" COctopusCamera(CWnd* pParent) "));

	//this is for the old versus Andor iXon 
	em_limit = 255;

	if ( B.Andor_new ) 
		em_limit = 950; //we are dealing with a new camera
		
}

void COctopusCamera::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
    
	DDX_Control( pDX, IDC_CAS_TEMP_CURRENT, m_Temperature_Now);
	DDX_Control(pDX, IDC_STATIC_GAIN1,      m_g1_text1);
	DDX_Control(pDX, IDC_STATIC_GAIN2,      m_g1_text2);

   	DDX_Text( pDX, IDC_CAS_EXPOSURE_TIME_SINGLE, u32_Exposure_Time_Single_ms);
   	DDV_MinMaxLong(pDX, u32_Exposure_Time_Single_ms, 1, 10000);
	DDX_Text( pDX, IDC_CAS_EXPOSURE_TIME_MOVIE, u32_Exposure_Time_Movie_ms);
	DDV_MinMaxLong(pDX, u32_Exposure_Time_Movie_ms, 1, 10000);

    	DDX_Text( pDX, IDC_CAS_GAIN_SINGLE, u16_Gain_Mult_Factor_Single);
    	DDV_MinMaxInt(pDX, u16_Gain_Mult_Factor_Single, 1, em_limit);
    	DDX_Text( pDX, IDC_CAS_GAIN_MOVIE, u16_Gain_Mult_Factor_Movie);
    	DDV_MinMaxInt(pDX, u16_Gain_Mult_Factor_Movie, 1, em_limit);

	DDX_Text( pDX, IDC_CAS_MANUAL_GAIN,	m_IDC_MANUAL_GAIN);
    	DDV_MinMaxDouble(pDX, m_IDC_MANUAL_GAIN, 0.0001, 10.0);
	
	DDX_Text( pDX, IDC_CAS_PICTURES_PER_FILE, m_IDC_PICTURES_PER_FILE);
	DDV_MinMaxInt(pDX, m_IDC_PICTURES_PER_FILE, 1, 10000);
    
	DDX_Text( pDX, IDC_CAS_TEMP_TARGET, m_CCD_target_temp);
	DDV_MinMaxInt(pDX, m_CCD_target_temp, -75, 25);

	DDX_Radio( pDX, IDC_CAS_DISPLAY_GAIN_MANUAL, m_DISPLAY_GAIN_CHOICE);
	DDX_Radio( pDX, IDC_CAS_DISPLAY_BIN_1, m_DISPLAY_BIN_CHOICE);

	DDX_Radio( pDX, IDC_CAS_VSSPEED_0, m_DISPLAY_VSSPEED);
	DDX_Radio( pDX, IDC_CAS_HSSPEED_0, m_DISPLAY_HSSPEED);
	DDX_Radio( pDX, IDC_CAS_GL_0,      m_DISPLAY_GL);

	DDX_Text(pDX, IDC_CAS_PATHNAME,          pathname_display);
	DDX_Control(pDX, IDC_CAS_SAVEONOFF_BMP,  m_status_save);
	DDX_Control(pDX, IDC_CAS_FT,		 m_ctlFTCheckBox);
}  

BEGIN_MESSAGE_MAP(COctopusCamera, CDialog)

	ON_BN_CLICKED(IDC_CAS_STARTSTOP_PICTURE, TakePicture)
	ON_BN_CLICKED(IDC_CAS_START_MOVIE, StartMovie)
	ON_BN_CLICKED(IDC_CAS_STOP_MOVIE, StopCameraThread)

	ON_BN_CLICKED(IDC_CAS_SETPATH, OnSetPath)

	ON_EN_KILLFOCUS(IDC_CAS_EXPOSURE_TIME_SINGLE, OnKillfocusGeneral)
	ON_EN_KILLFOCUS(IDC_CAS_EXPOSURE_TIME_MOVIE, OnKillfocusGeneral)

	ON_EN_KILLFOCUS(IDC_CAS_GAIN_SINGLE, OnKillfocusGeneral)
	ON_EN_KILLFOCUS(IDC_CAS_GAIN_MOVIE, OnKillfocusGeneral)

	ON_EN_KILLFOCUS(IDC_CAS_TEMP_TARGET, OnKillfocusTempTarget)
	ON_EN_KILLFOCUS(IDC_CAS_MANUAL_GAIN, OnKillfocusManualGain)
	
	ON_EN_KILLFOCUS(IDC_CAS_PICTURES_PER_FILE, OnKillfocusPicturesPerFile)
	
	ON_BN_CLICKED(IDC_CAS_DISPLAY_GAIN_MANUAL, OnDisplayGainManual)
	ON_BN_CLICKED(IDC_CAS_DISPLAY_GAIN_AUTOMATIC, OnDisplayGainAutomatic)

	ON_BN_CLICKED(IDC_CAS_DISPLAY_BIN_1, OnBinning1x1)
	ON_BN_CLICKED(IDC_CAS_DISPLAY_BIN_2, OnBinning2x2)
	ON_BN_CLICKED(IDC_CAS_DISPLAY_BIN_4, OnBinning4x4)
	ON_BN_CLICKED(IDC_CAS_DISPLAY_BIN_8, OnBinning8x8)

	ON_BN_CLICKED(IDC_CAS_VSSPEED_0, OnVSSPEED_0)
	ON_BN_CLICKED(IDC_CAS_VSSPEED_1, OnVSSPEED_1)
	ON_BN_CLICKED(IDC_CAS_VSSPEED_2, OnVSSPEED_2)
	ON_BN_CLICKED(IDC_CAS_VSSPEED_3, OnVSSPEED_3)
	ON_BN_CLICKED(IDC_CAS_VSSPEED_4, OnVSSPEED_4)

	ON_BN_CLICKED(IDC_CAS_HSSPEED_0, OnHSSPEED_0)
	ON_BN_CLICKED(IDC_CAS_HSSPEED_1, OnHSSPEED_1)
	ON_BN_CLICKED(IDC_CAS_HSSPEED_2, OnHSSPEED_2)
	ON_BN_CLICKED(IDC_CAS_HSSPEED_3, OnHSSPEED_3)
	ON_BN_CLICKED(IDC_CAS_HSSPEED_4, OnHSSPEED_4)
	ON_BN_CLICKED(IDC_CAS_HSSPEED_5, OnHSSPEED_5)
	
    	ON_BN_CLICKED(IDC_CAS_GL_0, OnGL_0)
	ON_BN_CLICKED(IDC_CAS_GL_1, OnGL_1)
	ON_BN_CLICKED(IDC_CAS_GL_2, OnGL_2)

	ON_BN_CLICKED(IDC_CAS_SAVEONOFF, OnFileChange)
	//ON_BN_CLICKED(IDC_CAS_FOCUSONOFF, OnFocusChange)
	ON_BN_CLICKED(IDC_CAS_FT, OnBnClickedCasFt)
	ON_WM_TIMER()

END_MESSAGE_MAP()

BOOL COctopusCamera::OnInitDialog() 
{
	CDialog::OnInitDialog();

	char aBuffer[256];
  
	GetCurrentDirectory(256, aBuffer);
	// Look in current working directory
	// for driver files

	if( andor::Initialize(aBuffer) != DRV_SUCCESS ) 
		AfxMessageBox(_T("Failure Opening Andor Camera"));
	
	Opencam(); 

	if ( glob_m_pLog != NULL ) 
		glob_m_pLog->Write(_T(" COctopusCamera::OnInitDialog() "));

	return TRUE;
}

COctopusCamera::~COctopusCamera() 
{
	if( B.Camera_Thread_running ) 
		StopCameraThread();

	if( CoolerOFF() != DRV_SUCCESS ) 
		AfxMessageBox(_T("Failure turning Cooler Off"));

	do{ GetTemperature(&m_CCD_current_temp); } while ( m_CCD_current_temp < 0 );
	
	if ( B.memory != NULL ) 
	{
		delete [] B.memory;
		B.memory = NULL;
	}

    	ShutDown();

	if( glob_m_pPictureDisplay != NULL ) 
	{
		glob_m_pPictureDisplay->DestroyWindow();
		delete glob_m_pPictureDisplay;
		glob_m_pPictureDisplay = NULL;
	}

	if ( glob_m_pLog != NULL ) 
		glob_m_pLog->Write(" ~COctopusCamera() ");
}

/*********************************************************************************
* Open the Camera
*********************************************************************************/

void COctopusCamera::Opencam() 
{
	SetTemperature( m_CCD_target_temp );
	
	if( CoolerON()!= DRV_SUCCESS ) 
		AfxMessageBox(_T("Failure turning Cooler On"));
	
	int xpixels;
	int ypixels;

	GetDetector( &xpixels, &ypixels );
	
	B.CCD_x_phys_e = xpixels;
	B.CCD_y_phys_e = ypixels;

	B.ROI_target.x1 = 1;
	B.ROI_target.y1 = 1;
	B.ROI_target.x2 = B.CCD_x_phys_e;
	B.ROI_target.y2 = B.CCD_y_phys_e;

	SetROI_To_Default();

	//keep track of temperature
	SetTimer( TIMER_TEMP, 2000, NULL ); 

	//enable access to gains > 300
	if ( B.Andor_new )
		SetEMAdvanced(1); 
	
	OnVSSPEED_3();
	OnHSSPEED_0(); //10 MHz, 14 bit, EMCCD
	
	// 0 = 0-255
	// 1 = 0-4096
	// 2 = linear
	// 3 = real
	
	if ( B.Andor_new )
		OnGL_2();
	else
		OnGL_0();

	CString str;
	
	if ( B.Andor_new )
	{
		str.Format(_T("EMCCD Gain (1 to 950):"));
		GetDlgItem( IDC_CAS_GL_0 )->EnableWindow( false );
		GetDlgItem( IDC_CAS_GL_1 )->EnableWindow( false );
	}
	else
	{
		str.Format(_T("EMCCD Gain (1 to 255):"));
		GetDlgItem( IDC_CAS_GL_1 )->EnableWindow( false );
		GetDlgItem( IDC_CAS_GL_2 )->EnableWindow( false );
	}

	m_g1_text1.SetWindowText( str );
	m_g1_text2.SetWindowText( str );

/*
	if ( FrameTransfer )
		CheckDlgButton(IDC_CAS_FT, BST_CHECKED);
	else
		CheckDlgButton(IDC_CAS_FT, BST_UNCHECKED);
*/

	if( FrameTransfer ) 
		m_ctlFTCheckBox.SetCheck( 1 );
	else
		m_ctlFTCheckBox.SetCheck( 0 );


	if ( glob_m_pLog != NULL ) 
		glob_m_pLog->Write(" COctopusCamera::OpenCam() ");

}

/*********************************************************************************
*********************************************************************************/

void COctopusCamera::TakePicture() 
{
	if( B.Camera_Thread_running ) 
		return;

	if ( glob_m_pLog != NULL ) 
		glob_m_pLog->Write(" COctopusCamera::TakePicture() ");

	SetReadMode(4);             //set read out mode to image
	SetAcquisitionMode(1);      //set mode for single scan
	SetExposureTime( float(u32_Exposure_Time_Single_ms) * 0.001 );
	SetTriggerMode( 0 );		// internal trigger

	B.CameraExpTime_ms = u32_Exposure_Time_Single_ms;

	if ( B.Ampl_Conv )
		SetEMCCDGain( 0 );
	else
        SetEMCCDGain( int(u16_Gain_Mult_Factor_Single) );
    
	if ( B.Andor_new )
		SetShutter( 0, 1, 0, 0 ); // Always open
	else
		SetShutter( 1, 1, 0, 0 ); // Always Open
	
	if ( B.ROI_changed ) 
		SetROI();

	SetImage( B.ROI_actual.bin, B.ROI_actual.bin, \
		  B.ROI_actual.x1,  B.ROI_actual.x2,  \
		  B.ROI_actual.y1,  B.ROI_actual.y2 );
	
	if ( glob_m_pPictureDisplay == NULL || B.ROI_changed ) 
		OpenWindow();

	StartAcquisition();
	
	int status = 0;
	do{ GetStatus( &status ); } while ( status != DRV_IDLE );
    
	GetAcquiredData16( B.memory, B.H * B.W );

	if ( glob_m_pPictureDisplay != NULL ) 
		glob_m_pPictureDisplay->Update_Bitmap( B.memory, 1 );
	
}

void COctopusCamera::TakePicture( double exposuretime_ms, int gain ) 
{
	if( B.Camera_Thread_running ) 
		return;

	if ( glob_m_pLog != NULL ) 
		glob_m_pLog->Write(" COctopusCamera::TakePicture() ");

	SetReadMode(4);             //set read out mode to image
	SetAcquisitionMode(1);      //set mode for single scan
	SetExposureTime(exposuretime_ms * 0.001 );
	SetTriggerMode( 0 );		// internal trigger

	B.CameraExpTime_ms = u32(exposuretime_ms);

	if ( B.Ampl_Conv )
		SetEMCCDGain( 0 );
	else
        SetEMCCDGain( gain );
    
	if ( B.Andor_new )
		SetShutter( 0, 1, 0, 0 ); // Always open
	else
		SetShutter( 1, 1, 0, 0 ); // Always Open
	
	if ( B.ROI_changed ) 
		SetROI();

	SetImage( B.ROI_actual.bin, B.ROI_actual.bin, \
		  B.ROI_actual.x1,  B.ROI_actual.x2,  \
		  B.ROI_actual.y1,  B.ROI_actual.y2 );
	
	if ( glob_m_pPictureDisplay == NULL || B.ROI_changed ) 
		OpenWindow();

	StartAcquisition();
	
	int status = 0;

	do{ GetStatus( &status ); } while ( status != DRV_IDLE );
    
	GetAcquiredData16( B.memory, B.H * B.W );

	if ( glob_m_pPictureDisplay != NULL ) 
		glob_m_pPictureDisplay->Update_Bitmap( B.memory, 1 );
	
}

/*********************************************************************************
* Start/stop continuous data acquisition
*********************************************************************************/

void COctopusCamera::StartMovie( void )
{	
	
	if ( glob_m_pLog != NULL ) 
		glob_m_pLog->Write(" COctopusCamera::StartMovie( void ) ");
	
	if( B.Camera_Thread_running ) 
		return;

	DisableDlgMovie();

	SetAcquisitionMode( 5 );		// set to run till abort
	SetReadMode( 4 );				// image mode

	SetExposureTime( float(u32_Exposure_Time_Movie_ms) * 0.001 );
	
	B.CameraExpTime_ms = u32_Exposure_Time_Movie_ms;

	SetKineticCycleTime( 0 );
	
	if ( B.Ampl_Conv )
		SetEMCCDGain( 0 );
	else
        	SetEMCCDGain( int(u16_Gain_Mult_Factor_Movie) );

	//if( TTL )
	//{	
	//	SetTriggerMode( 1 );		// base external trigger
	//	SetFastExtTrigger( 1 );     	// fast external trigger
	//}
	//else
		SetTriggerMode( 0 );		// internal trigger

	if ( FrameTransfer )
		SetFrameTransferMode( 1 );	// FT mode
	else
		SetFrameTransferMode( 0 );	// non FT mode

	if ( B.Andor_new )
		SetShutter( 0, 1, 0, 0 ); // Always open
	else
		SetShutter( 1, 1, 0, 0 ); // Always Open

	if ( B.ROI_changed ) 
		SetROI();
	
	SetImage( B.ROI_actual.bin, B.ROI_actual.bin, \
		  B.ROI_actual.x1,  B.ROI_actual.x2,  \
		  B.ROI_actual.y1,  B.ROI_actual.y2 );

	if ( glob_m_pPictureDisplay == NULL || B.ROI_changed ) 
		OpenWindow();

	StartAcquisition();

	StartWaitThread();

	//if( TTL )
	//	SetTimer( TIMER_TRIG, B.CameraExpTime_ms + 100, NULL ); 
}

void COctopusCamera::StopCameraThread() 
{
	KillWaitThread();

	AbortAcquisition();

	//KillTimer( TIMER_TRIG ); 

	if ( glob_m_pLog != NULL ) 
		glob_m_pLog->Write(" COctopusCamera::StopCameraThread() ");
}

/*********************************************************************************
********************	Utility functions ****************************************
*********************************************************************************/

void COctopusCamera::SetROI_To_Default( void )  // set ROI to default
{
	if ( glob_m_pLog != NULL ) 
		glob_m_pLog->Write(" COctopusCamera::SetROI_To_Default( void ) ");

	B.ROI_target.bin = 1;
	B.ROI_target.x1  = 1;
	B.ROI_target.y1  = 1;
	B.ROI_target.x2  = B.CCD_x_phys_e;
	B.ROI_target.y2  = B.CCD_y_phys_e;

	SetROI();
}

void COctopusCamera::SetROI( void ) 
{
	if ( glob_m_pLog != NULL ) 
		glob_m_pLog->Write(" COctopusCamera::SetROI( void ) ");

	// ROInew setting quality control
	if ((B.ROI_target.x2 > B.CCD_x_phys_e ) || (B.ROI_target.x2 <= B.ROI_target.x1 )) B.ROI_target.x2 = B.CCD_x_phys_e;
	if ((B.ROI_target.y2 > B.CCD_y_phys_e ) || (B.ROI_target.y2 <= B.ROI_target.y1 )) B.ROI_target.y2 = B.CCD_y_phys_e;
	if ((B.ROI_target.x1 > B.ROI_target.x2) || (B.ROI_target.x1 < 1)) B.ROI_target.x1 = 1;
	if ((B.ROI_target.y1 > B.ROI_target.y2) || (B.ROI_target.y1 < 1)) B.ROI_target.y1 = 1;

	//write the new
	B.ROI_actual.x1 = B.ROI_target.x1;	// serial start
	B.ROI_actual.x2 = B.ROI_target.x2;	// serial end
	B.ROI_actual.y1 = B.ROI_target.y1;	// parallel start
	B.ROI_actual.y2 = B.ROI_target.y2;	// parallel end

	BinChange();
}

void COctopusCamera::BinChange( void )
{
	if ( glob_m_pLog != NULL ) 
		glob_m_pLog->Write(" COctopusCamera::BinChange( void ) ");

	B.ROI_actual.bin = B.bin;
	
	while (((B.ROI_actual.x2 - B.ROI_actual.x1 + 1) % B.ROI_actual.bin) != 0) B.ROI_actual.x2--;
	while (((B.ROI_actual.y2 - B.ROI_actual.y1 + 1) % B.ROI_actual.bin) != 0) B.ROI_actual.y2--;
	
	B.W = (B.ROI_actual.x2 - B.ROI_actual.x1 + 1) / B.ROI_actual.bin;
	B.H = (B.ROI_actual.y2 - B.ROI_actual.y1 + 1) / B.ROI_actual.bin;

	//should be true anyway, but just in case.....
	B.ROI_changed = true;
}

void COctopusCamera::OpenWindow( void ) 
{
	if ( glob_m_pLog != NULL ) 
		glob_m_pLog->Write(" COctopusCamera::OpenWindow( void ) ");

	if( glob_m_pPictureDisplay != NULL ) 
	{
		glob_m_pPictureDisplay->DestroyWindow();
		delete glob_m_pPictureDisplay;
		glob_m_pPictureDisplay = NULL;
		if ( glob_m_pLog != NULL ) 
			glob_m_pLog->Write(" glob_m_pPictureDisplay != NULL ");
	}
	if( glob_m_pPictureDisplay == NULL ) 
	{
		B.ROI_changed = false;
		glob_m_pPictureDisplay = new COctopusPictureDisplay( this, B.W, B.H );
		glob_m_pPictureDisplay->Create( IDC_CAS_DISP );
		glob_m_pPictureDisplay->ShowWindow( SW_SHOW );
		glob_m_pPictureDisplay->Create_Bitmap(); 
		if ( glob_m_pLog != NULL ) 
			glob_m_pLog->Write(" glob_m_pPictureDisplay == NULL ");
	}
}

void COctopusCamera::OnKillfocusGeneral() 
{ 	
	UpdateData( true ); 
}

void COctopusCamera::OnKillfocusTempTarget() 
{
	UpdateData( true );
	SetTemperature ( m_CCD_target_temp );
}

void COctopusCamera::OnKillfocusManualGain() 
{
	UpdateData( true );
	B.manual_gain = m_IDC_MANUAL_GAIN;
}

void COctopusCamera::OnKillfocusPicturesPerFile() 
{
	UpdateData( true );
	B.pics_per_file = m_IDC_PICTURES_PER_FILE;
}

void COctopusCamera::OnBinning1x1() { m_DISPLAY_BIN_CHOICE = 0; B.bin = 1; BinChange(); }
void COctopusCamera::OnBinning2x2() { m_DISPLAY_BIN_CHOICE = 1; B.bin = 2; BinChange(); }
void COctopusCamera::OnBinning4x4() { m_DISPLAY_BIN_CHOICE = 2; B.bin = 4; BinChange(); }
void COctopusCamera::OnBinning8x8() { m_DISPLAY_BIN_CHOICE = 3; B.bin = 8; BinChange(); }

void COctopusCamera::OnVSSPEED_0() { m_DISPLAY_VSSPEED = 0; OnVSSPEED_Report(); }
void COctopusCamera::OnVSSPEED_1() { m_DISPLAY_VSSPEED = 1; OnVSSPEED_Report(); }
void COctopusCamera::OnVSSPEED_2() { m_DISPLAY_VSSPEED = 2; OnVSSPEED_Report(); }
void COctopusCamera::OnVSSPEED_3() { m_DISPLAY_VSSPEED = 3; OnVSSPEED_Report(); }
void COctopusCamera::OnVSSPEED_4() { m_DISPLAY_VSSPEED = 4; OnVSSPEED_Report(); }

void COctopusCamera::OnHSSPEED_0() { m_DISPLAY_HSSPEED = 0; OnHSSPEED_Report(); }
void COctopusCamera::OnHSSPEED_1() { m_DISPLAY_HSSPEED = 1; OnHSSPEED_Report(); }
void COctopusCamera::OnHSSPEED_2() { m_DISPLAY_HSSPEED = 2; OnHSSPEED_Report(); }
void COctopusCamera::OnHSSPEED_3() { m_DISPLAY_HSSPEED = 3; OnHSSPEED_Report(); }
void COctopusCamera::OnHSSPEED_4() { m_DISPLAY_HSSPEED = 4; OnHSSPEED_Report(); }
void COctopusCamera::OnHSSPEED_5() { m_DISPLAY_HSSPEED = 5; OnHSSPEED_Report(); }

void COctopusCamera::OnGL_0() 
{ 
	m_DISPLAY_GL = 0; 
	SetEMGainMode( 0 );
	UpdateData( false );
}

void COctopusCamera::OnGL_1() 
{ 
	if ( B.Andor_new ) 
	{
		m_DISPLAY_GL = 1; 
		SetEMGainMode( 2 );
	}
	else
	{
		m_DISPLAY_GL = 0; 
		SetEMGainMode( 0 );
	}
	UpdateData( false );
}

void COctopusCamera::OnGL_2() 
{ 
	if ( B.Andor_new ) 
	{
		m_DISPLAY_GL = 2; 
		SetEMGainMode( 3 );
	}
	else
	{
		m_DISPLAY_GL = 0; 
		SetEMGainMode( 0 );
	}
	UpdateData( false );
}

void COctopusCamera::OnVSSPEED_Report()
{
	SetVSSpeed( m_DISPLAY_VSSPEED ); 
}

void COctopusCamera::OnHSSPEED_Report()
{
	if ( m_DISPLAY_HSSPEED == 0 )
	{
		B.Ampl_Conv = false;
		SetOutputAmplifier( 0 ); // EMCCD
		SetADChannel( 0 );	 // 14 bit
		SetHSSpeed( 0, 0 );      // 10Mhz
		SetPreAmpGain( 1 );      // 5x
	} 
	else if ( m_DISPLAY_HSSPEED == 1 )
	{
		B.Ampl_Conv = false;
		SetOutputAmplifier( 0 ); // EMCCD
		SetADChannel( 0 );	 // 14 bit
		SetHSSpeed( 0, 1 );      // 5 Mhz
		SetPreAmpGain( 1 );	 // 5x
	} 
	else if ( m_DISPLAY_HSSPEED == 2 )
	{
		B.Ampl_Conv = false;
		SetOutputAmplifier( 0 ); // EMCCD
		SetADChannel( 0 );	 // 14 bit
		SetHSSpeed( 0, 2 );      // 3 Mhz
		SetPreAmpGain( 1 );	 // 5x
	} 
	else if ( m_DISPLAY_HSSPEED == 3 )
	{
		B.Ampl_Conv = false;
		SetOutputAmplifier( 0 ); // EMCCD
		SetADChannel( 1 );	 // 16 bit
		SetHSSpeed( 0, 3 );      // 1MHz
		SetPreAmpGain( 1 );	 // 5x
	} 
	else if ( m_DISPLAY_HSSPEED == 4 )
	{
		B.Ampl_Conv = true;
		SetOutputAmplifier( 1 ); // Conv
		SetADChannel( 1 );       // 16 bit
		SetHSSpeed( 1, 1 );      // 1 Mhz
	    SetPreAmpGain( 0 );		 // none
	} 
	else if ( m_DISPLAY_HSSPEED == 5 )
	{
		B.Ampl_Conv = true;
		SetOutputAmplifier( 1 ); // Conv 
		SetADChannel( 0 );       // 14 bit
		SetHSSpeed( 1, 0 );      // 3 Mhz
		SetPreAmpGain( 0 );	 // none
	}
}

void COctopusCamera::OnDisplayGainManual() 
{ 
	m_DISPLAY_GAIN_CHOICE = 0;
	UpdateData( false );
	B.automatic_gain = false;
}

void COctopusCamera::OnDisplayGainAutomatic() 
{ 
	m_DISPLAY_GAIN_CHOICE = 1;
	UpdateData( false );
	B.automatic_gain = true;  
}

void COctopusCamera::OnFileChange( void ) 
{
	if ( !B.savetofile ) 
		StartSaving();
	else 
		StopSaving();
}
/*
void COctopusCamera::OnFocusChange( void ) 
{
	if ( B.Camera_Thread_running == false ) 
		B.SetFocus = true;
}
*/
void COctopusCamera::StartSaving() 
{

	if ( glob_m_pLog != NULL ) 
		glob_m_pLog->Write(" COctopusCamera::StartSaving() ");

	if ( !B.savetofile ) 
	{	
		B.savetofile = true;
		m_status_save.SetBitmap( m_bmp_yes );
		if ( glob_m_pGoodClock == NULL ) return;
		B.savetime = (double)glob_m_pGoodClock->End();
		B.expt_frame = 0; //counter for saved frames
	}
}

void COctopusCamera::StopSaving()
{
	if ( glob_m_pLog != NULL ) 
		glob_m_pLog->Write(" COctopusCamera::StopSaving() ");

	if ( B.savetofile ) 
	{ 
		B.savetofile = false;
		m_status_save.SetBitmap( m_bmp_no );
	    if (glob_m_pPictureDisplay != NULL) 
			glob_m_pPictureDisplay->Close_The_File();
	}
}

void COctopusCamera::FileClose()
{
	if ( glob_m_pLog != NULL ) 
		glob_m_pLog->Write(" COctopusCamera::FileClose() ");

	if (glob_m_pPictureDisplay != NULL) 
			glob_m_pPictureDisplay->Close_The_File();
}

void COctopusCamera::OnSetPath( void ) 
{
	if ( glob_m_pLog != NULL ) 
		glob_m_pLog->Write(_T(" COctopusCamera::OnSetPath( void ) "));

	CFileDialog FileDlg( FALSE, _T("dth"), NULL, OFN_HIDEREADONLY, \
		_T("Octopus Header (*.dth)|*.dth|"), this);
    
	CString pathname;
	
	if( FileDlg.DoModal() == IDOK ) 
	{
		pathname   = FileDlg.GetPathName();
		pathname   = pathname.Left(pathname.Find(_T(".dth")));
		pathname_display = pathname;
		B.pathname       = pathname;
		UpdateData( false );
	}
}

void COctopusCamera::OnTimer( UINT nIDEvent ) 
{

	if( nIDEvent == TIMER_TEMP ) 
	{
		int status = 0;

		GetStatus( &status );
		
		if( status == DRV_IDLE ) 
		{
			GetTemperature( &m_CCD_current_temp );
			CString str;
			str.Format(_T("Current T: %d C"), m_CCD_current_temp);
			if( IsWindowVisible() ) 
				m_Temperature_Now.SetWindowText( str );
		}
	}

	CDialog::OnTimer(nIDEvent);
}

/************************************
 Read info from camera
 ***********************************/

void COctopusCamera::StartWaitThread( void )
{
	if ( glob_m_pLog != NULL ) 
		glob_m_pLog->Write(" COctopusCamera::StartWaitThread( void ) ");
	
	DWORD WaitThreadID;
	
	B.Camera_Thread_running = true;
	
	WaitThread = CreateThread( 0, 4096, FocusThread, NULL, 0, &WaitThreadID );
}

void COctopusCamera::KillWaitThread( void )
{
	if ( glob_m_pLog != NULL ) 
		glob_m_pLog->Write(" COctopusCamera::KillWaitThread( void ) ");

	if( !B.Camera_Thread_running ) return;

	B.Camera_Thread_running = false;

	WaitForSingleObject( WaitThread, 500 );
	
	Sleep( 500 );

	EnableDlg();
	
	if ( !CloseHandle( WaitThread ) ) 
		AfxMessageBox(_T("Error: could not close camera thread"));
}

DWORD WINAPI FocusThread(LPVOID lpParameter) 
{

	long first      = 0;
	long last       = 0;
	long validfirst = 0;
	long validlast  = 0;
	long retval     = 0;

	u16 NumberOfNewImages = 0;

	while( B.Camera_Thread_running ) 
	{		
		retval = GetNumberNewImages( &first, &last );
		
		// GetNumberNewImages is a weird function 
		// when there is one image to transfer, 
		// last == first, and thus number of images = 0;
		// likewise, transfer two images = ... 1, 1+1 and so forth

		NumberOfNewImages = u16(last - first + 1);

		if ( retval == DRV_NO_NEW_DATA )
		{
			//do nothing
		} 
		else if ( retval == DRV_SUCCESS && NumberOfNewImages >= 1 )
		{
			//let's get cracking...

			retval = GetImages16( first, last,               \
					B.memory, B.W * B.H * NumberOfNewImages, \
					&validfirst, &validlast);

			if ( retval == DRV_SUCCESS )
			{
				if ( glob_m_pPictureDisplay != NULL )
					glob_m_pPictureDisplay->Update_Bitmap( B.memory, NumberOfNewImages );
			} 
		}
	}
	return 0;	
}

/************************************
 User interface etc
 ***********************************/

void COctopusCamera::OnBnClickedCasFt()
{
    	int l_ChkBox = m_ctlFTCheckBox.GetCheck();
	
	if (l_ChkBox == 0) 
		FrameTransfer = false;
	else if (l_ChkBox == 1) 
		FrameTransfer = true;
}

BOOL COctopusCamera::OnCommand(WPARAM wParam, LPARAM lParam)
{
	int id    = LOWORD(wParam); // Notification code
	if( id == 2 ) return FALSE; // Trap ESC key
	if( id == 1 ) return FALSE; // Trap RTN key
    	return CDialog::OnCommand(wParam, lParam);
}

void COctopusCamera::EnableDlg( void ) 
{
	GetDlgItem( IDC_CAS_STARTSTOP_PICTURE )->EnableWindow( true  );
	GetDlgItem( IDC_CAS_START_MOVIE )->EnableWindow( true  );
	GetDlgItem( IDC_CAS_STOP_MOVIE )->EnableWindow( false );
	GetDlgItem( IDC_CAS_EXPOSURE_TIME_SINGLE )->EnableWindow( true  );
	GetDlgItem( IDC_CAS_EXPOSURE_TIME_MOVIE )->EnableWindow( true  );
	GetDlgItem( IDC_CAS_PICTURES_PER_FILE )->EnableWindow( true  );
	GetDlgItem( IDC_CAS_TEMP_TARGET	)->EnableWindow( true  );
	GetDlgItem( IDC_CAS_GAIN_SINGLE )->EnableWindow( true  );
	GetDlgItem( IDC_CAS_GAIN_MOVIE )->EnableWindow( true  );
	GetDlgItem( IDC_CAS_DISPLAY_BIN_1 )->EnableWindow( true  );
	GetDlgItem( IDC_CAS_DISPLAY_BIN_2 )->EnableWindow( true  );
	GetDlgItem( IDC_CAS_DISPLAY_BIN_4 )->EnableWindow( true  );
	GetDlgItem( IDC_CAS_DISPLAY_BIN_8 )->EnableWindow( true  );
	GetDlgItem( IDC_CAS_SETPATH )->EnableWindow( true  );
	GetDlgItem( IDC_CAS_HSSPEED_0 )->EnableWindow( true );
	GetDlgItem( IDC_CAS_HSSPEED_1 )->EnableWindow( true );
	GetDlgItem( IDC_CAS_HSSPEED_2 )->EnableWindow( true );
	GetDlgItem( IDC_CAS_HSSPEED_3 )->EnableWindow( true );
	GetDlgItem( IDC_CAS_HSSPEED_4 )->EnableWindow( true );
	GetDlgItem( IDC_CAS_HSSPEED_5 )->EnableWindow( true );
	GetDlgItem( IDC_CAS_VSSPEED_0 )->EnableWindow( true );
	GetDlgItem( IDC_CAS_VSSPEED_1 )->EnableWindow( true );
	GetDlgItem( IDC_CAS_VSSPEED_2 )->EnableWindow( true );
	GetDlgItem( IDC_CAS_VSSPEED_3 )->EnableWindow( true );
	GetDlgItem( IDC_CAS_VSSPEED_4 )->EnableWindow( true );
}

void COctopusCamera::DisableDlgMovie( void ) 
{
	GetDlgItem( IDC_CAS_STARTSTOP_PICTURE )->EnableWindow( false );
	GetDlgItem( IDC_CAS_STOP_MOVIE )->EnableWindow( true  );
	GetDlgItem( IDC_CAS_START_MOVIE )->EnableWindow( false );
	DisableDlg();
}

void COctopusCamera::DisableDlg( void ) 
{
	GetDlgItem( IDC_CAS_HSSPEED_0 )->EnableWindow( false );
	GetDlgItem( IDC_CAS_HSSPEED_1 )->EnableWindow( false );
	GetDlgItem( IDC_CAS_HSSPEED_2 )->EnableWindow( false );
	GetDlgItem( IDC_CAS_HSSPEED_3 )->EnableWindow( false );
	GetDlgItem( IDC_CAS_HSSPEED_4 )->EnableWindow( false );
	GetDlgItem( IDC_CAS_HSSPEED_5 )->EnableWindow( false );

	GetDlgItem( IDC_CAS_VSSPEED_0 )->EnableWindow( false );
	GetDlgItem( IDC_CAS_VSSPEED_1 )->EnableWindow( false );
	GetDlgItem( IDC_CAS_VSSPEED_2 )->EnableWindow( false );
	GetDlgItem( IDC_CAS_VSSPEED_3 )->EnableWindow( false );
	GetDlgItem( IDC_CAS_VSSPEED_4 )->EnableWindow( false );

	GetDlgItem( IDC_CAS_EXPOSURE_TIME_SINGLE )->EnableWindow( false );
	GetDlgItem( IDC_CAS_EXPOSURE_TIME_MOVIE	)->EnableWindow( false );
	GetDlgItem( IDC_CAS_PICTURES_PER_FILE )->EnableWindow( false );
	GetDlgItem( IDC_CAS_TEMP_TARGET	)->EnableWindow( false );
	GetDlgItem( IDC_CAS_GAIN_SINGLE	)->EnableWindow( false );
	GetDlgItem( IDC_CAS_GAIN_MOVIE )->EnableWindow( false );
	GetDlgItem( IDC_CAS_DISPLAY_BIN_1 )->EnableWindow( false );
	GetDlgItem( IDC_CAS_DISPLAY_BIN_2 )->EnableWindow( false );
	GetDlgItem( IDC_CAS_DISPLAY_BIN_4 )->EnableWindow( false );
	GetDlgItem( IDC_CAS_DISPLAY_BIN_8 )->EnableWindow( false );
	GetDlgItem( IDC_CAS_SETPATH )->EnableWindow( false );
}
