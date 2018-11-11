// nwmp3play_vc.h : main header file for the NWMP3PLAY_VC application
//

#if !defined(AFX_NWMP3PLAY_VC_H__DD5DAB24_C354_437C_9409_2D4DDBA8871F__INCLUDED_)
#define AFX_NWMP3PLAY_VC_H__DD5DAB24_C354_437C_9409_2D4DDBA8871F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CNwmp3play_vcApp:
// See nwmp3play_vc.cpp for the implementation of this class
//

class CNwmp3play_vcApp : public CWinApp
{
public:
	CNwmp3play_vcApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNwmp3play_vcApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CNwmp3play_vcApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NWMP3PLAY_VC_H__DD5DAB24_C354_437C_9409_2D4DDBA8871F__INCLUDED_)
