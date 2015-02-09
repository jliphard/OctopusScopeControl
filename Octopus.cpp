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
////////////////////////////////////////////////////////////////////////////////
// Octopus.cpp : Defines the class behaviors for the application.
////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Octopus.h"
#include "MainFrm.h"
#include "OctopusView.h"

COctopusApp::COctopusApp() {}

COctopusApp theApp;

BOOL COctopusApp::InitInstance() 
{

	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);
	//EnableVisualStyles();
	
	AfxEnableControlContainer();

	LoadStdProfileSettings();

	CSingleDocTemplate* pDocTemplate;
	
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(COctopusDoc),
		RUNTIME_CLASS(CMainFrame),
		RUNTIME_CLASS(COctopusView)
		);
	
	AddDocTemplate(pDocTemplate);

	CCommandLineInfo cmdInfo;
	
	ParseCommandLine(cmdInfo);
	
	if (!ProcessShellCommand(cmdInfo)) return false;

	return true;
}

int COctopusApp::ExitInstance() {return CWinApp::ExitInstance();}
