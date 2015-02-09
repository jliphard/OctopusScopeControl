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
#include "mainfrm.h"
#include "Octopus.h"
#include "OctopusDoc.h"
#include "OctopusView.h"
#include "OctopusGlobals.h"
#include "OctopusShutter.h"
#include "OctopusCameraDlg.h"
#include "OctopusScript.h"
#include "OctopusSamplePiezo.h"
#include "OctopusMultifunction.h"
#include "OctopusStage686.h"
#include "OctopusScope.h"

extern COctopusGlobals B;

extern COctopusCamera*          glob_m_pCamera;
extern COctopusShutterAndWheel* glob_m_pShutterAndWheel;
extern COctopusMultifunction*   glob_m_pNI;
extern COctopusSamplePiezo*     glob_m_pSamplePiezo;
extern COctopusStage686*        glob_m_pStage686;
extern COctopusScope*           glob_m_pScope;

COctopusScript::COctopusScript(CWnd* pParent)
	: CDialog(COctopusScript::IDD, pParent)
{    

	m_SeqListIndex          = -1;
	command_index           =  0;
	cycles_to_do            =  0;
	cycles_to_do_in_program =  0;
	B.program_running       = false;

	VERIFY(m_bmp_running.LoadBitmap(IDB_RUNNING));
	VERIFY(m_bmp_stopped.LoadBitmap(IDB_STOPPED));

	if( Create(COctopusScript::IDD, pParent) ) 
		ShowWindow( SW_SHOW );
}

void COctopusScript::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX,	IDC_LIST_SEQUENCE,	 m_SeqList);
	DDX_LBIndex(pDX,	IDC_LIST_SEQUENCE,	 m_SeqListIndex);
	DDX_Control(pDX,    IDC_SCR_RUNNING_BMP, m_status_scr);
	DDX_Control(pDX,    IDC_SCR_CYCLE,       m_cycle_count);
}  

BEGIN_MESSAGE_MAP(COctopusScript, CDialog)
	ON_BN_CLICKED(IDC_BUTTON_SEQ_LOAD, OnButtonSeqLoad)
	ON_BN_CLICKED(IDC_BUTTON_SEQ_RUN,  OnButtonSeqRun)
	ON_BN_CLICKED(IDC_BUTTON_SEQ_STOP, OnButtonSeqStop)
	ON_WM_TIMER()
END_MESSAGE_MAP()

COctopusScript::~COctopusScript() {}

/***************************************************************************/
/***************************************************************************/

void COctopusScript::OnButtonSeqLoad() 
{

	CFile flConfigs;
	CString str, str0, str1;
	CString cycles;
	CString sub;
	CString subsub;
	
	m_SeqList.ResetContent();
	m_SeqListIndex = 0;

	UpdateData(FALSE);

	CFileDialog FileDlg(TRUE, NULL, NULL, 4|2, "seq|*.seq||", NULL, 0);
	
	if( FileDlg.DoModal() == IDOK )
	{
		CStdioFile f(FileDlg.GetFileName(),CFile::modeRead);
		
		if( m_SeqList.GetCount() > 0 ) // zero the current sequence list
		{
			m_SeqList.ResetContent();
			m_SeqListIndex = 0;
		}

		bool looking_for_the_end = true;
		
		while ( looking_for_the_end )
		{
			f.ReadString(str);
			
			if( str.Find("#") >=0 ) // comment
			{	
				//do nothing
				//do not add them to the list
			} 
			else if (str.Find("subroutine") >= 0) 
			{
				str.Trim(); // remove leading and trailing space
				int n = str.Find(_T(" "));
				if( n > 0 ) /* space is at ... */
				{
					str.GetLength();
					sub = str.Right(str.GetLength() - n);
					sub.Trim();
					int insertcount = 0;
					m_SeqListIndex = m_SeqList.GetCount();
					
					CStdioFile subf(sub,CFile::modeRead);

					while ( subf.ReadString(str) )
					{
						if( str.Find("#") >=0 ) // comment
						{	
							//do nothing
							//do not add them to the list
						} 
						else if ( str.Find("subroutine") >= 0 ) 
						{
							//add it to the list
							str.Trim(); // remove leading and trailing space
							int k = str.Find(_T(" "));
							if( k > 0 ) /* space is at ... */
							{
								str.GetLength();
								subsub = str.Right(str.GetLength() - k);
								subsub.Trim();
								int insertcount_ss = 0;
								m_SeqListIndex = m_SeqList.GetCount();
					
								CStdioFile subsubf(subsub,CFile::modeRead);
								
								while ( subsubf.ReadString(str) )
								{
									if( str.Find("#") >=0 ) // comment
									{	
										//do nothing do not add them to the list
									} 
									else if ( str.Find("subroutine") >= 0 ) 
									{
										//do nothing do not add them to the list
									}
									else if ( str.Find("end") >= 0 ) 
									{
										//do nothing do not add them to the list
									}
									else if ( str.Find("cycles") >= 0 ) 
									{
										//do nothing do not add them to the list
									}
									else
									{
										//insert
										m_SeqList.InsertString( m_SeqListIndex + insertcount_ss, str );
										insertcount_ss++;
									}
								} //while
							} //k = 0
						}//else if sub within a sub
						else if ( str.Find("end") >= 0 ) 
						{
							//do nothing do not add them to the list
						}
						else if ( str.Find("cycles") >= 0 ) 
						{
							//do nothing do not add them to the list
						}
						else
						{
							//insert
							m_SeqList.InsertString( m_SeqListIndex + insertcount, str );
							insertcount++;
						}
					}
				}
			}
			else if (str.Find("cycles") >= 0) 
			{
				// last line has been found - good
				// swscanf(str, _T("%*s %d"), &cycles_to_do_in_program);
				sscanf_s(str, "%*s %d", &cycles_to_do_in_program);
				m_SeqList.AddString(str);
				looking_for_the_end = false;
			}
			else if (str.Compare("\n") != 0)
			{
				m_SeqList.AddString(str);
			}
		}
		m_SeqListIndex = m_SeqList.GetCount();
		f.Close();
	}
	else
		return;

	UpdateData(FALSE);
}

void COctopusScript::OnButtonSeqRun() 
{
	command_index  = 0;
	cycles_to_do   = cycles_to_do_in_program;
	m_nTimer       = SetTimer( TIMER_SCRIPT, 200, NULL );

	m_status_scr.SetBitmap( m_bmp_running );
    
	B.program_running = true;

	CString temp;
	
	temp.Format("Cycles to do:%d\nCycles done:%d", \
		cycles_to_do, cycles_to_do_in_program - cycles_to_do);

	m_cycle_count.SetWindowText( temp );
}

void COctopusScript::OnButtonSeqStop() 
{ 
	cycles_to_do = 0;
	B.program_running = false;
	m_status_scr.SetBitmap( m_bmp_stopped );
	KillTimer( m_nTimer );
}

void COctopusScript::OnTimer( UINT nIDEvent ) 
{
	if( nIDEvent == TIMER_SCRIPT ) 
	{
		CString str;

		if( B.focus_in_progress    ) return;

		if ( cycles_to_do > 0 )
		{	
			m_SeqList.GetText(command_index, str);
			
			str.MakeLower();

			if(str.Find(_T("pause:")) >= 0)
			{				
				command_index++;	
				float etime = 4.0;
				//swscanf(str, _T("%*s %e %*s"), &etime);
				sscanf_s(str, _T("%*s %e %*s"), &etime);
				m_nTimer = SetTimer( TIMER_SCRIPT, (u32)etime * 1000, NULL );				
			}
			else if( str.Find(_T("filter:"))>=0 ) // we have a filter command
			{
				command_index++;
				if ( glob_m_pShutterAndWheel == NULL ) return;
				u8 filter = 1;
				sscanf_s(str, _T("%*s %d"), &filter);
				glob_m_pShutterAndWheel->Filter( filter );
				m_nTimer = SetTimer( TIMER_SCRIPT, 200, NULL );
			}
			else if( str.Find(_T("mirrorunit:"))>=0 ) // we have a mirrorunit command
			{
				command_index++;
				if ( glob_m_pScope == NULL ) return;
				u8 mu = 1;
				sscanf_s(str,"%*s %d", &mu);
				glob_m_pScope->EpiFilterWheel( mu );
				m_nTimer = SetTimer( TIMER_SCRIPT, 200, NULL );
			}
			else if( str.Find(_T("brightnd:"))>=0 ) // we have a bright field filter command
			{
				command_index++;
				if ( glob_m_pScope == NULL ) return;
				u8 bf = 1;
				sscanf_s(str,"%*s %d", &bf);
				glob_m_pScope->BrightFieldFilterWheel( bf );
				m_nTimer = SetTimer( TIMER_SCRIPT, 200, NULL );
			}
			else if( str.Find(_T("autofocus:")) >=0 ) // we have an autofocus command
			{
				command_index++;

				float stepsize_microns = 1.0;

				sscanf_s(str, _T("%*s %e %*s"), &stepsize_microns);
				
				if( stepsize_microns <  0.0) stepsize_microns =  0.1;
				if( stepsize_microns > 20.0) stepsize_microns = 20.0;
				
				//if (glob_m_pFocus != NULL)
				//	glob_m_pFocus->AutoFocus( stepsize_microns );

				m_nTimer = SetTimer(TIMER_SCRIPT, 1000, NULL); 
			}
			else if( str.Find(_T("moveobjrel:")) >= 0 )
			{
				command_index++;

				float move_microns = 0.0;

				sscanf_s(str, _T("%*s %e %*s"), &move_microns);
			
				if( move_microns < -20.0 ) move_microns = -20.0;
				if( move_microns > +20.0 ) move_microns = +20.0;
				
				if ( glob_m_pSamplePiezo != NULL )
					glob_m_pSamplePiezo->MoveRelZ( move_microns );

				m_nTimer = SetTimer(TIMER_SCRIPT, 200, NULL); 
			}
			else if( str.Find(_T("movescopeobjrel:")) >= 0 )
			{
				command_index++;

				float move_microns = 0.0;

				sscanf_s(str, "%*s %e %*s", &move_microns);
			
				if( move_microns < -500.0 ) move_microns = -500.0;
				if( move_microns > +500.0 ) move_microns = +500.0;
				
				if ( glob_m_pScope != NULL ) 
					glob_m_pScope->Z_GoToRel( move_microns );

				m_nTimer = SetTimer(TIMER_SCRIPT, 200, NULL); 
			}
			else if( str.Find(_T("save on")) >= 0 )
			{
				command_index++;
				if (glob_m_pCamera == NULL) return;
				glob_m_pCamera->StartSaving();
				m_nTimer = SetTimer(TIMER_SCRIPT, 200, NULL); 
			}
			else if( str.Find(_T("save off")) >=0 )
			{
				command_index++;
				if (glob_m_pCamera == NULL) return;
				glob_m_pCamera->StopSaving();
				m_nTimer = SetTimer(TIMER_SCRIPT, 200, NULL); 
			}
			else if( str.Find(_T("shutter:")) >=0 ) // we have a shutter command
			{	
				command_index++;

				if (glob_m_pShutterAndWheel == NULL) return;

				u8 intensity = 1;
				
				sscanf_s(str, _T("%*s %d"), &intensity);

				if( intensity == 0 ) 
					glob_m_pShutterAndWheel->ShutterClose();
				else if ( intensity == 100 )
					glob_m_pShutterAndWheel->ShutterOpen();
				else
					glob_m_pShutterAndWheel->ShutterPartial( intensity ); 
					
				m_nTimer = SetTimer(TIMER_SCRIPT, 200, NULL); 
			}
			else if( str.Find(_T("movie on")) >= 0 ) // we have a camera command
			{	
				command_index++;
				if (glob_m_pCamera == NULL) return;
				glob_m_pCamera->StartMovie();
				m_nTimer = SetTimer(TIMER_SCRIPT, 200, NULL); 
			}
			else if( str.Find("movie off") >= 0 ) // we have a camera command
			{	
				command_index++;
				if (glob_m_pCamera == NULL) return;
				glob_m_pCamera->StopCameraThread();
				m_nTimer = SetTimer(TIMER_SCRIPT, 200, NULL); 
			}
			else if( str.Find("take picture") >= 0 ) // we have a camera command
			{	
				command_index++;
				if (glob_m_pCamera == NULL) return;
				u32 etime = glob_m_pCamera->GetExposureTime_Single_ms() + 100;
				glob_m_pCamera->TakePicture();
				m_nTimer = SetTimer(TIMER_SCRIPT, etime, NULL);
			}
			else if( str.Find("led off") >= 0 )
			{	
				command_index++;
				if (glob_m_pNI == NULL) return;
				//glob_m_pNI->LED_Off();
				m_nTimer = SetTimer(TIMER_SCRIPT, 200, NULL); 
			}
			else if( str.Find("led on") >= 0 )
			{	
				command_index++;
				if (glob_m_pNI == NULL) return;
				//glob_m_pNI->LED_On();
				m_nTimer = SetTimer(TIMER_SCRIPT, 200, NULL); 
			}
			else if( str.Find(_T("led:")) >=0 ) // we have a shutter command
			{	
				command_index++;
				if (glob_m_pNI == NULL) return;
				u8 intensity = 1;
				sscanf_s(str, _T("%*s %d"), &intensity);

				//if( intensity == 0 ) 
					//glob_m_pNI->LED_Off();
				//else
					//glob_m_pNI->LED_On( intensity);

				m_nTimer = SetTimer(TIMER_SCRIPT, 200, NULL); 
			}
			else if( str.Find(_T("movestage")) >= 0 )
			{
				command_index++;

				float move_mm = 0.0;

				sscanf_s(str, _T("%*s %e %*s"), &move_mm);
				
				if ( glob_m_pStage686 == NULL ) 
					return;

				if( move_mm < -6.0 ) move_mm = -6.0;
				if( move_mm > +6.0 ) move_mm = +6.0;
				
				if (str.Find('x') >= 0 ) 
					glob_m_pStage686->MoveRelX( move_mm );
				else if ( str.Find('y') >= 0 )
					glob_m_pStage686->MoveRelY( move_mm );

				m_nTimer = SetTimer(TIMER_SCRIPT, 200, NULL); 
			}
			else if( str.Find(_T("movetoxyz")) >= 0 )
			{
				command_index++;

				float x_mm      = 0.0;
				float y_mm      = 0.0;
				float z_microns = 0.0;

				sscanf_s(str, _T("%*s %e %e %e"), &x_mm, &y_mm, &z_microns);
				
				if ( glob_m_pStage686 == NULL ) return;
				if ( glob_m_pScope    == NULL ) return;

				if( x_mm <  0.0 ) x_mm =  0.0;
				if( x_mm > 24.0 ) x_mm = 24.0;
				glob_m_pStage686->MoveToX( x_mm );

				if( y_mm <  0.0 ) y_mm =  0.0;
				if( y_mm > 24.0 ) y_mm = 24.0;
				glob_m_pStage686->MoveToY( y_mm );

				if( z_microns <  100.0 ) z_microns =  100.0;
				if( z_microns > 9000.0 ) z_microns = 9000.0;
				glob_m_pScope->Z_GoTo( z_microns );
					
				m_nTimer = SetTimer(TIMER_SCRIPT, 200, NULL); 
			}
			/*
			else if( str.Find("laser561 off")>=0 )
			{	
				command_index++;
				if (glob_m_pLasers == NULL) return;
				glob_m_pLasers->Laser_561_Off();
				m_nTimer = SetTimer(TIMER_SCRIPT, 200, NULL); 
			}
			else if( str.Find("laser561 on")>=0 )
			{	
				command_index++;
				if (glob_m_pLasers == NULL) return;
				glob_m_pLasers->Laser_561_On();
				m_nTimer = SetTimer(TIMER_SCRIPT, 200, NULL); 
			}
			*/
			else if(str.Find("cycles")>=0) // we've hit the last line in the list of commands - repeat
			{	
				command_index = 0;
				cycles_to_do--;
				CString temp;
				temp.Format("Cycles to do:%d\nCycles done:%d", cycles_to_do, cycles_to_do_in_program - cycles_to_do);
				m_cycle_count.SetWindowText( temp );
				m_nTimer = SetTimer(TIMER_SCRIPT, 200, NULL);
			}
			else 
			{
				command_index++;
				CString err;
				err.Format("Invalid command, please check the script for errors!\n");
				err.Append( str );
				AfxMessageBox(err);
			}

		}
		else 
		{
			//we are done
			B.program_running = false;
			m_status_scr.SetBitmap( m_bmp_stopped );
			KillTimer(m_nTimer);
		}
	}
	CDialog::OnTimer(nIDEvent);
}