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
#include "OctopusScope.h"
#include "OctopusGlobals.h"
#include "cport.h"

extern COctopusGlobals B;

DWORD WINAPI ScopeCommandBuffer( LPVOID lpParameter )
{
	COctopusScope* this_ = (COctopusScope*)lpParameter; 

	while( this_->run_the_wait_thread == true ) 
	{
		
		if ( this_ != NULL ) this_->GetPosition();
		
		Sleep(300);

		if (  this_ != NULL && this_->m_SeqList.GetCount() > 0 ) 
			this_->ExecuteNextCommand();

	}

	return 0;
}

COctopusScope::COctopusScope(CWnd* pParent)
	: CDialog(COctopusScope::IDD, pParent)
{    
	position_now           = 0.0;
	stepsize_10nm          = 10;
	pPortScope             = NULL;
	Scope_initialized      = false;
	Scope_working          = false;
	B.position_z_objective = 0;
	B.Scope_loaded         = false;
	g_objective_now		   = 1;
	B.Scope_epiwheel       = 0;
	B.Scope_objective      = 0;
	g_cube_now			   = 1;
	g_NDfilter_now         = 1;
	g_cond_now             = 1;
	g_path_now             = 1;
	position_to_return_to  = 450000;
	brightfield_initial    = 25;
	g_volt                 = brightfield_initial;
	bfon                   = false;

	if( Create(COctopusScope::IDD, pParent) ) 
		ShowWindow( SW_SHOW );

	m_Slider.SetRange( 0, 120 );
	m_Slider.SetPos( brightfield_initial );
	m_Slider.SetTicFreq( 5 );

	DisplayBFIntensity( brightfield_initial );
}

BOOL COctopusScope::OnInitDialog() 
{
	CDialog::OnInitDialog();

	if( Init() == true )
	{
		wait_thread_cmd          = NULL;
		run_the_wait_thread      = true;
		DWORD wait_thread_cmd_id = NULL;
		wait_thread_cmd = CreateThread( 0, 4096, ScopeCommandBuffer, (LPVOID)this, 0, &wait_thread_cmd_id );
		
		OnObjectiveStepSize2();

		BrightField( brightfield_initial );
	};

	return TRUE;
}

bool COctopusScope::Init( void )
{

	if( Scope_initialized ) 
	{
		AfxMessageBox(_T("Microscope already init()ed..."));
		return false;
	}

	pPortScope = new CPort;
	
	if( pPortScope == NULL ) 
	{
		AfxMessageBox(_T("pPortScope is a Null pointer..."));
		return false;
	}

	pPortScope->mPort.Format(_T("COM1"));

	if( pPortScope->OpenCPort() )//open and setup the comm port
	{
		
		///AfxMessageBox(_T("Opening the scope"));

		Scope_initialized = true;

		WriteScope(_T("1UNIT?\r\n")); //anyone out there?	
		Sleep(300);

		if ( ReadScope( 9 ).Find(_T("1UNIT IX2")) >= 0 ) 
		{

			ReadScope( 24+2 ); //clear out the remaining chars
			
			WriteScope(_T("1LOG IN\r\n"));
			WaitTill(_T("1LOG +\r\n"));
			
			//CString temp(_T(""));

			WriteScope(_T("2LOG IN\r\n"));
			WaitTill(_T("2LOG +\r\n"));

			WriteScope(_T("1SW ON\r\n"));
			WaitTill(_T("1SW +\r\n"));
			ReadScope(7);

			WriteScope(_T("1SNDOB ON\r\n"));
			WaitTill(("1SNDOB +\r\n"));

			WriteScope(_T("2maxspd 70000,300000,250\r\n"));
			WaitTill(_T("2maxspd +\r\n"));

			WriteScope("2JOG ON\r\n");
			WaitTill("2JOG +\r\n");

			WriteScope("2NEARLMT 900000\r\n");
			WaitTill("2NEARLMT +\r\n");

			WriteScope("2FARLMT 100\r\n");
			WaitTill("2FARLMT +\r\n");

			WriteScope("2JOGSNS 7\r\n"); //something like 100 microns per turn
			WaitTill("2JOGSNS +\r\n");

			WriteScope("2joglmt ON\r\n");
			WaitTill("2joglmt + \r\n"); //that extra space is NOT a typo - that is a bug in the olympus firmwarwe

			//WriteScope("1LMPSW ON\r\n");
			//WaitTill("1LMPSW +\r\n");

			WriteScope("1LMPSW?\r\n");
			//WaitTill("1LMPSW +\r\n");

			CString temp(_T(""));
			temp = ReadScope(9);
			CString lampstate = _T("");
			AfxExtractSubString(lampstate, temp, 1,' ');

			if ( lampstate == _T("OF") )
			{
				//AfxMessageBox(_T("Lamp is off - enabling it"));
				ReadScope(3);
				WriteScope("1LMPSW ON\r\n");
				WaitTill("1LMPSW +\r\n");
			} 
			else
			{
				//AfxMessageBox(_T("Lamp is already enabled - all good!"));
				ReadScope(2);
			}

			g_objective_now = GetObj();
			g_cube_now      = GetMU();
			g_cond_now      = GetCond();
			g_path_now      = GetPath();
			g_NDfilter_now  = GetBF();
			
			m_Radio_OB   = g_objective_now - 1; 
			m_Radio_MU   = g_cube_now      - 1; 
			m_Radio_CO   = g_cond_now      - 1;
			m_Radio_PA   = g_path_now      - 1; 
			m_Radio_BF   = g_NDfilter_now  - 1; 
		
			B.Scope_objective = g_objective_now;
			B.Scope_epiwheel  = g_cube_now;

			m_SeqList.ResetContent();

			B.Scope_loaded = true;

			UpdateData( false );

			return true;
		} 
		else 
		{
			Close();
			return false;
		}
	}
	else 
	{
		Close();
		return false;
	}
}

void COctopusScope::DoDataExchange(CDataExchange* pDX) 
{
	CDialog::DoDataExchange( pDX );
	DDX_Radio(pDX,		IDC_SCOPE_SSIZE_1,			        m_Radio_S);
	DDX_Radio(pDX,		IDC_SCOPE_OB_1,			            m_Radio_OB);
	DDX_Radio(pDX,		IDC_SCOPE_CO_1,			            m_Radio_CO);
	DDX_Radio(pDX,		IDC_SCOPE_MU_1,			            m_Radio_MU);
	DDX_Radio(pDX,		IDC_SCOPE_BF_1,		                m_Radio_BF);
	DDX_Radio(pDX,		IDC_SCOPE_PATH_EYE,		            m_Radio_PA);
	DDX_Control(pDX,	IDC_SCOPE_POS,				        m_Pos);
	DDX_Control(pDX,	IDC_SCOPE_INTENSITY_SLIDER,		    m_Slider);
	DDX_Control(pDX,	IDC_SCOPE_INTENSITY_SLIDER_SETTING, m_Slider_Setting);
	DDX_Control(pDX,	IDC_SCOPE_LIST_SEQUENCE,			m_SeqList);
}  

BEGIN_MESSAGE_MAP(COctopusScope, CDialog)
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_SCOPE_INTENSITY_SLIDER, OnNMCustomdrawExecute)
	ON_BN_CLICKED(IDC_SCOPE_DOWN,	     Z_StepDown)
	ON_BN_CLICKED(IDC_SCOPE_UP,		     Z_StepUp)
	ON_BN_CLICKED(IDC_SCOPE_ESC,		 ObjToBottom)
	ON_BN_CLICKED(IDC_SCOPE_RTN,		 ObjReturn)
	ON_BN_CLICKED(IDC_SCOPE_SSIZE_1,     OnObjectiveStepSize1)
	ON_BN_CLICKED(IDC_SCOPE_SSIZE_2,     OnObjectiveStepSize2)
	ON_BN_CLICKED(IDC_SCOPE_SSIZE_3,     OnObjectiveStepSize3)
	ON_BN_CLICKED(IDC_SCOPE_SSIZE_4,     OnObjectiveStepSize4)
	ON_BN_CLICKED(IDC_SCOPE_OB_1,        OnObj_1)
	ON_BN_CLICKED(IDC_SCOPE_OB_2,        OnObj_2)
	ON_BN_CLICKED(IDC_SCOPE_OB_3,        OnObj_3)
	ON_BN_CLICKED(IDC_SCOPE_OB_4,        OnObj_4)
	ON_BN_CLICKED(IDC_SCOPE_OB_5,        OnObj_5)
	ON_BN_CLICKED(IDC_SCOPE_OB_6,        OnObj_6)
	ON_BN_CLICKED(IDC_SCOPE_MU_1,        OnMU_1)
	ON_BN_CLICKED(IDC_SCOPE_MU_2,        OnMU_2)
	ON_BN_CLICKED(IDC_SCOPE_MU_3,        OnMU_3)
	ON_BN_CLICKED(IDC_SCOPE_MU_4,        OnMU_4)
	ON_BN_CLICKED(IDC_SCOPE_MU_5,        OnMU_5)
	ON_BN_CLICKED(IDC_SCOPE_MU_6,        OnMU_6)
	ON_BN_CLICKED(IDC_SCOPE_CO_1,        OnCO_1)
	ON_BN_CLICKED(IDC_SCOPE_CO_2,        OnCO_2)
	ON_BN_CLICKED(IDC_SCOPE_CO_3,        OnCO_3)
	ON_BN_CLICKED(IDC_SCOPE_CO_4,        OnCO_4)
	ON_BN_CLICKED(IDC_SCOPE_CO_5,        OnCO_5)
	ON_BN_CLICKED(IDC_SCOPE_CO_6,        OnCO_6)
	ON_BN_CLICKED(IDC_SCOPE_BF_1,        OnBF_1)
	ON_BN_CLICKED(IDC_SCOPE_BF_2,        OnBF_2)
	ON_BN_CLICKED(IDC_SCOPE_BF_3,        OnBF_3)
	ON_BN_CLICKED(IDC_SCOPE_BF_4,        OnBF_4)
	ON_BN_CLICKED(IDC_SCOPE_BF_5,        OnBF_5)
	ON_BN_CLICKED(IDC_SCOPE_BF_6,        OnBF_6)
	ON_BN_CLICKED(IDC_SCOPE_PATH_EYE,    OnPathEye)
	ON_BN_CLICKED(IDC_SCOPE_PATH_CAMERA, OnPathCamera)
	ON_BN_CLICKED(IDC_SCOPE_LIGHT_ONOFF, OnLightOnOff)

END_MESSAGE_MAP()

/**************************************************************************************
SHUTDOWN
**************************************************************************************/

COctopusScope::~COctopusScope() 
{  
}

void COctopusScope::Close() 
{  

	run_the_wait_thread = false;

	WaitForSingleObject( wait_thread_cmd, 5000 ); //timeout after 5 seconds

	Sleep( 1000 );

	WriteScope(_T("2STOP\r\n"));

	Sleep(30);
	
	WriteScope(_T("1LOG OUT\r\n"));

	Sleep(30);

	WriteScope(_T("2LOG OUT\r\n"));
	
	Sleep(30);

	Scope_initialized = false;

	B.Scope_loaded    = false;

	if ( pPortScope != NULL ) 
	{
		delete pPortScope;
		pPortScope = NULL;
	}	
}

/**************************************************************************************
OBJECTIVE Z SETTING
**************************************************************************************/

void COctopusScope::Z_StepDown( void ) 
{ 
	m_SeqList.AddString(_T("2JOG OFF\r\n"));

	CString temp;
	temp.Format(_T("2MOV F,%d\r\n"),stepsize_10nm ); //internal units of scope
	m_SeqList.AddString(temp);
	
	m_SeqList.AddString(_T("2JOG ON\r\n"));
}
void COctopusScope::Z_StepUp( void ) 
{ 
	m_SeqList.AddString(_T("2JOG OFF\r\n"));

	CString temp;
	temp.Format(_T("2MOV N,%d\r\n"),stepsize_10nm ); //internal units of scope
	m_SeqList.AddString(temp);
	
	m_SeqList.AddString(_T("2JOG ON\r\n"));
}

void COctopusScope::Z_GoToRel( double step_microns ) 
{ 
	m_SeqList.AddString(_T("2JOG OFF\r\n"));

	CString temp;
	temp.Format(_T("2MOV N,%d\r\n"), s32(step_microns * 100) ); //internal units of scope
	m_SeqList.AddString(temp);
	
	m_SeqList.AddString(_T("2JOG ON\r\n"));
}

void COctopusScope::Z_GoTo( double target_microns ) 
{ 

	if ( target_microns < 100  ) target_microns =  100;
	if ( target_microns > 9000 ) target_microns = 9000;
	
	m_SeqList.AddString(_T("2JOG OFF\r\n"));

	CString temp;
	temp.Format(_T("2MOV d,%d\r\n"), u32(target_microns * 100) ); //internal units of scope
	m_SeqList.AddString(temp);
	
	m_SeqList.AddString(_T("2JOG ON\r\n"));
}

void COctopusScope::ObjToBottom( void ) 
{ 
	//this is where we will go back to, once we are done.
	position_to_return_to = u32(position_now); 	
	
	//move to the bottom
	CString temp;
	temp.Format(_T("2MOV d,%d\r\n"), 10000 );
	m_SeqList.AddString(temp);
}

void COctopusScope::ObjReturn( void ) 
{ 
	//move back to where we were
	CString temp;
	temp.Format(_T("2MOV d,%d\r\n"), position_to_return_to );
	m_SeqList.AddString(temp);
}

void COctopusScope::OnObjectiveStepSize1() { m_Radio_S = 0; stepsize_10nm =   1; UpdateData( false ); }; //0.01 micron
void COctopusScope::OnObjectiveStepSize2() { m_Radio_S = 1; stepsize_10nm =   5; UpdateData( false ); }; //0.05 micron
void COctopusScope::OnObjectiveStepSize3() { m_Radio_S = 2; stepsize_10nm =  10; UpdateData( false ); }; //0.10 micron
void COctopusScope::OnObjectiveStepSize4() { m_Radio_S = 3; stepsize_10nm = 100; UpdateData( false ); }; //1.00 micron

void COctopusScope::ExecuteNextCommand( void )
{
	CString command;
	
	m_SeqList.GetText(0, command);
	
	if (command.Left(4) == _T("1OB "))
	{
		int i = atoi( command.Mid(3, 2) ); //!!!!this is zero based, hence 4 and not 5!!!!
		ObjectiveProc( i );
		m_SeqList.DeleteString(0);
	} 
	else
	{
		WriteScope(command);
		m_SeqList.DeleteString(0);
		
		CString sToken=_T("");
		AfxExtractSubString(sToken, command, 0,' ');
		sToken = sToken + _T(" +\r\n");

		WaitTill(sToken);
		//while ( WaitTill(sToken) == false ) Sleep(50);
	}
	
}

void COctopusScope::GetPosition( void )
{
	WriteScope(_T("2POS?\r\n"));

	CString position(_T(""));
	CString temp(_T(""));
	CString ret(_T(""));

	WaitTill(_T("2POS "));

	for( int i = 0; i < 40; i++ )
	{
		temp = ReadScope(1);
		ret  = temp.SpanIncluding(_T("0123456789"));
		
		if (ret.GetLength() == 0 ) 
		{	
			ReadScope(1); //we have hit the end of the number
			break;
		}
		position.Append(temp);
	}

	position_now = atof(position);

	B.position_z_objective = position_now / 100.0; //to convert to microns

	DisplayPosition();
} 

void COctopusScope::DisplayPosition( void )
{
	if( IsWindowVisible() ) 
	{
		CString str;
		str.Format(_T("%.2f"), position_now / 100.0 );
		m_Pos.SetWindowText( str );	
	}		
}

double COctopusScope::Z_GetPosMicrons( void )
{
	return (position_now / 100.00);
}

/**************************************************************************************
OBJECTIVE CHOICE
**************************************************************************************/

void COctopusScope::OnObj_1() { Objective( 1 ); };
void COctopusScope::OnObj_2() { Objective( 2 ); };
void COctopusScope::OnObj_3() { Objective( 3 ); };
void COctopusScope::OnObj_4() { Objective( 4 ); };
void COctopusScope::OnObj_5() { Objective( 5 ); };
void COctopusScope::OnObj_6() { Objective( 6 ); };

void COctopusScope::OnMU_1() { EpiFilterWheel( 1 ); };
void COctopusScope::OnMU_2() { EpiFilterWheel( 2 ); };
void COctopusScope::OnMU_3() { EpiFilterWheel( 3 ); };
void COctopusScope::OnMU_4() { EpiFilterWheel( 4 ); };
void COctopusScope::OnMU_5() { EpiFilterWheel( 5 ); };
void COctopusScope::OnMU_6() { EpiFilterWheel( 6 ); };

void COctopusScope::OnBF_1() { BrightFieldFilterWheel( 1 ); };
void COctopusScope::OnBF_2() { BrightFieldFilterWheel( 2 ); };
void COctopusScope::OnBF_3() { BrightFieldFilterWheel( 3 ); };
void COctopusScope::OnBF_4() { BrightFieldFilterWheel( 4 ); };
void COctopusScope::OnBF_5() { BrightFieldFilterWheel( 5 ); };
void COctopusScope::OnBF_6() { BrightFieldFilterWheel( 6 ); };

void COctopusScope::OnCO_1() { Condensor( 1 ); };
void COctopusScope::OnCO_2() { Condensor( 2 ); };
void COctopusScope::OnCO_3() { Condensor( 3 ); };
void COctopusScope::OnCO_4() { Condensor( 4 ); };
void COctopusScope::OnCO_5() { Condensor( 5 ); };
void COctopusScope::OnCO_6() { Condensor( 6 ); };

void COctopusScope::Objective( int obj )
{
	if( obj == g_objective_now )
	{
		//we are already there! No need to do anything
		//AfxMessageBox(_T("We are already there!"));
		m_Radio_OB = g_objective_now - 1;
		UpdateData( false );
	} 
	else
	{
		CString temp;
		temp.Format(_T("1OB %d\r\n"), obj);
		m_SeqList.AddString(temp);
	}
}

void COctopusScope::ObjectiveProc( int obj )
{
	
	int old_obj = g_objective_now; 
	
	//this is where we will go back to, once we are done. 
	//this value should be good
	int  old_position = u32(position_now); 

	long obj2offset =      0;
	long obj3offset =      0; //125000;
	long obj4offset =      0;
	long obj5offset =      0; //44600;
	long obj6offset =      0;

	//1, 10x is the standard reference frame
	if ( old_obj == 1 ) old_position = old_position - 0;
	if ( old_obj == 2 ) old_position = old_position - obj2offset;
	if ( old_obj == 3 ) old_position = old_position - obj3offset;
	if ( old_obj == 4 ) old_position = old_position - obj4offset;
	if ( old_obj == 5 ) old_position = old_position - obj5offset;
	if ( old_obj == 6 ) old_position = old_position - obj6offset;

	//now we are in standard frame
	//add the correct offsets back
		
	if ( obj == 1 ) old_position = old_position + 0;
	if ( obj == 2 ) old_position = old_position + obj2offset;
	if ( obj == 3 ) old_position = old_position + obj3offset;
	if ( obj == 4 ) old_position = old_position + obj4offset;
	if ( obj == 5 ) old_position = old_position + obj5offset;
	if ( obj == 6 ) old_position = old_position + obj6offset;
	
	CString temp;
	//move to a defined place, if we are not yet there anyway
	//this will fail if we already were there to begin with
	if( old_position != 10000 )
	{
		temp.Format(_T("2MOV d,%d\r\n"), 10000 );
		WriteScope( temp );
		WaitTill(_T("2MOV +\r\n"));
	}

	temp.Format(_T("1OB %d\r\n"), obj);
	WriteScope( temp );
	WaitTill(_T("1OB +\r\n"));

	//this will fail if we already were there to begin with
	if( old_position != 10000 )
	{
		temp.Format(_T("2MOV d,%d\r\n"), old_position );
		WriteScope( temp );
		WaitTill(_T("2MOV +\r\n"));
	}

	//something is wrong here? 
	g_objective_now   = obj;
	m_Radio_OB        = g_objective_now - 1;
	B.Scope_objective = g_objective_now;

	//UpdateData(false); CHECK CHECK BUG BUG BUG
}

int COctopusScope::GetObj( void )
{
	//this is _never_ called by the user - 
    //no need to worry about sequencing/order
	//_probably_ not in any case :-)
	WriteScope(_T("1OB?\r\n"));
	WaitTill(_T("1OB "));
	
	int i = atoi( ReadScope(1) );
	ReadScope(2); //clear it out
	return i;
}

int COctopusScope::GetMU( void )
{
	WriteScope(_T("1MU?\r\n"));
	WaitTill(_T("1MU "));
	
	int i = atoi( ReadScope(1) );
	ReadScope( 2 ); //clear it out
	return i;
}

int COctopusScope::GetBF( void )
{
	//Brightfield filter wheel
	WriteScope(_T("1CD?\r\n"));
	WaitTill(_T("1CD "));
	
	int i = atoi( ReadScope(1) );
	ReadScope( 2 ); //clear it out
	return i;
}

int COctopusScope::GetCond( void )
{
	//Condensor
	WriteScope(_T("1CD?\r\n"));
	WaitTill(_T("1CD "));
	
	int i = atoi( ReadScope( 1 ) );
	ReadScope(2); //clear it out
	return i;
}

int COctopusScope::GetPath( void )
{
	//Condensor
	WriteScope(_T("1PRISM?\r\n"));
	WaitTill(_T("1PRISM "));
	
	int i = atoi( ReadScope( 1 ) );
	ReadScope(2); //clear it out
	return i;
}

void COctopusScope::EpiFilterWheel( int cube )
{
	CString temp;
	temp.Format(_T("1MU %d\r\n"), cube);
	m_SeqList.AddString(temp);
	
	//in case we call this programmatically
	m_Radio_MU       = cube - 1; 
	g_cube_now       = cube;
	B.Scope_epiwheel = g_cube_now;
	B.filter_wheel   = B.Scope_epiwheel;
	
	UpdateData( true ); //possible BUG BUG BUG
}

void COctopusScope::EpiFilterWheelDirect( int cube )
{
	CString command;
	command.Format(_T("1MU %d\r\n"), cube);

	WriteScope(command);

	CString sToken=_T("");
	AfxExtractSubString(sToken, command, 0,' ');
	sToken = sToken + _T(" +\r\n");

	WaitTill(sToken);

	//while ( WaitTill(sToken) == false ) Sleep(50);

	//in case we call this programmatically
	m_Radio_MU       = cube - 1; 
	g_cube_now       = cube;
	B.Scope_epiwheel = g_cube_now;
	B.filter_wheel   = g_cube_now;

	UpdateData( true ); //should this be true? possible BUG BUG BUG
}

void COctopusScope::Condensor( int element )
{
	//Mirror unit - slightly confusing - same as bright field wheel - should really use different connecter on controller
	CString temp;
	temp.Format(_T("1CD %d\r\n"), element);
	m_SeqList.AddString(temp);
	
	//in case we call this programmatically
	m_Radio_CO = element - 1; 
	g_cond_now = element;
	UpdateData( false );
}

void COctopusScope::OnPathEye()
{
	m_SeqList.AddString(_T("1PRISM 1\r\n"));
	
	//in case we call this programmatically
	m_Radio_PA = 0; 
	g_path_now = 1;
	UpdateData( false );
}

void COctopusScope::OnPathCamera()
{
	m_SeqList.AddString(_T("1PRISM 2\r\n"));
	
	//in case we call this programmatically
	m_Radio_PA = 1; 
	g_path_now = 2;
	UpdateData( false );
}

void COctopusScope::BrightFieldFilterWheel( int filter )
{
	CString temp;
	temp.Format(_T("1CD %d\r\n"), filter);
	m_SeqList.AddString(temp);
	
	//in case we call this programmatically
	m_Radio_BF     = filter - 1; 
	g_NDfilter_now = filter;
	UpdateData( false );
}

void COctopusScope::ClearBuffer( void )
{
	if( pPortScope == NULL ) return;

	for( int i = 0; i < 40; i++ )
	{
		if (ReadScope( 1 ) == _T("\r\n")) break;
	}
}

bool COctopusScope::WaitTill( CString waitStr )
{
	u16 strlen  = 0;
	u16 timeout = 0;
	
	Sleep( 100 );

	strlen = waitStr.GetLength();
	
	while ( ReadScope( strlen ).Find( waitStr ) == -1 )
	{
		Sleep( 50 );

		timeout++;
		
		if (timeout > 100) //ten seconds
		{
			AfxMessageBox(waitStr + _T("\nScope timeout."));
			ClearBuffer();
			break;
		}
	}
			
	return true;
}

/**************************************************************************************
LAMP
**************************************************************************************/
void COctopusScope::OnLightOnOff( void )
{
	CString temp;

	if ( bfon )
	{
		temp.Format(_T("1LMP %d\r\n"), 0);
		m_SeqList.AddString(temp);
		bfon = false;
	} 
	else
	{
		temp.Format(_T("1LMP %d\r\n"), g_volt);
		m_SeqList.AddString(temp);
		bfon = true;
	}

}

void COctopusScope::BrightField( int volt )
{
	if ( volt >= 120 )
	{
		g_volt = 120;
		bfon = true;
	}
	else if ( volt <= 0 ) 
	{
		g_volt = 0;
		bfon = false;
	}
	else
	{
		g_volt = volt;
		bfon = true;
	}

	CString temp;
	temp.Format(_T("1LMP %d\r\n"), g_volt);
	m_SeqList.AddString(temp);

	DisplayBFIntensity( g_volt );
}

void COctopusScope::BrightFieldFastOn( void )
{

	CString command;
	command.Format(_T("1LMP %d\r\n"), g_volt);

	WriteScope( command );

	CString sToken=_T("");
	AfxExtractSubString(sToken, command, 0,' ');
	sToken = sToken + _T(" +\r\n");

	//while ( WaitTill(sToken) == false ) Sleep(50);

	WaitTill( sToken );

	bfon = true;

	DisplayBFIntensity( g_volt );
}

void COctopusScope::BrightFieldFastOff( void )
{

	CString command;
	command.Format(_T("1LMP %d\r\n"), 0);

	WriteScope( command );

	CString sToken=_T("");
	AfxExtractSubString(sToken, command, 0,' ');
	sToken = sToken + _T(" +\r\n");

	//while ( WaitTill(sToken) == false ) Sleep(50);

	WaitTill( sToken );

	bfon = false;

	DisplayBFIntensity( 0 );
}

void COctopusScope::OnNMCustomdrawExecute( NMHDR *pNMHDR, LRESULT *pResult )
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	
	int CurPos = m_Slider.GetPos();
	
	if ( CurPos >   120 ) CurPos = 120;
	if ( CurPos <=    0 ) CurPos =   0;
	
	BrightField( CurPos );

	*pResult = 0;	
}

void COctopusScope::DisplayBFIntensity( int val )
{
	if( IsWindowVisible() ) 
	{
		CString str;
		str.Format(_T("BF Lamp (0-12V): %.1f"), double(val)/10 );
		m_Slider_Setting.SetWindowText( str );
		m_Slider.SetPos( val );
		UpdateData( false );
	}
}

/**************************************************************************************
NITTY
**************************************************************************************/
bool COctopusScope::WriteScope( CString str )
{
	if( Scope_initialized )
	{
		strcpy(pPortScope->mOutBuf, (LPCTSTR)str);

		if( pPortScope->WriteCPort() ) 
		{
			while( pPortScope->mResWrite != true ) 
			{
				pPortScope->CheckWrite();
			}
		}
		else//an error occurred during the write
			return false;
        
		return true;//the write was successful
	}
	else//the scope was not first initialized
		return false;
}

CString COctopusScope::ReadScope( u16 BytesToRead )
{
	
	if( pPortScope == NULL ) 
		return _T("Error");
	
	pPortScope->mBytesToRead = BytesToRead;

	if( pPortScope->ReadCPort() ) //issue the comm port read command
	{
		while( pPortScope->mResRead != true ) 
		{
			pPortScope->CheckRead();
		}	
		return (CString)pPortScope->mInBuf;
	}
	else 
	{
		return _T("Error");
	}
}

unsigned char COctopusScope::ReadScopeOne( void )
{
	
	if( pPortScope == NULL ) 
		return _T('\0');

	if( pPortScope->ReadCPortOneByte() ) //issue the comm port read command
	{
		while( pPortScope->mResRead != true ) 
		{
			pPortScope->CheckRead();
		}	
		return pPortScope->mInOne;
	}
	else 
	{
		return _T('\0');
	}
}

BOOL COctopusScope::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	int id = LOWORD(wParam);     // Notification code
	if( id == 2 ) return FALSE;  // Trap ESC key
	if( id == 1 ) return FALSE;  // Trap RTN key
    return CDialog::OnCommand(wParam, lParam);
}
