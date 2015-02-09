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
///////////////////////////////////////////////////////////////////////////////
// CameraDisplay.h : header file
///////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_H_OctopusCameraDisplay)
#define AFX_H_OctopusCameraDisplay

#include "stdafx.h"
#include "OctopusGlobals.h"
#include <stdio.h>
#include "afxwin.h"

using namespace std;

class COctopusPictureDisplay : public CDialog
{

public:
	
	COctopusPictureDisplay( CWnd* pParent = NULL, u16 x = 50, u16 y = 50 );
	~COctopusPictureDisplay();
	
	enum { IDD = IDC_CAS_DISP };

	void Update_Bitmap( u16 *pic, u16 FramesTransferred );	
	void Create_Bitmap( void );
	void Close_The_File( void );

protected:

	void WritePic( void );
	bool Open_A_File( void );

	CBitmap Bitmap_main;

	uC*  pPic_main;
	u16* first_picture;
	u16* first_picture_flip;
	u16* frames_to_save;
	u16  MemoryFramesOld;
	u16  g_NumFT;

	FILE * pFileData;
	FILE * pFileHead;

	CString filename; 
	COctopusROI ROI;

	void Draw_Bitmap( void );
	void UpdateTitle( void );
	void UpdateCellInfo( void ); 
	
	u16	Width; 
	u16	Height;

	u32 Length;
	u32 FullFrame;

	u16 pic_x1;
	u16 pic_y1;
	u16 pic_x2;
	u16 pic_y2;

	int px;
	int py;
	int px2;
	int py2;

	u16    pic_size_u16;
	double pic_size_d;

	u16 pic_edge_x;
	u16 pic_edge_y;

	double	g_mp;
	double	g_max;
	double	g_min;
	double  g_mean;

	u32		pictures_written;

	double time_dx;
	double time_old;
	double frequency;
	double camera_freq;
	double display_freq;
	
	bool Setting_Cell;
	
	void LeftButtonDown( CPoint point );
	void MouseMove( CPoint point );

	void ValidateMarkersAndSetROI( void );

	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);

	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual BOOL OnInitDialog();

	double ScreenYToStageX(int py);
	double ScreenXToStageY(int px);

	int StageYToScreenX(double y);
	int StageXToScreenY(double x);
	
	afx_msg void AddCell( void );

	CStatic m_info;
	CStatic m_cell_info;

	double magfactor;

	virtual void DoDataExchange(CDataExchange* pDX);

	DECLARE_MESSAGE_MAP()
public:

};

#endif
