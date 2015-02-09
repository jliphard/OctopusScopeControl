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
//////////////////////////////////////////////////////////////////////
// OctopusClock.cpp: implementation of the COctopusGoodClock class.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Octopus.h"
#include "OctopusClock.h"

COctopusGoodClock::COctopusGoodClock() { // get the frequency of the counter
	
	if(QueryPerformanceFrequency( (LARGE_INTEGER *)&Frequency )) 
	{  
		NotInitialized = false;
	} else {
		NotInitialized = true;
	}

	if(QueryPerformanceCounter( (LARGE_INTEGER *)&BeginTime )) 
	{  
		NotInitialized = false;
	} else {
		NotInitialized = true;
	}
}

COctopusGoodClock::~COctopusGoodClock() {}

double COctopusGoodClock::End(void) const{   // stop timing and get elapsed time in seconds

      if(NotInitialized) return 0.0;         // error - couldn't get frequency

      // get the ending counter value
      QueryPerformanceCounter((LARGE_INTEGER *)&EndTime);

      // determine the elapsed counts
      __int64 Elapsed = EndTime - BeginTime;

      // convert counts to time in seconds and return it
      return (double)Elapsed / (double)Frequency;
}
