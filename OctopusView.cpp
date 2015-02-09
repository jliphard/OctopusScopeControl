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
***************************   OctopusView           *******************************
**********************************************************************************/

#include "stdafx.h"
#include "MainFrm.h"
#include "Octopus.h"
#include "OctopusView.h"
#include "OctopusClock.h"
#include "OctopusCameraDlg.h"
#include "OctopusShutter.h"
#include "OctopusGlobals.h"
#include "OctopusScript.h"
#include "OctopusStage686.h"
#include "OctopusMultifunction.h"
#include "OctopusLog.h"
#include "OctopusSamplePiezo.h"
#include "OctopusScope.h"
#include "OctopusLasers.h"

COctopusCamera*            glob_m_pCamera          = NULL;
COctopusShutterAndWheel*   glob_m_pShutterAndWheel = NULL;
COctopusScript*            glob_m_pScript          = NULL;
COctopusMultifunction*	   glob_m_pNI              = NULL;
COctopusLasers*            glob_m_pLasers          = NULL;
COctopusStage686*          glob_m_pStage686		   = NULL;
COctopusSamplePiezo*       glob_m_pSamplePiezo     = NULL;
COctopusScope*             glob_m_pScope		   = NULL;
//COctopusMotor*             glob_m_pMotor		   = NULL;

COctopusGoodClock          GoodClock;
COctopusGoodClock*         glob_m_pGoodClock = &GoodClock;
COctopusLog                Log;
COctopusLog*               glob_m_pLog = &Log;
COctopusGlobals            B;

IMPLEMENT_DYNCREATE(COctopusView, CFormView)

BEGIN_MESSAGE_MAP(COctopusView, CFormView)
	ON_BN_CLICKED(IDC_STATUS_OPEN_ANDOR,   OnOpenAndor)
	ON_BN_CLICKED(IDC_STATUS_OPEN_SCRIPT,  OnOpenScript)
	ON_BN_CLICKED(IDC_STATUS_OPEN_WHEEL,   OnOpenWheel)
	ON_BN_CLICKED(IDC_STATUS_OPEN_PIEZO,   OnOpenPiezo)
	ON_BN_CLICKED(IDC_STATUS_OPEN_SCOPE,   OnOpenScope)
	ON_BN_CLICKED(IDC_STATUS_OPEN_STAGE,   OnOpenStage686)
END_MESSAGE_MAP()

void COctopusView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange( pDX );
}  

COctopusView::COctopusView():CFormView(COctopusView::IDD) 
{
}

void COctopusView::OnInitialUpdate() 
{
	// toyscope
	// scope = 1 Phil's toyscope
	// scope = 3 Pol scope
	scope = 1;
	
	if ( scope == 1 ) //Cutting scope
	{
		//set this to true if you have a iXon Plus.
		B.Andor_new = true;

		//GetDlgItem( IDC_STATUS_OPEN_PIEZO )->EnableWindow( true );
		//GetDlgItem( IDC_STATUS_OPEN_PIEZO )->ShowWindow(   true );

		//GetDlgItem( IDC_STATUS_OPEN_WHEEL )->EnableWindow( true );
		//GetDlgItem( IDC_STATUS_OPEN_WHEEL )->ShowWindow(   true );

		OnOpenMultifunction(); 
		OnOpenLasers(); 
	}
	else if ( scope == 2 ) //Rebecca's scope
	{
		B.Andor_new = false;
		GetDlgItem( IDC_STATUS_OPEN_LASERS )->EnableWindow( false );
		GetDlgItem( IDC_STATUS_OPEN_LASERS )->ShowWindow(   false );
	}
	else if ( scope == 3 ) //Pol scope
	{
		B.Andor_new = true;

		GetDlgItem( IDC_STATUS_OPEN_LASERS )->EnableWindow( true );
		GetDlgItem( IDC_STATUS_OPEN_LASERS )->ShowWindow(   true );
		GetDlgItem( IDC_STATUS_OPEN_STAGE  )->EnableWindow( true );
		GetDlgItem( IDC_STATUS_OPEN_STAGE  )->ShowWindow(   true );
		GetDlgItem( IDC_STATUS_OPEN_WHEEL  )->EnableWindow( true );
		GetDlgItem( IDC_STATUS_OPEN_WHEEL  )->ShowWindow(   true );

		OnOpenMultifunction();

	}
	else
	{
		B.Andor_new = false;
	}

	CFormView::OnInitialUpdate();
}

COctopusView::~COctopusView() 
{
	if( glob_m_pCamera != NULL ) 
	{
		if ( B.Camera_Thread_running ) 
			glob_m_pCamera->StopCameraThread();
		glob_m_pCamera->DestroyWindow();
		delete glob_m_pCamera;
		glob_m_pCamera = NULL;
	}
	if( glob_m_pShutterAndWheel != NULL ) 
	{
		glob_m_pShutterAndWheel->DestroyWindow();
		delete glob_m_pShutterAndWheel;
		glob_m_pShutterAndWheel = NULL;
	}
	if( glob_m_pNI != NULL ) 
	{
		glob_m_pNI->DestroyWindow();
		delete glob_m_pNI;
		glob_m_pNI = NULL;
	}
	if( glob_m_pScript != NULL ) 
	{
		glob_m_pScript->DestroyWindow();
	}
	if( glob_m_pStage686 != NULL ) 
	{
		glob_m_pStage686->DestroyWindow();
		delete glob_m_pStage686;
		glob_m_pStage686 = NULL;
	}
	if( glob_m_pSamplePiezo != NULL ) 
	{
		glob_m_pSamplePiezo->DestroyWindow();
		delete glob_m_pSamplePiezo;
		glob_m_pSamplePiezo = NULL;
	}
	if( glob_m_pScope != NULL ) 
	{
		glob_m_pScope->DestroyWindow();
		delete glob_m_pScope;
		glob_m_pScope = NULL;
	}
	if( glob_m_pLasers != NULL ) 
	{
		glob_m_pLasers->DestroyWindow();
		delete glob_m_pLasers;
		glob_m_pLasers = NULL;
	}
}

void COctopusView::OnOpenAndor( void ) 
{	
	GetDlgItem( IDC_STATUS_OPEN_ANDOR )->EnableWindow( false );

	if( glob_m_pCamera == NULL ) 
	{
		glob_m_pCamera = new COctopusCamera( this );
	} 
	else 
	{
		if ( B.Camera_Thread_running )
		{
			AfxMessageBox(_T("The camera is running in movie mode,\n and you just tried to close it!\nPlease turn it off first!"));
		}
		else if( glob_m_pCamera->IsWindowVisible() ) 
		{
			glob_m_pCamera->DestroyWindow();
			delete glob_m_pCamera;
			glob_m_pCamera = NULL;
		} 
		else 
		{
			glob_m_pCamera->ShowWindow( SW_RESTORE );
		}
	}
	GetDlgItem( IDC_STATUS_OPEN_ANDOR )->EnableWindow( true );
}

void COctopusView::OnOpenScript( void ) 
{	
	GetDlgItem( IDC_STATUS_OPEN_SCRIPT )->EnableWindow( false );

	if( glob_m_pScript == NULL ) 
	{
		glob_m_pScript = new COctopusScript( this );
	} 
	else 
	{
		if( glob_m_pScript->IsWindowVisible() ) 
		{
			glob_m_pScript->DestroyWindow();
			delete glob_m_pScript;
			glob_m_pScript = NULL;
		} 
		else 
		{
			glob_m_pScript->ShowWindow( SW_RESTORE );
		}
	}
	GetDlgItem( IDC_STATUS_OPEN_SCRIPT )->EnableWindow( true );
}

void COctopusView::OnOpenWheel( void ) 
{	

	GetDlgItem( IDC_STATUS_OPEN_WHEEL )->EnableWindow( false );

	if( glob_m_pShutterAndWheel == NULL ) 
	{
		
		glob_m_pShutterAndWheel = new COctopusShutterAndWheel( this );
		
		if ( B.load_wheel_failed ) 
		{ // close it
			glob_m_pShutterAndWheel->DestroyWindow();
			delete glob_m_pShutterAndWheel;
			glob_m_pShutterAndWheel = NULL;
		}
	} 
	else 
	{
		if( glob_m_pShutterAndWheel->IsWindowVisible() ) 
		{
			glob_m_pShutterAndWheel->DestroyWindow();
			delete glob_m_pShutterAndWheel;
			glob_m_pShutterAndWheel = NULL;
		} 
		else 
		{
			glob_m_pShutterAndWheel->ShowWindow( SW_RESTORE );
		}
	}

	GetDlgItem( IDC_STATUS_OPEN_WHEEL )->EnableWindow( true );

}

void COctopusView::OnOpenMultifunction( void ) 
{	
	
	if( glob_m_pNI == NULL ) 
	{
		glob_m_pNI = new COctopusMultifunction( this );
	} 
	else 
	{
		if( glob_m_pNI->IsWindowVisible() ) 
		{
			glob_m_pNI->DestroyWindow();
			delete glob_m_pNI;
			glob_m_pNI = NULL;
		} 
		else 
		{
			glob_m_pNI->ShowWindow( SW_RESTORE );
		}
	}
}

void COctopusView::OnOpenScope( void ) 
{	
	
	GetDlgItem( IDC_STATUS_OPEN_SCOPE )->EnableWindow( false );

	if( glob_m_pScope == NULL ) 
	{
		glob_m_pScope = new COctopusScope( this );
	} 
	else 
	{
		if( glob_m_pScope->IsWindowVisible() ) 
		{
			glob_m_pScope->run_the_wait_thread = false;
			//Sleep(3000);
			AfxMessageBox(_T("Closing the thread...."));
			glob_m_pScope->Close();
			glob_m_pScope->DestroyWindow();
			delete glob_m_pScope;
			glob_m_pScope = NULL;
		} 
		else 
		{
			glob_m_pScope->ShowWindow( SW_RESTORE );
		}
	}

	GetDlgItem( IDC_STATUS_OPEN_SCOPE )->EnableWindow( true );
	
}

void COctopusView::OnOpenStage686( void ) 
{	

	GetDlgItem( IDC_STATUS_OPEN_STAGE )->EnableWindow( false );

	if( glob_m_pStage686 == NULL ) 
	{
		glob_m_pStage686 = new COctopusStage686( this );

		debug.Format(_T(" PI 686 stage class opened "));
		if ( glob_m_pLog != NULL ) glob_m_pLog->Write(debug);
	} 
	else {
		if( glob_m_pStage686->IsWindowVisible() ) {
			glob_m_pStage686->Close();
			glob_m_pStage686->DestroyWindow();
			delete glob_m_pStage686;
			glob_m_pStage686 = NULL;
			debug.Format(_T(" PI 686 stage class closed "));
			if ( glob_m_pLog != NULL ) glob_m_pLog->Write(debug);
		} else {
			glob_m_pStage686->ShowWindow( SW_RESTORE );
		}
	}

	GetDlgItem( IDC_STATUS_OPEN_STAGE )->EnableWindow( true );
}

void COctopusView::OnOpenPiezo( void ) 
{	

	GetDlgItem( IDC_STATUS_OPEN_PIEZO )->EnableWindow( false );

	if( glob_m_pSamplePiezo == NULL ) 
	{
		glob_m_pSamplePiezo = new COctopusSamplePiezo( this );
		debug.Format(_T(" Obj Piezo class opened "));
		if ( glob_m_pLog != NULL ) glob_m_pLog->Write(debug);
	} 
	else 
	{
		if( glob_m_pSamplePiezo->IsWindowVisible() ) 
		{
			glob_m_pSamplePiezo->Close();
			glob_m_pSamplePiezo->DestroyWindow();
			delete glob_m_pSamplePiezo;
			glob_m_pSamplePiezo = NULL;
			debug.Format(_T(" Obj Piezo class closed "));
			if ( glob_m_pLog != NULL ) glob_m_pLog->Write(debug);
		} 
		else 
		{
			glob_m_pSamplePiezo->ShowWindow( SW_RESTORE );
		}
	}

/*
	if( glob_m_pFocus == NULL ) 
	{
		glob_m_pFocus = new COctopusFocus( this );
		debug.Format(_T(" Focus class opened "));
		if ( glob_m_pLog != NULL ) glob_m_pLog->Write(debug);
	} 
	else 
	{
		if( glob_m_pFocus->IsWindowVisible() ) 
		{
			glob_m_pFocus->Close();
			glob_m_pFocus->DestroyWindow();
			delete glob_m_pFocus;
			glob_m_pFocus = NULL;
			debug.Format(_T(" Focus class closed "));
			if ( glob_m_pLog != NULL ) glob_m_pLog->Write(debug);
		} 
		else 
		{
			glob_m_pFocus->ShowWindow( SW_RESTORE );
		}
	}
*/

	GetDlgItem( IDC_STATUS_OPEN_PIEZO )->EnableWindow( true );
}

void COctopusView::OnOpenMotor( void ) 
{	
/*
	GetDlgItem( IDC_STATUS_OPEN_MOTOR )->EnableWindow( false );

	if( glob_m_pMotor == NULL ) 
	{
		glob_m_pMotor = new COctopusMotor( this );
		debug.Format(_T(" Motor class opened "));
		if ( glob_m_pLog != NULL ) glob_m_pLog->Write(debug);
	} 
	else 
	{
		if( glob_m_pMotor->IsWindowVisible() ) 
		{
			glob_m_pMotor->Close();
			glob_m_pMotor->DestroyWindow();
			delete glob_m_pMotor;
			glob_m_pMotor = NULL;
			debug.Format(_T(" Motor class closed "));
			if ( glob_m_pLog != NULL ) glob_m_pLog->Write(debug);
		} 
		else 
		{
			glob_m_pMotor->ShowWindow( SW_RESTORE );
		}
	}

	GetDlgItem( IDC_STATUS_OPEN_MOTOR )->EnableWindow( true );
*/
}


void COctopusView::OnOpenLasers( void ) 
{	

//	GetDlgItem( IDC_STATUS_OPEN_LASERS )->EnableWindow( false );

	if( glob_m_pLasers == NULL ) 
	{
		glob_m_pLasers = new COctopusLasers( this );
		
		if ( !B.Lasers_loaded ) { // close it
			glob_m_pLasers->DestroyWindow();
			delete glob_m_pLasers;
			glob_m_pLasers = NULL;
		}
	} else {
		if( glob_m_pLasers->IsWindowVisible() ) {
			glob_m_pLasers->DestroyWindow();
			delete glob_m_pLasers;
			glob_m_pLasers = NULL;
		} 
		else {
			glob_m_pLasers->ShowWindow( SW_RESTORE );
		}
	}

//	GetDlgItem( IDC_STATUS_OPEN_LASERS )->EnableWindow( true );
	
}
