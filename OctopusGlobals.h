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

#if !defined(AFX_H_OctopusCascadeGlobals)
#define AFX_H_OctopusCascadeGlobals

#include "stdafx.h"

struct COctopusROI 
{
	int x1;
	int x2;
	int y1;
	int y2;
	int bin;
};

struct COctopusCellPosition
{
	double x;
	double y;
	double z;
};

struct COctopusCentroid
{
	double X;
	double Y;
	double Intensity;
	double max;
	double min;
	double mean;
	double totalint;
	double cut_high;

	double LM_X;
	double LM_Y;
	double LM_X_sd;
	double LM_Y_sd;
	double LM_fwhm;

};

struct COctopusGlobals 
{
	u32		files_written;
	bool	savetofile;
	u16		W;
	u16		H;
	bool	automatic_gain;
	double	manual_gain;
	u32		pics_per_file;
	u16		CCD_x_phys_e;
	u16		CCD_y_phys_e;
	bool	Camera_Thread_running;
	CString pathname;
	u16     bin;
	u16*    memory;
	double  savetime;
	u32     expt_frame;
	double  expt_time;
	double  time_now;
	bool    Ampl_Conv;
	//u32     Ampl_Setting;
	u32     Ampl_Setting_old;
	u32     CameraExpTime_ms;
	bool    Andor_new;
	
	bool        ROI_changed;
	COctopusROI ROI_actual;
	COctopusROI ROI_target;
	COctopusROI Focus_ROI;
	bool        Focus_ROI_Set;
	bool        SetFocus;

	//Shutter and filter
	u8      nd_setting;
	bool    load_wheel_failed;
	u8      filter_wheel;
	bool    program_running;
	
	//autofocus
	bool    focus_in_progress;
	double  focus_score;
	double	focus_beadX;
	double	focus_beadY;
	double	focus_beadX_LM;
	double	focus_beadY_LM;
	double  focus_beadX_sd;
	double  focus_beadY_sd; 
	double  focus_bead_fwhm; 
	double	focus_min;
	double	focus_max;

	//piezo and scope
	double  position_x_686;
	double  position_y_686;
	double  position_z_sample;
	double  position_z_objective;

//	double  position_x_volt;
//	double  position_y_volt;
//	double  position_z_volt;

	double ADC_1;

	bool NI_loaded;

	bool Laser_405_is_On;
	bool Laser_488_is_On;
	bool Laser_561_is_On;
	bool Laser_639_is_On;

	bool Lasers_loaded;

	bool Cleaning;

	//AOTF
	bool AOTF_loaded;
	bool AOTF_running;

	COctopusCellPosition  CellParams;

	double position_motor_mm; 

	bool Scope_loaded;
	u8   Scope_epiwheel;
	u8   Scope_objective;

	unsigned int AUTO_CellLabel;

	CListBox CellList;

	CString cellstring;

};



#endif