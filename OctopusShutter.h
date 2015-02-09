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

#if !defined(AFX_H_OctopusShutterAndWheel)
#define AFX_H_OctopusShutterAndWheel

#include "stdafx.h"
#include "OctopusGlobals.h"
#include "ftd2xx.h"

class COctopusShutterAndWheel : public CDialog
{

public:
	
	COctopusShutterAndWheel(CWnd* pParent = NULL);
	virtual ~COctopusShutterAndWheel();

	enum { IDD = IDC_LAMBDA };

protected:

	int			board_present;
	double      ND_value;
	CStatic     m_Slider_Setting;
	CSliderCtrl m_Slider;
	CString     m_Slider_Setting_String;
	int			m_ShutterOpen;
	int			m_Filter;
	FT_HANDLE	m_ftHandle;
	bool        USB_ready;

	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

	void Loadem();

	virtual BOOL OnInitDialog();
	
	void UpdateShutterVal( void );

	afx_msg void OnRadioFilter0();
	afx_msg void OnRadioFilter1();
	afx_msg void OnRadioFilter2();
	afx_msg void OnRadioFilter3();
	afx_msg void OnRadioFilter4();
	afx_msg void OnRadioFilter5();
	afx_msg void OnRadioFilter6();
	afx_msg void OnRadioFilter7();
	afx_msg void OnRadioFilter8();
	afx_msg void OnRadioFilter9();
	
	DECLARE_MESSAGE_MAP()

public:

	bool ShutterReady ( void );
	void ShutterOpen( void );
	void ShutterClose( void );
	void ShutterPartial( u8 val );
	void Filter( u8 filter ); 

	afx_msg void OnNMCustomdrawExecute( NMHDR* pNMHDR, LRESULT* pResult );
};

#endif
