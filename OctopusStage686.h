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

#if !defined(AFX_H_OctopusStage686)
#define AFX_H_OctopusStage686

#include "stdafx.h"
#include "OctopusGlobals.h"
#include "afxwin.h"
#include <process.h>
#include <stdio.h>
#include "Pi_gcs2_dll.h"

class COctopusStage686 : public CDialog
{

public:
	
	COctopusStage686(CWnd* pParent = NULL);
	virtual ~COctopusStage686();
	enum { IDD = IDC_STAGE_686 };

	void MoveRelX( double dist );
	void MoveRelY( double dist );

	void MoveToX( double target_mm );
	void MoveToY( double target_mm );

	void MoveToXY( double target_x_mm, double target_y_mm );

	void MoveLeft( void );
	void MoveRight( void );
	void MoveFwd( void );
	void MoveBack( void );

	void OnJoyStickOnOff( void );
	void SetVelocity( double velocity );

	bool StageMoving( void );

	double old_x;
	double old_y;
	double old_z;
	
	double velocity_nor;

	bool   old_joy_state;
	void   TurnJoyStickOn( void );

	void Laser_On( void );
	void Laser_Off( void );

protected:

	bool JoyStickOn;
	void TurnJoyStickOff( void );
	bool IsJoyStickOn( void );
	void JoystickModeNo( void );
	void JoystickModeYes( void );

	//FILE * pFileHead;
	//CString PathFileName;

	//void WritePos( void ); 
	//bool Open_A_File( void ); 
    //void Close_The_File( void );
	
	virtual BOOL OnInitDialog();

	//CButton m_btn_GoToPos;
	//CButton m_btn_SavePos;

	//CFont m_Font;

	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	afx_msg void OnNMCustomdrawExecute( NMHDR* pNMHDR, LRESULT* pResult );

	DECLARE_MESSAGE_MAP()

	CStatic m_Pos;
	CStatic m_Pos_Save;
	CStatic m_Step;

	double middle;
	double range;
	double save_xy[2];
	double target_xy[2];
	double stepsize;

	int	m_Radio_S;

	void ShowPosition( void );

	bool initialized;

	char    axis_xy[6];
	int		ID_XY;
	bool	XY_connected;
	double	dPos[2];
	BOOL	bIsMoving[2];
	BOOL	bJONState[2];

	CString debug; 

public:

	void GetPosition( void );

	HANDLE wait_thread;
	bool wait_thread_running;
	int thread_count;

	afx_msg void OnStepSize1();
	afx_msg void OnStepSize2();
	afx_msg void OnStepSize3();
	afx_msg void OnStepSize4();
    afx_msg void OnStepSize5();

	afx_msg void OnSave();
	afx_msg void OnSaveGoTo();

	afx_msg void OnKillfocusGeneral();
	afx_msg void OnKillfocusVelocityNor();

	afx_msg void OnTimer(UINT nIDEvent);

	afx_msg void ImageOneCell(       void );
	afx_msg void ImageThisCell( unsigned int CellID );
	afx_msg void ImageManyCells(     void );
	afx_msg void ImageStop(          void );

	void InitializeStage( void );
	void Close( void );
	afx_msg void StageCenter();

public:
	CListBox Cell_List;

	void CellAdd( void );

	void CellRemove( void );
	
	bool emergencySTOP;

	CButton Cbox_Laser1;
	CButton Cbox_Laser2;
	CButton Cbox_Laser3;
	CButton Cbox_Laser4;
	CButton Cbox_Brightfield;

	CComboBox Las1_FW_Choice;
	CComboBox Las2_FW_Choice;
	CComboBox Las3_FW_Choice;
	CComboBox Las4_FW_Choice;
	CComboBox Las5_FW_Choice;

	unsigned int Las1_ExpTime;
	unsigned int Las2_ExpTime;
	unsigned int Las3_ExpTime;
	unsigned int Las4_ExpTime;
	unsigned int Las5_ExpTime;
	
	unsigned int Las1_Gain;
	unsigned int Las2_Gain;
	unsigned int Las3_Gain;
	unsigned int Las4_Gain;
	unsigned int Las5_Gain;

	unsigned int AutoStepNum;
	unsigned int AutoStepSize;
};

#endif