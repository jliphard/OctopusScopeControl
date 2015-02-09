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
////////////////////////////////////////////////////////////////////////
// stdafx.h
////////////////////////////////////////////////////////////////////////

#if !defined(AFX_H_STDAFX)
#define AFX_H_STDAFX

typedef signed   char  sC;
typedef unsigned char  uC;
typedef signed   char  s8;
typedef unsigned char  u8;
typedef unsigned short u16;
typedef signed   short s16;
typedef unsigned long  u32;
typedef signed   long  s32;

#define WINVER 0x0500 

#define VC_EXTRALEAN				// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         		// MFC core and standard components
#include <afxext.h>         		// MFC extensions
#include <afxdtctl.h>				// MFC support for Internet Explorer 4 Common Controls
#include <afxdlgs.h>				// MFC common dialogs

#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>					// MFC support for Windows Common Controls
#endif								// _AFX_NO_AFXCMN_SUPPORT

//#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
//#endif


#endif