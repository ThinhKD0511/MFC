
// TCPServer.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CTCPServerApp:
// See TCPServer.cpp for the implementation of this class
//

class CTCPServerApp : public CWinApp
{
public:
	CTCPServerApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CTCPServerApp theApp;
