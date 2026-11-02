// ChatClient.cpp : Defines the class behaviors for the application.
//

#include "pch.h"
#include "framework.h"
#include "ChatClient.h"
#include "ChatClientDlg.h"
#include "CLoginDlg.h"
#include "ClientConnection.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CChatClientApp

BEGIN_MESSAGE_MAP(CChatClientApp, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()


// CChatClientApp construction

CChatClientApp::CChatClientApp()
{
	// support Restart Manager
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;
}


// The one and only CChatClientApp object

CChatClientApp theApp;


// CChatClientApp initialization

BOOL CChatClientApp::InitInstance()
{
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	AfxEnableControlContainer();

	CShellManager *pShellManager = new CShellManager;

	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	if (!AfxInitRichEdit2())
	{
		AfxMessageBox(_T("Cannot initialize Rich Edit."));
		return FALSE;
	}

	WSADATA wsaData = {};
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		AfxMessageBox(_T("Không thể khởi tạo Winsock 2.2."));
		return FALSE;
	}

	while (true)
	{
		MSG quitMsg = {};
		while (::PeekMessage(&quitMsg, nullptr, WM_QUIT, WM_QUIT, PM_REMOVE))
		{
		}

		m_pMainWnd = nullptr;
		CLoginDlg login;
		INT_PTR loginResult = login.DoModal();

		if (loginResult == -1)
		{
			AfxMessageBox(_T("Cannot open Login dialog."));
			break;
		}

		if (loginResult != IDOK)
		{
			break;
		}

		while (::PeekMessage(&quitMsg, nullptr, WM_QUIT, WM_QUIT, PM_REMOVE))
		{
		}

		CChatClientDlg chat;
		INT_PTR chatResult = chat.DoModal();

		if (chatResult != IDRETRY)
		{
			break;
		}
	}

	ClientConnection::Instance().Disconnect();
	WSACleanup();

	if (pShellManager != nullptr)
	{
		delete pShellManager;
	}

#if !defined(_AFXDLL) && !defined(_AFX_NO_MFC_CONTROLS_IN_DIALOGS)
	ControlBarCleanUp();
#endif

	return FALSE;
}