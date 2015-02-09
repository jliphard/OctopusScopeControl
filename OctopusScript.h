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
#if !defined(AFX_H_OctopusScript)
#define AFX_H_OctopusScript

#include "stdafx.h"
#include "OctopusGlobals.h"

class COctopusScript : public CDialog
{

public:
	COctopusScript(CWnd* pParent = NULL);
	virtual ~COctopusScript();
	enum { IDD = IDC_SCRIPT };

protected:
	
	UINT        m_nTimer;
	CListBox	m_SeqList;
	int			m_SeqListIndex;
	u16		    cycles_to_do;
	u16         cycles_to_do_in_program;
	u16			command_index;
	CBitmap     m_bmp_running;
	CBitmap     m_bmp_stopped;
	CStatic     m_status_scr;
	CStatic     m_cycle_count;

	virtual void DoDataExchange(CDataExchange* pDX);

	afx_msg void OnButtonSeqLoad();
	afx_msg void OnButtonSeqRun();
	afx_msg void OnButtonSeqStop();

	afx_msg void OnTimer(UINT nIDEvent);

	DECLARE_MESSAGE_MAP()
};

#endif
