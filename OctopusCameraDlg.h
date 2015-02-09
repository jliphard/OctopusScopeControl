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
#if !defined(AFX_H_OctopusCamera)
#define AFX_H_OctopusCamera

#include "stdafx.h"
#include "OctopusGlobals.h"

class COctopusCamera : public CDialog
{

public:
	
	COctopusCamera(CWnd* pParent = NULL);
	virtual ~COctopusCamera();

	enum { IDD = IDC_CAS };

	void SetROI_To_Default( void );

	void StartSaving( void );
	void StopSaving( void );

	void StartMovie( void );
	void StopCameraThread( void );

	void TakePicture( void );

	void TakePicture( double exposuretime_ms, int gain );

	void FileClose( void );

	u32 GetExposureTime_Single_ms( void ) { return u32_Exposure_Time_Single_ms; };

protected:

	void SetROI( void );
	void BinChange( void ); 
	void StartMovie(  float exptime_ms, u8 gain );

	CBitmap m_bmp_yes;
	CBitmap m_bmp_no;

	CStatic	m_Temperature_Now;
	CStatic m_status_save;

	CStatic m_g1_text1;
	CStatic m_g1_text2;

	CButton m_ctlFTCheckBox;

	bool FrameTransfer;

	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual BOOL OnInitDialog();

	void OpenWindow( void );
	
	CString pathname_display;	
	CString debug;

	HANDLE memhandle;

	afx_msg void OnSetPath();

	int m_DISPLAY_GAIN_CHOICE;
	int m_DISPLAY_BIN_CHOICE;
	int m_DISPLAY_VSSPEED;
	int m_DISPLAY_HSSPEED;
	int m_DISPLAY_GL;
	double m_IDC_MANUAL_GAIN;
	u32 m_IDC_PICTURES_PER_FILE;

	u32 u32_Exposure_Time_Single_ms;
	u32 u32_Exposure_Time_Movie_ms;
	
	int  u16_Gain_Mult_Factor_Single;
	int  u16_Gain_Mult_Factor_Movie;

	int m_CCD_target_temp;
	int m_CCD_current_temp;

	u16 em_limit; 

    void Opencam( void );
	
	void EnableDlg( void );
	void DisableDlg( void );
	void DisableDlgMovie( void );
	void OnFileChange( void );

	afx_msg void OnBinning1x1( void );
	afx_msg void OnBinning2x2( void );
	afx_msg void OnBinning4x4( void );
	afx_msg void OnBinning8x8( void );
		
	afx_msg void OnVSSPEED_0( void );
	afx_msg void OnVSSPEED_1( void );
	afx_msg void OnVSSPEED_2( void );
	afx_msg void OnVSSPEED_3( void );
	afx_msg void OnVSSPEED_4( void );

	void OnHSSPEED_0( void );
	void OnHSSPEED_1( void );
	void OnHSSPEED_2( void );
	void OnHSSPEED_3( void );
	void OnHSSPEED_4( void );
	void OnHSSPEED_5( void );

	afx_msg void OnGL_0( void );
	afx_msg void OnGL_1( void );
	afx_msg void OnGL_2( void );

	void OnVSSPEED_Report( void );
	void OnHSSPEED_Report( void );

	afx_msg void OnResizeWindow();
	afx_msg void OnKillfocusGeneral();
	afx_msg void OnKillfocusTempTarget();
	afx_msg void OnKillfocusManualGain();
	afx_msg void OnKillfocusPicturesPerFile();
	afx_msg void OnDisplayGainManual();
	afx_msg void OnDisplayGainAutomatic();

	afx_msg void OnTimer(UINT nIDEvent);

	DECLARE_MESSAGE_MAP()

	void StartWaitThread( void );  //Starts the thread to wait for SDK events
	void KillWaitThread( void );   //Terminates the SDK event thread

	HANDLE WaitThread;
	
public:

	afx_msg void OnBnClickedCasFt();
};

#endif
