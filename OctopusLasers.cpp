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
#include "OctopusLasers.h"
#include "OctopusMultifunction.h"

extern COctopusMultifunction*   glob_m_pNI;
extern COctopusGlobals B;

COctopusLasers::COctopusLasers(CWnd* pParent)
	: CDialog(COctopusLasers::IDD, pParent)
{    

	B.Laser_405_is_On = false;
	B.Laser_561_is_On = false;
	B.Laser_488_is_On = false;
	B.Laser_639_is_On = false;

	pulsing = false;

	lasertopulse = 1;

	pulsetime_405     = 100;
	pulsetime_488     = 100;
	pulsetime_561     = 100;
	pulsetime_639     = 100;

	pulsedelay_405    = 1000;
	pulsedelay_488    = 1000;
	pulsedelay_561    = 1000;
	pulsedelay_639    = 1000;

	volts_CH1 = 0.0;
	volts_CH2 = 0.0;
	
	VERIFY(m_bmp_no.LoadBitmap(IDB_NO));
	VERIFY(m_bmp_yes.LoadBitmap(IDB_YES));

	if( Create(COctopusLasers::IDD, pParent) ) 
		ShowWindow( SW_SHOW );

	m_Slider_CH1.SetRange( 0, 100 );
	m_Slider_CH1.SetPos( 10 );
	m_Slider_CH1.SetTicFreq( 5 );

	m_Slider_CH2.SetRange( 0, 100 );
	m_Slider_CH2.SetPos( 10 );
	m_Slider_CH2.SetTicFreq( 5 );

	B.Lasers_loaded = true;

}

BOOL COctopusLasers::OnInitDialog() 
{
	CDialog::OnInitDialog();
	SetWindowPos(NULL, 337, 809, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	return TRUE;
}

void COctopusLasers::DoDataExchange(CDataExchange* pDX) 
{
	CDialog::DoDataExchange( pDX );
	DDX_Control(pDX, IDC_LASER_405ONOFF_BMP, m_status_405);
	DDX_Control(pDX, IDC_LASER_488ONOFF_BMP, m_status_488);
	DDX_Control(pDX, IDC_LASER_561ONOFF_BMP, m_status_561);
	DDX_Control(pDX, IDC_LASER_639ONOFF_BMP, m_status_639);

	DDX_Control(pDX, IDC_LASER_405_PULSE_BMP, m_status_405_Pulse);
	DDX_Control(pDX, IDC_LASER_488_PULSE_BMP, m_status_488_Pulse);
	DDX_Control(pDX, IDC_LASER_561_PULSE_BMP, m_status_561_Pulse);
	DDX_Control(pDX, IDC_LASER_639_PULSE_BMP, m_status_639_Pulse);
	
	DDX_Control(pDX, IDC_LASER_488_INT, m_Slider_CH1);
	DDX_Control(pDX, IDC_LASER_561_INT, m_Slider_CH2);

	DDX_Text( pDX, IDC_LASER_405_PULSE_ONTIME, pulsetime_405);
	DDV_MinMaxInt(pDX, pulsetime_405, 50, 1000);
	DDX_Text( pDX, IDC_LASER_405_PULSE_DELAY, pulsedelay_405);
	DDV_MinMaxInt(pDX, pulsedelay_405, 100, 10000);

	DDX_Text( pDX, IDC_LASER_488_PULSE_ONTIME, pulsetime_488);
	DDV_MinMaxInt(pDX, pulsetime_488, 50, 1000);
	DDX_Text( pDX, IDC_LASER_488_PULSE_DELAY, pulsedelay_488);
	DDV_MinMaxInt(pDX, pulsedelay_488, 100, 10000);

	DDX_Text( pDX, IDC_LASER_561_PULSE_ONTIME, pulsetime_561);
	DDV_MinMaxInt(pDX, pulsetime_561, 50, 1000);
	DDX_Text( pDX, IDC_LASER_561_PULSE_DELAY, pulsedelay_561);
	DDV_MinMaxInt(pDX, pulsedelay_561, 100, 10000);

	DDX_Text( pDX, IDC_LASER_639_PULSE_ONTIME, pulsetime_639);
	DDV_MinMaxInt(pDX, pulsetime_639, 50, 1000);
	DDX_Text( pDX, IDC_LASER_639_PULSE_DELAY, pulsedelay_639);
	DDV_MinMaxInt(pDX, pulsedelay_639, 100, 10000);

}  

BEGIN_MESSAGE_MAP(COctopusLasers, CDialog)

	ON_BN_CLICKED(IDC_LASER_405,	    OnClicked405)
	ON_BN_CLICKED(IDC_LASER_488,	    OnClicked488)
	ON_BN_CLICKED(IDC_LASER_561,	    OnClicked561)
	ON_BN_CLICKED(IDC_LASER_639,	    OnClicked639)

	ON_BN_CLICKED(IDC_LASER_405_PULSE,	OnClicked405Pulse)
	ON_BN_CLICKED(IDC_LASER_488_PULSE,	OnClicked488Pulse)
	ON_BN_CLICKED(IDC_LASER_561_PULSE,	OnClicked561Pulse)
	ON_BN_CLICKED(IDC_LASER_639_PULSE,	OnClicked639Pulse)

	ON_EN_KILLFOCUS(IDC_LASER_405_PULSE_ONTIME, OnKillfocusGeneral)
	ON_EN_KILLFOCUS(IDC_LASER_405_PULSE_DELAY,  OnKillfocusGeneral)

	ON_EN_KILLFOCUS(IDC_LASER_488_PULSE_ONTIME, OnKillfocusGeneral)
	ON_EN_KILLFOCUS(IDC_LASER_488_PULSE_DELAY,  OnKillfocusGeneral)

	ON_EN_KILLFOCUS(IDC_LASER_561_PULSE_ONTIME, OnKillfocusGeneral)
	ON_EN_KILLFOCUS(IDC_LASER_561_PULSE_DELAY,  OnKillfocusGeneral)

	ON_EN_KILLFOCUS(IDC_LASER_639_PULSE_ONTIME, OnKillfocusGeneral)
	ON_EN_KILLFOCUS(IDC_LASER_639_PULSE_DELAY,  OnKillfocusGeneral)

	ON_NOTIFY(NM_CUSTOMDRAW, IDC_LASER_488_INT, OnIntensitySliderCH1)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_LASER_561_INT, OnIntensitySliderCH2)

	ON_WM_TIMER()

END_MESSAGE_MAP()

COctopusLasers::~COctopusLasers() 
{  
	B.Lasers_loaded = false;
}

void COctopusLasers::OnKillfocusGeneral() { UpdateData( true ); }

void COctopusLasers::OnTimer( UINT nIDEvent ) 
{
	if( nIDEvent == TIMER_LASER_PULSE ) 
	{
		if (lasertopulse == 1)
			Laser_405_On( pulsetime_405 ); 
		else if (lasertopulse == 2)
			Laser_488_On( pulsetime_488 ); 
		else if (lasertopulse == 3)
			Laser_561_On( pulsetime_561 ); 
		else if (lasertopulse == 4)
			Laser_639_On( pulsetime_639 ); 
	}	

	CDialog::OnTimer(nIDEvent);
}

void COctopusLasers::OnClicked405Pulse()
{
	if ( pulsing ) {
		//turn it off
		m_status_405_Pulse.SetBitmap( m_bmp_no );
		KillTimer( m_nTimer );
		GetDlgItem( IDC_LASER_488_PULSE )->EnableWindow( true  );
		GetDlgItem( IDC_LASER_561_PULSE )->EnableWindow( true  );
		GetDlgItem( IDC_LASER_639_PULSE )->EnableWindow( true  );
		Laser_405_Off();
		pulsing = false;
	} else {
		//turn it on
		lasertopulse = 1;
		m_nTimer = SetTimer( TIMER_LASER_PULSE, pulsedelay_405, NULL );
		m_status_405_Pulse.SetBitmap( m_bmp_yes );
		GetDlgItem( IDC_LASER_488_PULSE )->EnableWindow( false );
		GetDlgItem( IDC_LASER_561_PULSE )->EnableWindow( false );
		GetDlgItem( IDC_LASER_639_PULSE )->EnableWindow( false );
		pulsing = true;
	}
}

void COctopusLasers::OnClicked488Pulse()
{
	if ( pulsing ) {
		//turn it off
		m_status_488_Pulse.SetBitmap( m_bmp_no );
		KillTimer( m_nTimer );
		GetDlgItem( IDC_LASER_405_PULSE )->EnableWindow( true  );
		GetDlgItem( IDC_LASER_561_PULSE )->EnableWindow( true  );
		GetDlgItem( IDC_LASER_639_PULSE )->EnableWindow( true  );
		Laser_488_Off();
		pulsing = false;
	} else {
		//turn it on
		lasertopulse = 2;
		m_nTimer = SetTimer( TIMER_LASER_PULSE, pulsedelay_488, NULL );
		m_status_488_Pulse.SetBitmap( m_bmp_yes );
		GetDlgItem( IDC_LASER_405_PULSE )->EnableWindow( false );
		GetDlgItem( IDC_LASER_561_PULSE )->EnableWindow( false );
		GetDlgItem( IDC_LASER_639_PULSE )->EnableWindow( false );
		pulsing = true;
	}
}

void COctopusLasers::OnClicked561Pulse()
{
	if ( pulsing ) {
		//turn it off
		m_status_561_Pulse.SetBitmap( m_bmp_no );
		KillTimer( m_nTimer );
		GetDlgItem( IDC_LASER_488_PULSE )->EnableWindow( true  );
		GetDlgItem( IDC_LASER_405_PULSE )->EnableWindow( true  );
		GetDlgItem( IDC_LASER_639_PULSE )->EnableWindow( true  );
		Laser_561_Off();
		pulsing = false;
	} else {
		//turn it on
		lasertopulse = 3;
		m_nTimer = SetTimer( TIMER_LASER_PULSE, pulsedelay_561, NULL );
		m_status_561_Pulse.SetBitmap( m_bmp_yes );
		GetDlgItem( IDC_LASER_488_PULSE )->EnableWindow( false );
		GetDlgItem( IDC_LASER_405_PULSE )->EnableWindow( false );
		GetDlgItem( IDC_LASER_639_PULSE )->EnableWindow( false );
		pulsing = true;
	}
}

void COctopusLasers::OnClicked639Pulse()
{
	if ( pulsing ) {
		//turn it off
		m_status_639_Pulse.SetBitmap( m_bmp_no );
		KillTimer( m_nTimer );
		GetDlgItem( IDC_LASER_488_PULSE )->EnableWindow( true  );
		GetDlgItem( IDC_LASER_561_PULSE )->EnableWindow( true  );
		GetDlgItem( IDC_LASER_405_PULSE )->EnableWindow( true  );
		Laser_639_Off();
		pulsing = false;
	} else {
		//turn it on
		lasertopulse = 4;
		m_nTimer = SetTimer( TIMER_LASER_PULSE, pulsedelay_639, NULL );
		m_status_639_Pulse.SetBitmap( m_bmp_yes );
		GetDlgItem( IDC_LASER_488_PULSE )->EnableWindow( false );
		GetDlgItem( IDC_LASER_561_PULSE )->EnableWindow( false );
		GetDlgItem( IDC_LASER_405_PULSE )->EnableWindow( false );
		pulsing = true;
	}
}

void COctopusLasers::Laser_405_On( unsigned int ontime ) 
{		
		Laser_405_On();
		Sleep(ontime);
		Laser_405_Off();
}

void COctopusLasers::Laser_488_On( unsigned int ontime ) 
{		
		Laser_488_On();
		Sleep(ontime);
		Laser_488_Off();
}

void COctopusLasers::Laser_561_On( unsigned int ontime ) 
{		
		Laser_561_On();
		Sleep(ontime);
		Laser_561_Off();
}

void COctopusLasers::Laser_639_On( unsigned int ontime ) 
{		
		Laser_639_On();
		Sleep(ontime);
		Laser_639_Off();
}


void COctopusLasers::OnClicked405()
{
	if ( B.Laser_405_is_On ) {
		Laser_405_Off();
	} else {
		Laser_405_On();
	}
}

void COctopusLasers::OnClicked488()
{
	if ( B.Laser_488_is_On ) {
		Laser_488_Off();
	} else {
		Laser_488_On();
	}
}

void COctopusLasers::OnClicked561()
{
	if ( B.Laser_561_is_On ) {
		Laser_561_Off();
	} else {
		Laser_561_On();
	}
}

void COctopusLasers::OnClicked639()
{
	if ( B.Laser_639_is_On ) {
		Laser_639_Off();
	} else {
		Laser_639_On();
	}
}

void COctopusLasers::Laser_405_On( void ) {		
		if (glob_m_pNI == NULL) return;
		glob_m_pNI->TTL_Up(1,0);
		B.Laser_405_is_On = true;
		m_status_405.SetBitmap( m_bmp_yes );
}

void COctopusLasers::Laser_405_Off( void ) {		
		if (glob_m_pNI == NULL) return;
		glob_m_pNI->TTL_Down(1,0);
		B.Laser_405_is_On = false;	
		m_status_405.SetBitmap( m_bmp_no );
}

void COctopusLasers::Laser_488_On( void ) {		
		if (glob_m_pNI == NULL) return;
		glob_m_pNI->TTL_Up(1,1);
		B.Laser_488_is_On = true;
		m_status_488.SetBitmap( m_bmp_yes );
}

void COctopusLasers::Laser_488_Off( void ) {		
		if (glob_m_pNI == NULL) return;
		glob_m_pNI->TTL_Down(1,1);
		B.Laser_488_is_On = false;	
		m_status_488.SetBitmap( m_bmp_no );
}

void COctopusLasers::Laser_561_On( void ) {		
		if (glob_m_pNI == NULL) return;
		glob_m_pNI->TTL_Up(1,2);
		B.Laser_561_is_On = true;
		m_status_561.SetBitmap( m_bmp_yes );
}

void COctopusLasers::Laser_561_Off( void ) {		
		if (glob_m_pNI == NULL) return;
		glob_m_pNI->TTL_Down(1,2);
		B.Laser_561_is_On = false;	
		m_status_561.SetBitmap( m_bmp_no );
}

void COctopusLasers::Laser_639_On( void ) {		
		if (glob_m_pNI == NULL) return;
		glob_m_pNI->TTL_Up(1,3);
		B.Laser_639_is_On = true;
		m_status_639.SetBitmap( m_bmp_yes );
}

void COctopusLasers::Laser_639_Off( void ) {		
		if (glob_m_pNI == NULL) return;
		glob_m_pNI->TTL_Down(1,3);
		B.Laser_639_is_On = false;	
		m_status_639.SetBitmap( m_bmp_no );
}

BOOL COctopusLasers::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	int id = LOWORD(wParam);     // Notification code
	if( id == 2 ) return FALSE;  // Trap ESC key
	if( id == 1 ) return FALSE;  // Trap RTN key
    return CDialog::OnCommand(wParam, lParam);
}

void COctopusLasers::OnIntensitySliderCH1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	
	u16 CurPos = m_Slider_CH1.GetPos();
	
	if ( CurPos > 100 ) 
		volts_CH1 = 10.0;
	else if ( CurPos <   0 ) 
		volts_CH1 = 0.0;
	else 
		volts_CH1 = double(CurPos)/10.0;
	
	if (glob_m_pNI == NULL) return;

	glob_m_pNI->AO(0, volts_CH1);
	glob_m_pNI->AO(1,      10.0);  //old one

	*pResult = 0;
}

void COctopusLasers::OnIntensitySliderCH2(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	
	u16 CurPos = m_Slider_CH2.GetPos();
	
	if ( CurPos > 100 ) 
		volts_CH2 = 10.0;
	else if ( CurPos <   0 ) 
		volts_CH2 = 0.0;
	else 
		volts_CH2 = double(CurPos)/10.0;
	
	if (glob_m_pNI == NULL) return;

	glob_m_pNI->AO(2, volts_CH2);
	glob_m_pNI->AO(3,       5.0); //new one

	*pResult = 0;
}

