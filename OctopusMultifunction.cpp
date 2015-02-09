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
#include "OctopusMultifunction.h"
#include "NIDAQmx.h"

extern COctopusGlobals B;

COctopusMultifunction::COctopusMultifunction(CWnd* pParent)
	: CDialog(COctopusMultifunction::IDD, pParent)
{    

	error        = 0;
	
	AItaskHandle  = 0;
	AOtaskHandle0 = 0;
	AOtaskHandle1 = 0;
	AOtaskHandle2 = 0;
	AOtaskHandle3 = 0;
	DOtaskHandle  = 0;

	value_out     = 0.0;
	value_in      = 0.0;

	//rate         = 1000; //samples per second
	//sampsPerChan = 1000;

	for (int j = 0; j < 8; j++) data_DO[j] = 0;
	//for (int j = 0; j < 4; j++) data_AO[j] = 0;
    
	//LED_intensity_setpoint = 0;
	//LED_intensity_current  = 0;
	//first_tick             = true;
    //B.ADC_1                = 0.0;

	if( Create(COctopusMultifunction::IDD, pParent) ) 
		ShowWindow( SW_SHOW );

	//m_Slider.SetRange( 0, 100 );
	//m_Slider.SetPos( 100 );
	//m_Slider.SetTicFreq( 5 );

	SetTimer( TIMER_NI, 500, NULL );
}

BOOL COctopusMultifunction::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	SetWindowPos(NULL, 605, 57, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	//SetWindowPos(NULL, 493, 775, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	/*********************************************/
	// DAQmx Configure Code
	/*********************************************/
	DAQmxCreateTask("",&AOtaskHandle0);
	DAQmxCreateAOVoltageChan(AOtaskHandle0,"Dev1/ao0","",-10.0,10.0,DAQmx_Val_Volts,"");

	DAQmxCreateTask("",&AOtaskHandle1);
	DAQmxCreateAOVoltageChan(AOtaskHandle1,"Dev1/ao1","",-10.0,10.0,DAQmx_Val_Volts,"");

	DAQmxCreateTask("",&AOtaskHandle2);
	DAQmxCreateAOVoltageChan(AOtaskHandle2,"Dev1/ao2","",-10.0,10.0,DAQmx_Val_Volts,"");

	DAQmxCreateTask("",&AOtaskHandle3);
	DAQmxCreateAOVoltageChan(AOtaskHandle3,"Dev1/ao3","",-10.0,10.0,DAQmx_Val_Volts,"");
	
	//DAQmxCreateTask("",&AOtaskHandle);
	//DAQmxCreateAOVoltageChan(AOtaskHandle,"Dev1/ao0","",-10.0,10.0,DAQmx_Val_Volts,"");
	//DAQmxCfgSampClkTiming(AOtaskHandle,"OnboardClock",rate,DAQmx_Val_Rising,DAQmx_Val_ContSamps,sampsPerChan);

	DAQmxCreateTask("",&AItaskHandle);
	DAQmxCreateAIVoltageChan(AItaskHandle,"Dev1/ai0","",DAQmx_Val_Cfg_Default,-10.0,10.0,DAQmx_Val_Volts,"");

	DAQmxCreateTask("",&DOtaskHandle);
	DAQmxCreateDOChan(DOtaskHandle,"Dev1/port1/line0:7","",DAQmx_Val_ChanForAllLines);
	/*********************************************/
	// DAQmx Start Code
	/*********************************************/
	DAQmxStartTask(AItaskHandle);

	DAQmxStartTask(AOtaskHandle0);
	DAQmxStartTask(AOtaskHandle1);
	DAQmxStartTask(AOtaskHandle2);
	DAQmxStartTask(AOtaskHandle3);
	
	DAQmxStartTask(DOtaskHandle);

	return TRUE;
}

void COctopusMultifunction::DoDataExchange(CDataExchange* pDX) 
{
	CDialog::DoDataExchange( pDX );
	//DDX_Control(pDX,	IDC_NI_INTENSITY_SLIDER, m_Slider);
	//DDX_Control(pDX,	IDC_NI_INTENSITY,        m_Slider_Text);
	DDX_Control(pDX,	IDC_NI_ADC,              m_ADC_Text);
}  

BEGIN_MESSAGE_MAP(COctopusMultifunction, CDialog)
	ON_WM_TIMER()
	//ON_NOTIFY(NM_CUSTOMDRAW, IDC_NI_INTENSITY_SLIDER, OnNMCustomdrawLedIntensitySlider)
END_MESSAGE_MAP()

COctopusMultifunction::~COctopusMultifunction() 
{  
	if( AOtaskHandle0 != 0 ) 
	{
		DAQmxStopTask(AOtaskHandle0);
		DAQmxClearTask(AOtaskHandle0);
	}
	if( AOtaskHandle1 != 0 ) 
	{
		DAQmxStopTask(AOtaskHandle1);
		DAQmxClearTask(AOtaskHandle1);
	}
	if( AOtaskHandle2 != 0 ) 
	{
		DAQmxStopTask(AOtaskHandle2);
		DAQmxClearTask(AOtaskHandle2);
	}
	if( AOtaskHandle3 != 0 ) 
	{
		DAQmxStopTask(AOtaskHandle3);
		DAQmxClearTask(AOtaskHandle3);
	}
	if( AItaskHandle != 0 ) 
	{
		DAQmxStopTask(AItaskHandle);
		DAQmxClearTask(AItaskHandle);
	}
	if( DOtaskHandle != 0 ) 
	{
		DAQmxStopTask(DOtaskHandle);
		DAQmxClearTask(DOtaskHandle);
	}
}

void COctopusMultifunction::TTL_Up( u8 port, u8 pin )
{
	if ( pin > 7 ) return;
	data_DO[pin] = 1;
	DAQmxWriteDigitalLines(DOtaskHandle,1,1,10.0,DAQmx_Val_GroupByChannel,data_DO,NULL,NULL);
	//TTL is now high
}

void COctopusMultifunction::TTL_Down( u8 port, u8 pin )
{
	if ( pin > 7 ) return;
	data_DO[pin] = 0;
	DAQmxWriteDigitalLines(DOtaskHandle,1,1,10.0,DAQmx_Val_GroupByChannel,data_DO,NULL,NULL);
	//TTL is now low
}

void COctopusMultifunction::TTL_Pulse_Up( void )
{
	TTL_Up( 1, 0 );
	Sleep(10);
	TTL_Down( 1, 0 );
}
/*
void COctopusMultifunction::LED_On( void )
{
	LED_SetIntensity( LED_intensity_setpoint );
}

void COctopusMultifunction::LED_On( u16 intensity )
{
	LED_intensity_setpoint = intensity;
	LED_SetIntensity( LED_intensity_setpoint );
}

void COctopusMultifunction::LED_Off( void )
{
	LED_SetIntensity( 0 );
}

void COctopusMultifunction::LED_SetIntensity( u16 intensity ) 
{

	if ( intensity > 100 ) intensity = 100;
	if ( intensity <   0 ) intensity =   0;

	LED_intensity_current = intensity;

	//1.5 volts = max brightness
	//4.5 and greater volts = off

	value_out = 4.5 - (intensity * 0.030);

	if ( value_out > 4.5 ) value_out = 4.5;
	if ( value_out < 1.5 ) value_out = 1.5;

	DAQmxWriteAnalogScalarF64(AOtaskHandle, TRUE, 10.0, value_out, NULL);

	UpdateLEDIntensity();
}
*/
void COctopusMultifunction::UpdateLEDIntensity( void )
{
	if( IsWindowVisible() && !B.focus_in_progress ) 
	{
		CString str;
		
		//str.Format(_T("LED Intensity:\n%d %%"), LED_intensity_current );
		//m_Slider_Text.SetWindowText( str );
		//m_Slider.SetPos( 100 - LED_intensity_current );

		str.Format(_T("ADC signal:\n%.2f C"), B.ADC_1 );
		m_ADC_Text.SetWindowText( str );

		UpdateData( false );
	}
}

BOOL COctopusMultifunction::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	int id = LOWORD(wParam);     // Notification code
	if( id == 2 ) return FALSE;  // Trap ESC key
	if( id == 1 ) return FALSE;  // Trap RTN key
    return CDialog::OnCommand(wParam, lParam);
}

void COctopusMultifunction::OnTimer( UINT nIDEvent ) 
{
	if( nIDEvent == TIMER_NI ) 
	{
		if( first_tick ) 
		{
			//LED_On( 50 );
			first_tick = false;
		}

		/*********************************************/
		// DAQmx Read Code
		/*********************************************/
		DAQmxReadAnalogScalarF64(AItaskHandle,10.0,&value_in,0);
		
		//50 mV/C
		B.ADC_1 = (double)value_in * 20.0;

		UpdateLEDIntensity();
	}
	CDialog::OnTimer(nIDEvent);
}
/*
void COctopusMultifunction::OnNMCustomdrawLedIntensitySlider(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	
	u16 CurPos = m_Slider.GetPos();
	
	if ( CurPos > 100 ) CurPos = 100;
	if ( CurPos <   0 ) CurPos =   0;
	
	LED_intensity_setpoint = 100 - CurPos;

	if ( LED_intensity_setpoint > 0 )
		LED_On( LED_intensity_setpoint );
	else
		LED_Off();
	
	*pResult = 0;
}
*/

void COctopusMultifunction::AO( u8 Channel, double value ) 
{

	if ( value > 10.0 ) 
		value_out = 10.0;
	else if ( value < 0.0 ) 
		value_out = 0.0;
	else 
		value_out = value;

	if( Channel == 0 )
		DAQmxWriteAnalogScalarF64(AOtaskHandle0, TRUE, 10.0, value_out, NULL);
	else if (Channel == 1 )
		DAQmxWriteAnalogScalarF64(AOtaskHandle1, TRUE, 10.0, value_out, NULL);
	else if (Channel == 2 )
		DAQmxWriteAnalogScalarF64(AOtaskHandle2, TRUE, 10.0, value_out, NULL);
	else if (Channel == 3 )
		DAQmxWriteAnalogScalarF64(AOtaskHandle3, TRUE, 10.0, value_out, NULL);

}