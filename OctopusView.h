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
/////////////////////////////////////////////////////////////////////////////
// OctopusView.h
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_H_OctopusView)
#define AFX_H_OctopusView

#include "stdafx.h"
#include "OctopusDoc.h"

class COctopusView : public CFormView {

protected:	
	
	COctopusView();
	DECLARE_DYNCREATE(COctopusView)

public:

	enum { IDD = IDC_BACKGROUND };
	virtual ~COctopusView();

protected:

	afx_msg void OnOpenAndor();
	afx_msg void OnOpenWheel();
	afx_msg void OnOpenScript();
	afx_msg void OnOpenStage545();
	afx_msg void OnOpenStage686();
	afx_msg void OnOpenPiezo();
	afx_msg void OnOpenLasers();
	afx_msg void OnOpenGrating();
	afx_msg void OnOpenPolarizer();
	afx_msg void OnOpenScope();
	afx_msg void OnOpenMotor();

	CTime systemtime;

	virtual void OnInitialUpdate();
	virtual void DoDataExchange(CDataExchange* pDX);

	void OnOpenMultifunction();
	u16 scope;
	CString debug;

	DECLARE_MESSAGE_MAP()

};

#endif
