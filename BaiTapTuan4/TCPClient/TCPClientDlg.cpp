
// TCPClientDlg.cpp : implementation file
//
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "pch.h"
#include "framework.h"
#include "TCPClient.h"
#include "TCPClientDlg.h"
#include "afxdialogex.h"
#include <atlconv.h>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define DEFAULT_PORT "27015"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CTCPClientDlg dialog



CTCPClientDlg::CTCPClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_TCPCLIENT_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTCPClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CTCPClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_START, &CTCPClientDlg::OnBnClickedBtnStart)
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BTN_SEND, &CTCPClientDlg::OnBnClickedBtnSend)
	ON_MESSAGE(WM_CLIENT_TEXT, &CTCPClientDlg::OnClientText)
END_MESSAGE_MAP()


// CTCPClientDlg message handlers

BOOL CTCPClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CTCPClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CTCPClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CTCPClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CTCPClientDlg::OnBnClickedBtnStart()
{
	if (m_connectSocket != INVALID_SOCKET)
		return;

	WSADATA wsaData = {};
	addrinfo hints = {};
	addrinfo* address = nullptr;
	int errorCode = 0;

	do
	{
		errorCode = WSAStartup(MAKEWORD(2, 2), &wsaData);

		if (errorCode != 0)
			break;

		m_winsockStarted = true;

		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;

		errorCode = getaddrinfo(
			SERVER_IP, DEFAULT_PORT, &hints, &address);

		if (errorCode != 0)
			break;

		// Tao socket Client.
		m_connectSocket = socket(
			address->ai_family,
			address->ai_socktype,
			address->ai_protocol);

		if (m_connectSocket == INVALID_SOCKET)
		{
			errorCode = WSAGetLastError();
			break;
		}

		// Ket noi den Server.
		if (connect(m_connectSocket, address->ai_addr,
			static_cast<int>(address->ai_addrlen))
			== SOCKET_ERROR)
		{
			errorCode = WSAGetLastError();
			break;
		}
	} while (false);

	if (address != nullptr)
		freeaddrinfo(address);

	// Don dep chung neu ket noi that bai.
	if (errorCode != 0)
	{
		if (m_connectSocket != INVALID_SOCKET)
		{
			closesocket(m_connectSocket);
			m_connectSocket = INVALID_SOCKET;
		}

		if (m_winsockStarted)
		{
			WSACleanup();
			m_winsockStarted = false;
		}

		CString text;
		text.Format(_T("Connect failed. Error: %d"), errorCode);
		AfxMessageBox(text);
		return;
	}

	AddLog(_T("Connected to Server at 127.0.0.1:27015."));

	m_receiveThread = AfxBeginThread(
		ReceiveThread, this,
		THREAD_PRIORITY_NORMAL, 0, CREATE_SUSPENDED);

	if (m_receiveThread == nullptr)
	{
		closesocket(m_connectSocket);
		m_connectSocket = INVALID_SOCKET;
		AddLog(_T("Cannot start receive thread."));
		return;
	}

	m_receiveThread->m_bAutoDelete = FALSE;
	m_receiveThread->ResumeThread();

	GetDlgItem(IDC_BTN_START)->EnableWindow(FALSE);
}

void CTCPClientDlg::AddLog(const CString& text)
{
	CEdit* history =
		static_cast<CEdit*>(GetDlgItem(IDC_EDIT_HISTORY));

	history->SetSel(-1, -1);
	history->ReplaceSel(text + _T("\r\n"));
}

void CTCPClientDlg::OnDestroy()
{
	if (m_connectSocket != INVALID_SOCKET)
		shutdown(m_connectSocket, SD_BOTH);

	if (m_receiveThread != nullptr)
	{
		WaitForSingleObject(m_receiveThread->m_hThread, INFINITE);

		delete m_receiveThread;
		m_receiveThread = nullptr;
	}

	if (m_connectSocket != INVALID_SOCKET)
	{
		closesocket(m_connectSocket);
		m_connectSocket = INVALID_SOCKET;
	}

	MSG message = {};

	while (::PeekMessage(&message, m_hWnd,
		WM_CLIENT_TEXT, WM_CLIENT_TEXT, PM_REMOVE))
	{
		delete reinterpret_cast<CString*>(message.lParam);
	}

	if (m_winsockStarted)
	{
		WSACleanup();
		m_winsockStarted = false;
	}

	CDialogEx::OnDestroy();
}

void CTCPClientDlg::OnBnClickedBtnSend()
{
	if (m_connectSocket == INVALID_SOCKET)
	{
		AfxMessageBox(_T("Please connect to Server first."));
		return;
	}

	CString text;
	GetDlgItemText(IDC_EDIT_MESSAGE, text);

	if (text.IsEmpty())
		return;

	CStringA data(CT2A(text.GetString(), CP_UTF8));

	const char* buffer = data.GetString();
	int total = data.GetLength() + 1;
	int sent = 0;

	while (sent < total)
	{
		int result = send(
			m_connectSocket,
			buffer + sent,
			total - sent,
			0);

		if (result <= 0)
		{
			CString error;

			if (result == SOCKET_ERROR)
				error.Format(_T("Send failed. Error: %d"),
					WSAGetLastError());
			else
				error = _T("Send failed.");

			shutdown(m_connectSocket, SD_BOTH);
			GetDlgItem(IDC_BTN_SEND)->EnableWindow(FALSE);

			AfxMessageBox(error);
			return;
		}

		sent += result;
	}

	AddLog(CString(_T("Me: ")) + text);
	SetDlgItemText(IDC_EDIT_MESSAGE, _T(""));
}

UINT CTCPClientDlg::ReceiveThread(LPVOID parameter)
{
	CTCPClientDlg* dialog =
		static_cast<CTCPClientDlg*>(parameter);

	SOCKET client = dialog->m_connectSocket;
	HWND window = dialog->m_hWnd;

	char buffer[512];
	std::string pending;

	while (true)
	{
		int received = recv(client, buffer, sizeof(buffer), 0);

		if (received <= 0)
			break;

		pending.append(buffer, received);

		size_t position;

		while ((position = pending.find('\0')) != std::string::npos)
		{
			std::string message = pending.substr(0, position);
			pending.erase(0, position + 1);

			CString* text =
				new CString(CA2T(message.c_str(), CP_UTF8));

			if (!::PostMessage(window, WM_CLIENT_TEXT, 0,
				reinterpret_cast<LPARAM>(text)))
			{
				delete text;
				return 0;
			}
		}
	}

	CString* text = new CString(_T("Server disconnected or receive failed."));

	if (!::PostMessage(window, WM_CLIENT_TEXT, 1,
		reinterpret_cast<LPARAM>(text)))
	{
		delete text;
	}

	return 0;
}

LRESULT CTCPClientDlg::OnClientText(WPARAM wParam, LPARAM lParam)
{
	CString* text = reinterpret_cast<CString*>(lParam);

	if (text == nullptr)
		return 0;

	if (wParam == 0)
		AddLog(CString(_T("Server: ")) + *text);
	else
		AddLog(*text);

	delete text;

	return 0;
}
