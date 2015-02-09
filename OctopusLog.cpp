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
#include "OctopusClock.h"
#include "OctopusLog.h"
#include <stdio.h>

extern COctopusGoodClock* glob_m_pGoodClock;

COctopusLog::COctopusLog()
{
	pFile = NULL;
	entry = 0;
	Open_File();
}

COctopusLog::~COctopusLog() 
{
	if ( pFile != NULL ) 
	{
		fclose( pFile );
		pFile = NULL;
	}
}

bool COctopusLog::Open_File( void ) 
{
	pFile = fopen( "debuglog.txt" , "wt");

	if ( pFile != NULL ) 
		return true;
	else 
		return false;
}

void COctopusLog::Write( CString step ) 
{
	if( pFile == NULL ) return; 

	CString out;

	systemtime = CTime::GetCurrentTime();
	CString t = systemtime.Format(_T(" %m/%d/%y %H:%M:%S "));

	out.Format(_T("N:%d T:%.3f"), entry++, (double)glob_m_pGoodClock->End());
	out.Append( t );
	out.Append( step );
	out.Append(_T("\n"));

	//fwprintf( pFile, out );	

	fprintf( pFile, out );	
	fflush( pFile );
}