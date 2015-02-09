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

#if !defined(AFX_H_OctopusMultifunction)
#define AFX_H_OctopusMultifunction

#include "stdafx.h"
#include "OctopusGlobals.h"
#include "NIDAQmx.h"

class COctopusMultifunction : public CDialog
{

public:
	
	COctopusMultifunction(CWnd* pParent = NULL);
	virtual ~COctopusMultifunction();
	enum { IDD = IDC_NI };

	//void LED_On( void );
	//void LED_On( u16 intensity );
	//void LED_Off( void );

	void TTL_Pulse_Up( void );
	void TTL_Up( u8 port, u8 pin );
	void TTL_Down( u8 port, u8 pin );
	void AO(u8 Channel, double value);

protected:

	int			error;

	TaskHandle  AItaskHandle;
	
	TaskHandle  AOtaskHandle0;
	TaskHandle  AOtaskHandle1;
	TaskHandle  AOtaskHandle2;
	TaskHandle  AOtaskHandle3;

	TaskHandle  DOtaskHandle;

	//float64     data_out[1];
	//float64     data_in[100];
	float64     value_in;

	float64     value_out;
	//float64     value_out1;
	//float64     value_out2;
	//float64     value_out3;

	uInt8       data_DO[8];
	//float64     data_AO[4];

	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual BOOL OnInitDialog();

//	void LED_SetIntensity ( u16 intensity );
    void UpdateLEDIntensity( void );
	
	DECLARE_MESSAGE_MAP()

	//CSliderCtrl m_Slider;
	//CStatic     m_Slider_Text;
	//CString     m_Slider_String;

	CStatic     m_ADC_Text;

	//u16 LED_intensity_setpoint;
	//u16 LED_intensity_current;

	//float64     dataAO[1000];
	//float64		rate;
	//uInt64		sampsPerChan;

	bool first_tick;

	afx_msg void OnTimer(UINT nIDEvent);

//public:
//	afx_msg void OnNMCustomdrawLedIntensitySlider(NMHDR *pNMHDR, LRESULT *pResult);
};

#endif