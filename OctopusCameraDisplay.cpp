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
#include "OctopusCameraDisplay.h"
#include "OctopusGlobals.h"
#include "OctopusCameraDlg.h"
#include "OctopusClock.h"
#include "OctopusStage686.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <float.h>

#define COL_BLUE   0x00FF0000
#define COL_RED    0x000000FF
#define COL_GREEN  0x0000FF00

CPen penB(PS_SOLID,1,COL_BLUE);
CPen penR(PS_SOLID,1,COL_RED);
CPen penG(PS_SOLID,1,COL_GREEN);

extern COctopusGlobals	  B;
extern COctopusCamera*    glob_m_pCamera;
extern COctopusGoodClock* glob_m_pGoodClock;
extern COctopusStage686*  glob_m_pStage686;

COctopusPictureDisplay::COctopusPictureDisplay(CWnd* pParent, u16 x, u16 y)
	: CDialog(COctopusPictureDisplay::IDD, pParent) 
{
	Width			 = x; 
	Height			 = y;
	Length           = x * y;
	FullFrame        = B.CCD_x_phys_e * B.CCD_y_phys_e;
	g_mp		     = 0.05;
	g_max		     = 0.0;
	g_min		     = 65535.0;
	g_mean           = 0.0;
	time_dx			 = 0.0;
	time_old		 = 0.0;
	camera_freq      = 1.0;
	display_freq     = 1.0;
	pictures_written = 0;
	pPic_main        = NULL;
	MemoryFramesOld  = 1;
	g_NumFT          = 1;

	B.CellParams.x	 = 0.0;
	B.CellParams.y	 = 0.0;
	B.CellParams.z	 = 0.0;

	//magfactor        = 1.500; // 5x
	//magfactor        = 0.750; //10x
	magfactor          = 0.075; //100x

	Setting_Cell      = false;

	//allocate some memory
	first_picture       = new u16[ Length ];
	first_picture_flip	= new u16[ Length ];
	
	//may need to reallocate for very fast runs
	frames_to_save   = new u16[ Length * MemoryFramesOld ];

	pFileData        = NULL;
	pFileHead		 = NULL;

	pic_size_u16     = 699;
	pic_size_d       = double(pic_size_u16);
	pic_edge_x       = 5;
	pic_edge_y       = 96;

	B.cellstring     = _T("CellND");
	
	//focus
	B.focus_score    = 0.0;

	pic_x1 = 0;
	pic_y1 = 0;
	pic_x2 = 0;
	pic_y2 = 0;
}

BOOL COctopusPictureDisplay::OnInitDialog() 
{
	CDialog::OnInitDialog();
	GetDlgItem( IDC_CAS_DISP_CELL_ADD )->EnableWindow( true );
	return TRUE;
}

BEGIN_MESSAGE_MAP(COctopusPictureDisplay, CDialog)
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_BN_CLICKED(IDC_CAS_DISP_CELL_ADD, AddCell)
END_MESSAGE_MAP()



void COctopusPictureDisplay::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CAS_DISP_INFO, m_info);
	DDX_Control(pDX, IDC_CAS_DISP_CELL_INFO, m_cell_info);
}

void COctopusPictureDisplay::AddCell( void ) 
{	
	Setting_Cell = true;
	GetDlgItem( IDC_CAS_DISP_CELL_ADD )->EnableWindow( false );
}

COctopusPictureDisplay::~COctopusPictureDisplay() 
{
	Bitmap_main.DeleteObject();

	delete [] first_picture;		first_picture		= NULL;
	delete [] first_picture_flip;	first_picture_flip  = NULL;
	delete [] frames_to_save;		frames_to_save		= NULL;
	delete [] pPic_main;			pPic_main			= NULL;

	if ( pFileData != NULL ) 
	{
		fclose( pFileData );
		pFileData = NULL;
	}
	if ( pFileHead != NULL ) 
	{
		fclose( pFileHead );
		pFileHead = NULL;
	}
}

void COctopusPictureDisplay::Create_Bitmap() 
{
	u16 window_size_x = pic_size_u16 + pic_edge_x + pic_edge_x + 8;
	u16 window_size_y = pic_size_u16 + pic_edge_y + 35;
	
	MoveWindow( 900, 8, window_size_x,  window_size_y );
	
	//bitmap placement
	pic_x1 = pic_edge_x;
	pic_y1 = pic_edge_y;
	pic_x2 = pic_edge_x + pic_size_u16;
	pic_y2 = pic_edge_y + pic_size_u16;

	CClientDC aDC( this );
	CDC* pDC = &aDC;
	
	//main bitmap
	BITMAP bm;
	Bitmap_main.CreateCompatibleBitmap( pDC, 512, 512 );	
	Bitmap_main.GetObject( sizeof(BITMAP), &bm );
	pPic_main = new uC [ bm.bmHeight * bm.bmWidthBytes ];

}

void COctopusPictureDisplay::Draw_Bitmap( void ) 
{
	CClientDC aDC( this );
	CDC* pDC = &aDC;
	CDC hMemDC;
	hMemDC.CreateCompatibleDC( pDC );

	BITMAP bm;
	Bitmap_main.GetBitmap( &bm );

	hMemDC.SelectObject( &Bitmap_main );
	hMemDC.SetMapMode( MM_ANISOTROPIC );

	//-1,1
	//both flipped
	//hMemDC.SetViewportOrg(bm.bmHeight, 0);
	//hMemDC.SetViewportExt(-1, 1);

	//1,1
	//y is good, x is still flipped
	//hMemDC.SetViewportOrg(0, 0);
	//hMemDC.SetViewportExt(1, 1);

	//1,-1
	//y is good, x is good
	hMemDC.SetViewportOrg(0, bm.bmHeight);
	hMemDC.SetViewportExt(1, -1);

    pDC->StretchBlt( pic_x1, pic_y1, pic_size_u16, pic_size_u16, \
		&hMemDC, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY );

}

void COctopusPictureDisplay::Update_Bitmap( u16 *pict, u16 FramesTransferred ) 
{

	u16 temp   = 0;
	u32 c	   = 0;
	u32 b      = 0;
	uC  val    = 0;
	u32 x      = 0;
	u32 y      = 0;
	double rj  = 0.0;
	g_mean     = 0.0;
	g_min      = 65535.0;
	g_max      = 0.0;
    g_NumFT    = FramesTransferred;
	
	double diff		= 0;
	double focus	= 0;
	double totalint = 0;

	if ( g_NumFT == MemoryFramesOld )
	{
		//all is well - stick with old memory
	} 
	else
	{
		//yipes need to change memory size
		delete [] frames_to_save;     
		frames_to_save  = NULL;
		frames_to_save  = new u16[ Length * FramesTransferred ];
		MemoryFramesOld = g_NumFT;
	}

	//there may be more frames in this vector, 
	//but let's only look at the first one. 
	for ( c = 0; c < Length; c++ ) 
	{
		temp = *( pict + c );
		*( first_picture + c ) = temp;
		if ( temp < g_min ) 
			g_min = double(temp); 
		else if ( temp > g_max ) 
			g_max = double(temp);	
		totalint = totalint + double(temp);
	}

	//some basic statistics
	g_mean = totalint / Length;

	//and a basic focus score
	for ( c = 0; c < Length; c++ ) 
	{
		temp = *( pict + c );
		diff   = double(temp) - g_mean;
		focus += (diff * diff);
	}

	//this is the normalized focus score
	B.focus_score = focus / totalint;

	// these are the data to be saved - there may be multiple 
	// frames in one vector, hence c < Length * g_NumFT
	for ( c = 0; c < Length * g_NumFT; c++ ) 
	{
		*( frames_to_save + c ) = *( pict + c );
	}

	//if CONV amplifier, need to flip image...	
	if ( B.Ampl_Conv ) 
	{
		u32 xf = 0;

		for ( y = 0; y < Height; y++ ) 
		{
			for ( x = 0; x < Width; x++ ) 
			{
				xf = Width - 1 - x;
				*( first_picture_flip + ( y * Width ) + xf ) = \
					*( first_picture + ( y * Width ) + x  );
			}
		}
	} 

	u16 *good_pic = first_picture;

	if( B.Ampl_Conv )
		good_pic = first_picture_flip;

	//u16 *good_pic = first_picture;
    
	if ( B.automatic_gain ) 
	{
		g_mp = 255.0 / ( g_max - g_min ); 
	} 
	else 
	{
		g_mp = B.manual_gain;
	}

	B.time_now = (double)glob_m_pGoodClock->End();
	time_dx    = B.time_now - time_old;
	time_old   = B.time_now;

	if( time_dx > 0 ) 
	{
		camera_freq  = double(g_NumFT) / time_dx;
		display_freq = 1.0             / time_dx;
	}

	for ( c=0, b=0; c < Length; c++ ) 
	{	
		rj = double( *( good_pic + c ) - g_min ) * g_mp;

		val = (uC)(rj);

		*(pPic_main + b++) = val;
		*(pPic_main + b++) = val;
		*(pPic_main + b++) = val;
		*(pPic_main + b++) = val;
	}

	BITMAP bm;
	Bitmap_main.GetBitmap( &bm );	
	Bitmap_main.SetBitmapBits( bm.bmWidthBytes * bm.bmHeight, pPic_main );
	Draw_Bitmap();
	UpdateTitle();

	if( B.savetofile ) WritePic();

	
	CClientDC aDC( this );
	CDC* pDC = &aDC;

	//draw a cross in the middle
	pDC->SelectObject(&penB);
	pDC->MoveTo( pic_edge_x + 350 - 5, pic_edge_y + 350 );
	pDC->LineTo( pic_edge_x + 350 + 5, pic_edge_y + 350 );
	pDC->MoveTo( pic_edge_x + 350    , pic_edge_y + 350 - 5 );
	pDC->LineTo( pic_edge_x + 350    , pic_edge_y + 350 + 5 );

}

bool COctopusPictureDisplay::Open_A_File( void ) 
{
	CString temp;

	B.files_written++;

	temp.Format(_T("%ld"), B.files_written );
	filename = B.pathname + temp + B.cellstring;
	
	pFileData = fopen( filename + _T(".dat"), _T("wb"));//_wfopen( filename , _T("wb"));
	pFileHead = fopen( filename + _T(".dth"), _T("wt"));//_wfopen( filename , _T("wt"));

	if ( pFileData != NULL && pFileHead != NULL ) 
		return true;
	else 
		return false;
}

void COctopusPictureDisplay::Close_The_File( void )
{
	if( pFileHead == NULL || pFileData == NULL ) return;

	pictures_written = 0;

	fclose( pFileData );
	pFileData = NULL;

	fclose( pFileHead );
	pFileHead = NULL;
}

void COctopusPictureDisplay::WritePic( void ) 
{

	if( pictures_written >= B.pics_per_file ) 
	{
		Close_The_File();
		pictures_written = 0;
	}

	// if the file is closed - due to any number of reasons 
	// => open a new one
	if( pFileHead == NULL || pFileData == NULL ) 
	{ 
		if ( !Open_A_File() ) return; 
	}

	u16 h = Height;
	u16 w = Width;
	
	CString str;

	u8 Las1on = 0;
	if(B.Laser_405_is_On) Las1on = 1;

	u8 Las2on = 0;
	if(B.Laser_488_is_On) Las2on = 1;

	u8 Las3on = 0;
	if(B.Laser_561_is_On) Las3on = 1;

	u8 Las4on = 0;
	if(B.Laser_639_is_On) Las4on = 1;

	for ( u16 n = 0; n < g_NumFT; n++)
	{
		str.Format(_T("N:%ld H:%d W:%d T:%.3f Filter:%d ND:%d X:%.5f Y:%.5f Z:%.5f L1:%d L2:%d L3:%d L4:%d Cell:%d\n"),
		        pictures_written, h, w, B.time_now, 
				B.filter_wheel, B.nd_setting,
				B.position_x_686, B.position_y_686, B.position_z_sample,
				Las1on, Las2on, Las3on, Las4on, B.AUTO_CellLabel);
			
		pictures_written++;

		fprintf( pFileHead, str );
	}
	fflush( pFileHead );

	fwrite( frames_to_save, sizeof(u16), w * h * g_NumFT, pFileData );
	fflush( pFileData );

	B.expt_time = B.time_now - B.savetime;
	B.expt_frame++;
}

void COctopusPictureDisplay::UpdateTitle( void ) 
{
	CString temp;
	
	temp.Format(_T("Min:%.1f Max:%.1f Cutoff:%.1f Mean:%.1f Focus:%.2f Cam_Freq(Hz):%.1f PicsTransfer:%d\n"), \
		         g_min, g_max, g_mp, g_mean, B.focus_score, camera_freq, g_NumFT);
	temp.AppendFormat(_T("Frames in current file:%d  Total saved frames:%d Time(s):%.1f SaveDuration(s):%.1f \n"), \
		         pictures_written + 1, B.expt_frame + 1, B.time_now, B.expt_time ); 
	temp.AppendFormat(_T("X:%.1f Y:%.1f Z:%.1f"), \
		         B.position_x_686, B.position_y_686, B.position_z_sample);

	m_info.SetWindowText( temp );
}

void COctopusPictureDisplay::UpdateCellInfo( void ) 
{
	CString temp;
	temp.Format(_T("Xmid:%.3f Ymid:%.3f Zmid:%.3f"), B.CellParams.x, B.CellParams.y, B.CellParams.z);
	m_cell_info.SetWindowText( temp );
}


/**************************************************************************
********************* MOUSE ***********************************************
**************************************************************************/

void COctopusPictureDisplay::MouseMove( CPoint point ) 
{

	if( point.x < pic_x1 ) point.x = pic_x1;
	if( point.x > pic_x2 ) point.x = pic_x2;
	if( point.y < pic_y1 ) point.y = pic_y1;
	if( point.y > pic_y2 ) point.y = pic_y2;

	Draw_Bitmap();

	CClientDC aDC( this );

	CDC* pDC = &aDC;
}

double COctopusPictureDisplay::ScreenYToStageX(int py)
{
	double xc = double(py - pic_edge_y - 350) / double(pic_size_u16);
	xc = xc * magfactor;
	xc = xc + B.position_x_686;
	return xc;
}

int COctopusPictureDisplay::StageXToScreenY(double x)
{
	double xc = x - B.position_x_686;
	xc = xc / magfactor * double(pic_size_u16);	
	return u16(xc + 350 + pic_edge_y);
}

double COctopusPictureDisplay::ScreenXToStageY(int px)
{
	double yc = double(px - pic_edge_x - 350);
	yc = yc / double(pic_size_u16);
	yc = yc * magfactor;
	yc = yc + B.position_y_686;
	return yc;
}

int COctopusPictureDisplay::StageYToScreenX(double yc)
{
	//corrected
	yc = yc - B.position_y_686;
    yc = yc / magfactor;
    yc = yc * double(pic_size_u16);
	return u16(yc + 350 + pic_edge_x);
}


void COctopusPictureDisplay::LeftButtonDown( CPoint point ) 
{	
	if( point.x < pic_x1 ) point.x = pic_x1;
	if( point.x > pic_x2 ) point.x = pic_x2;
	if( point.y < pic_y1 ) point.y = pic_y1;
	if( point.y > pic_y2 ) point.y = pic_y2;

	double y = ScreenXToStageY(point.x);
	double x = ScreenYToStageX(point.y);

	if ( Setting_Cell == true )
	{
		//set it
		B.CellParams.x		= x;
		B.CellParams.y		= y;
		B.CellParams.z		= B.position_z_sample;

		Setting_Cell		= false;
		
		GetDlgItem( IDC_CAS_DISP_CELL_ADD )->EnableWindow( true );

		UpdateCellInfo();

		if(glob_m_pStage686 != NULL) glob_m_pStage686->CellAdd();
	}
}

void COctopusPictureDisplay::OnMouseMove( UINT nFlags, CPoint point ) 
{
	if ( Setting_Cell ) 
	{
		SetCursor(LoadCursor(NULL,IDC_HAND));
		MouseMove( point );
	}
}

void COctopusPictureDisplay::OnLButtonDown( UINT nFlags, CPoint point ) 
{
	if ( Setting_Cell ) 
		LeftButtonDown( point );
}

BOOL COctopusPictureDisplay::OnCommand(WPARAM wParam, LPARAM lParam)
{
	int id = LOWORD(wParam);    // Notification code
	if( id == 1 ) return FALSE; // Trap RTN key
	if( id == 2 ) return FALSE; // Trap ESC key
    return CDialog::OnCommand(wParam, lParam);
}