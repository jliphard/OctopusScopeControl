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
#if !defined(AFX_H_OctopusScope)
#define AFX_H_OctopusScope

#include "stdafx.h"
#include "afxwin.h"
#include <process.h>
#include <stdio.h>
#include "cport.h"

DWORD WINAPI ScopeCommandBuffer( LPVOID lpParameter );

class COctopusScope : public CDialog
{

public:
	
	COctopusScope(CWnd* pParent = NULL);
	virtual ~COctopusScope();
	enum { IDD = IDC_SCOPE };
	
	void Z_StepUp( void );
	void Z_StepDown( void );
	void Z_GoTo( double target_microns );
	void Z_GoToRel( double step_microns ); 
	double Z_GetPosMicrons( void );

	void BrightField( int volt );
	void BrightFieldFastOn( void );
	void BrightFieldFastOff( void );
	
	void ChangePath( void );
	void Close( void );
	void EpiFilterWheel( int cube );
	void EpiFilterWheelDirect( int cube );
	void BrightFieldFilterWheel( int filter );
	void Condensor( int element );
	bool IsBFOn( void ) { return bfon; };

	bool Scope_working;
	CListBox m_SeqList;
	void ExecuteNextCommand( void );

protected:

	bool Scope_initialized;

	void DisplayPosition( void );
	bool WaitTill( CString waitStr );
	//bool WaitTillPlus( void );
	void ClearBuffer( void );

	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual BOOL OnInitDialog();
	afx_msg void OnNMCustomdrawExecute( NMHDR* pNMHDR, LRESULT* pResult );

	DECLARE_MESSAGE_MAP()

	CPort* pPortScope;
	bool WriteScope( CString str );
	CString ReadScope( u16 BytesToRead );
	unsigned char ReadScopeOne( void );

	bool Init( void );
	bool bfon;

	CStatic m_Pos;

	int	m_Radio_S;
	int m_Radio_OB;
	int m_Radio_MU;
	int m_Radio_BF;
	int m_Radio_CO;
	int m_Radio_PA;

	double position_now;
	u32 position_to_return_to;
  	u16 stepsize_10nm;

	int g_objective_now;
	int g_cube_now;
	int g_NDfilter_now;
	int g_cond_now;
	int g_path_now;

	void DisplayBFIntensity( int val );

	int  GetObj( void );
	int  GetMU( void );
	int  GetBF( void );
	int  GetCond( void );
	int  GetPath( void );
	int  GetBrightfield( void );

	CStatic     m_Slider_Setting;
	CSliderCtrl m_Slider;
	CString     m_Slider_Setting_String;

public:

	afx_msg void OnObjectiveStepSize1();
	afx_msg void OnObjectiveStepSize2();
	afx_msg void OnObjectiveStepSize3();
	afx_msg void OnObjectiveStepSize4();

	afx_msg void OnObj_1();
	afx_msg void OnObj_2();
	afx_msg void OnObj_3();
	afx_msg void OnObj_4();
	afx_msg void OnObj_5();
	afx_msg void OnObj_6();

	afx_msg void OnMU_1();
	afx_msg void OnMU_2();
	afx_msg void OnMU_3();
	afx_msg void OnMU_4();
	afx_msg void OnMU_5();
	afx_msg void OnMU_6();

	afx_msg void OnCO_1();
	afx_msg void OnCO_2();
	afx_msg void OnCO_3();
	afx_msg void OnCO_4();
	afx_msg void OnCO_5();
	afx_msg void OnCO_6();

	afx_msg void OnBF_1();
	afx_msg void OnBF_2();
	afx_msg void OnBF_3();
	afx_msg void OnBF_4();
	afx_msg void OnBF_5();
	afx_msg void OnBF_6();

	afx_msg void OnPathEye();
	afx_msg void OnPathCamera();
	afx_msg void OnLightOnOff( void );

	void Objective( int obj );
	void ObjectiveProc( int obj );

	afx_msg void ObjToBottom();
	afx_msg void ObjReturn();

	HANDLE wait_thread_cmd;
	bool run_the_wait_thread;

	void GetPosition( void );

	int brightfield_initial;
	int g_volt;
};

#endif