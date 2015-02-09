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

#if !defined(AFX_H_OctopusLasers)
#define AFX_H_OctopusLasers

#include "stdafx.h"
#include "OctopusGlobals.h"

class COctopusLasers : public CDialog
{

public:
	
	COctopusLasers(CWnd* pParent = NULL);
	virtual ~COctopusLasers();
	enum { IDD = IDC_LASERS };

	void Laser_405_On( void );
	void Laser_405_Off( void );
	void Laser_488_On( void );
	void Laser_488_Off( void );
	void Laser_561_On( void );
	void Laser_561_Off( void );
	void Laser_639_On( void );
	void Laser_639_Off( void );

protected:

	double volts_CH1;
	double volts_CH2;

	CSliderCtrl m_Slider_CH1;
	CSliderCtrl m_Slider_CH2;

	CBitmap m_bmp_yes;
	CBitmap m_bmp_no;
	
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()

	CStatic m_status_405;
	CStatic m_status_488;
	CStatic m_status_561;
	CStatic m_status_639;

	CStatic m_status_405_Pulse;
	CStatic m_status_488_Pulse;
	CStatic m_status_561_Pulse;
	CStatic m_status_639_Pulse;

	bool pulsing;

	void OnIntensitySliderCH1(NMHDR *pNMHDR, LRESULT *pResult);
	void OnIntensitySliderCH2(NMHDR *pNMHDR, LRESULT *pResult);

public:

	unsigned int pulsetime_405;
	unsigned int pulsetime_488;
	unsigned int pulsetime_561;
	unsigned int pulsetime_639;

	unsigned int pulsedelay_405;
	unsigned int pulsedelay_488;
	unsigned int pulsedelay_561;
	unsigned int pulsedelay_639;

	afx_msg void OnClicked405();
	afx_msg void OnClicked488();
	afx_msg void OnClicked561();
    afx_msg void OnClicked639();

	unsigned int lasertopulse;

	afx_msg void OnClicked405Pulse();
	afx_msg void OnClicked488Pulse();
	afx_msg void OnClicked561Pulse();
	afx_msg void OnClicked639Pulse();

	afx_msg void OnKillfocusGeneral();

	void Laser_405_On( unsigned int ontime );
	void Laser_488_On( unsigned int ontime );
	void Laser_561_On( unsigned int ontime );
	void Laser_639_On( unsigned int ontime );

	afx_msg void OnTimer(UINT nIDEvent);

	UINT m_nTimer;

};

#endif