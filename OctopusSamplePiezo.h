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
#if !defined(AFX_H_COctopusSamplePiezo)
#define AFX_H_OctopusSamplePiezo

#include "stdafx.h"
#include "OctopusGlobals.h"
#include "Pi_gcs2_dll.h"

class COctopusSamplePiezo : public CDialog
{

public:
	
	COctopusSamplePiezo(CWnd* pParent = NULL);
	virtual ~COctopusSamplePiezo();
	enum { IDD = IDC_PIEZO };

	void MoveUp( void );
	void MoveDown( void );
	void MoveRelZ( double dist );
	void MoveToZ( double z );

protected:

	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	afx_msg void OnNMCustomdrawExecute( NMHDR* pNMHDR, LRESULT* pResult );

	DECLARE_MESSAGE_MAP()

	CStatic m_Pos;
	
    bool first_tick;
	double save_z_microns; 
	double target_z_microns;
	double stepsize_z_microns;
	double middle_z_microns;
	double max_z_microns;

	int	m_Radio_S;

	CBitmap m_bmp_yes;
	CBitmap m_bmp_no;
	CStatic m_status;

	CString debug;
	CString result;

	void   ShowPosition( void );
	void   UpdatePosition( void );
	double GetPosition( void );

	char axis[17];
	int ID;
	bool connected;
	bool initialized;

	void WriteLogString( CString logentry );

public:

	afx_msg void OnStepSize1();
	afx_msg void OnStepSize2();
	afx_msg void OnStepSize3();
	afx_msg void OnStepSize4();
	afx_msg void OnStepSize5();

	afx_msg void OnSave();
	afx_msg void OnSaveGoTo();

	virtual BOOL OnInitDialog();

	afx_msg void OnKillfocusGeneral();
	afx_msg void OnTimer(UINT nIDEvent);

	int Initialize( void );
	void Close( void );
	afx_msg void Center();

};

#endif