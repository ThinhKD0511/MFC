#include "pch.h"
#include "framework.h"
#include "ChatAppUsingIPC2.h"
#include "ChatAppUsingIPC2Dlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);   

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


// CChatAppUsingIPC2Dlg dialog



CChatAppUsingIPC2Dlg::CChatAppUsingIPC2Dlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CHATAPPUSINGIPC2_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CChatAppUsingIPC2Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_MESSAGE, m_editMessage);
	DDX_Control(pDX, IDC_COMBO_METHOD, m_comboMethod);
	DDX_Control(pDX, IDC_LIST_CHAT, m_listChat);
}

BEGIN_MESSAGE_MAP(CChatAppUsingIPC2Dlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_SEND, &CChatAppUsingIPC2Dlg::OnBnClickedBtnSend)
	ON_WM_COPYDATA()
	ON_MESSAGE(WM_APP + 1, &CChatAppUsingIPC2Dlg::OnPipeMessage)
	ON_WM_DESTROY()
END_MESSAGE_MAP()


BOOL CChatAppUsingIPC2Dlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetWindowText(_T("MFC Chat 2"));

	m_comboMethod.ResetContent();
	m_comboMethod.AddString(_T("Windows message"));
	m_comboMethod.AddString(_T("Named pipe"));
	m_comboMethod.SetCurSel(0);

	m_editMessage.SetWindowText(_T(""));

	m_listChat.ModifyStyle(
		LVS_TYPEMASK | LVS_NOCOLUMNHEADER,
		LVS_REPORT);

	m_listChat.SetExtendedStyle(
		m_listChat.GetExtendedStyle()
		| LVS_EX_FULLROWSELECT
		| LVS_EX_GRIDLINES);

	m_listChat.InsertColumn(
		0, _T("Nguoi gui"), LVCFMT_LEFT, 90);

	m_listChat.InsertColumn(
		1, _T("Noi dung"), LVCFMT_LEFT, 220);

	m_listChat.InsertColumn(
		2, _T("Co che"), LVCFMT_LEFT, 120);

	m_listChat.SetColumnWidth(2, LVSCW_AUTOSIZE_USEHEADER);

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

	SetIcon(m_hIcon, TRUE);			
	SetIcon(m_hIcon, FALSE);	
	
	if (!StartPipeReceiver())
	{
		AfxMessageBox(
			_T("Khong khoi dong duoc bo nhan Named pipe."));
	}

	return TRUE;  
}

void CChatAppUsingIPC2Dlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CChatAppUsingIPC2Dlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); 

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

HCURSOR CChatAppUsingIPC2Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CChatAppUsingIPC2Dlg::OnLvnItemchangedList2(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;
}


void CChatAppUsingIPC2Dlg::OnEnChangeEditMessage()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}


void CChatAppUsingIPC2Dlg::OnBnClickedBtnSend()
{
	CString text;
	m_editMessage.GetWindowText(text);

	if (text.IsEmpty())
		return;

	if (text.GetLength() > 1000)
	{
		AfxMessageBox(
			_T("Tin nhan toi da 1000 ky tu."));
		return;
	}

	int method = m_comboMethod.GetCurSel();

	BOOL sent = FALSE;
	CString methodName;

	if (method == 0)
	{
		methodName = _T("Windows message");
		sent = SendByWindowMessage(text);
	}
	else if (method == 1)
	{
		methodName = _T("Named pipe");
		sent = SendByNamedPipe(text);
	}
	else
	{
		return;
	}

	if (!sent)
	{
		AfxMessageBox(
			_T("Khong gui duoc. Kiem tra app ben kia da mo."));
		return;
	}

	AddChatLine(_T("App 2"), text, methodName);

	m_editMessage.SetWindowText(_T(""));
}

void CChatAppUsingIPC2Dlg::AddChatLine(
	const CString& sender,
	const CString& text,
	const CString& method)
{
	int row = m_listChat.InsertItem(
		m_listChat.GetItemCount(),
		sender);

	m_listChat.SetItemText(row, 1, text);
	m_listChat.SetItemText(row, 2, method);

	m_listChat.EnsureVisible(row, FALSE);
}

BOOL CChatAppUsingIPC2Dlg::SendByWindowMessage(
	const CString& text)
{
	HWND hOther = ::FindWindow(
		nullptr,
		_T("MFC Chat 1"));

	if (hOther == nullptr)
		return FALSE;

	COPYDATASTRUCT data = {};

	data.dwData = 1;

	data.cbData = static_cast<DWORD>(
		(text.GetLength() + 1) * sizeof(TCHAR));

	data.lpData = const_cast<LPTSTR>(
		text.GetString());

	LRESULT result = ::SendMessage(
		hOther,
		WM_COPYDATA,
		reinterpret_cast<WPARAM>(m_hWnd),
		reinterpret_cast<LPARAM>(&data));

	return result != FALSE;
}

BOOL CChatAppUsingIPC2Dlg::OnCopyData(
	CWnd* pWnd,
	COPYDATASTRUCT* pData)
{
	if (pData == nullptr ||
		pData->lpData == nullptr)
	{
		return FALSE;
	}

	if (pData->dwData != 1)
		return FALSE;

	if (pData->cbData < sizeof(TCHAR) ||
		pData->cbData > 1001 * sizeof(TCHAR) ||
		pData->cbData % sizeof(TCHAR) != 0)
	{
		return FALSE;
	}

	LPCTSTR buffer =
		static_cast<LPCTSTR>(pData->lpData);

	int count = static_cast<int>(
		pData->cbData / sizeof(TCHAR));

	if (buffer[count - 1] != _T('\0'))
		return FALSE;

	CString text(buffer, count - 1);

	AddChatLine(
		_T("App 1"),
		text,
		_T("Windows message"));

	return TRUE;
}

BOOL CChatAppUsingIPC2Dlg::StartPipeReceiver()
{
	m_pipe.window = m_hWnd;

	m_pipe.stopEvent = ::CreateEvent(
		nullptr, TRUE, FALSE, nullptr);

	if (m_pipe.stopEvent == nullptr)
		return FALSE;

	m_pipe.pipe = ::CreateNamedPipe(
		_T("\\\\.\\pipe\\MFCChat2"),
		PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE,
		PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
		1,
		4096,
		4096,
		0,
		nullptr);

	if (m_pipe.pipe == INVALID_HANDLE_VALUE)
	{
		StopPipeReceiver();
		return FALSE;
	}

	m_receiveThread = ::CreateThread(
		nullptr,
		0,
		&CChatAppUsingIPC2Dlg::PipeThreadProc,
		&m_pipe,
		0,
		nullptr);

	if (m_receiveThread == nullptr)
	{
		StopPipeReceiver();
		return FALSE;
	}

	return TRUE;
}

DWORD WINAPI CChatAppUsingIPC2Dlg::PipeThreadProc(
	LPVOID parameter)
{
	PipeContext* context =
		static_cast<PipeContext*>(parameter);

	while (::WaitForSingleObject(
		context->stopEvent, 0) == WAIT_TIMEOUT)
	{
		BOOL connected = ::ConnectNamedPipe(
			context->pipe, nullptr);

		if (!connected &&
			::GetLastError() != ERROR_PIPE_CONNECTED)
		{
			break;
		}

		if (::WaitForSingleObject(
			context->stopEvent, 0) != WAIT_TIMEOUT)
		{
			break;
		}

		TCHAR buffer[1001] = {};
		DWORD bytesRead = 0;

		BOOL readOK = ::ReadFile(
			context->pipe,
			buffer,
			sizeof(buffer),
			&bytesRead,
			nullptr);

		BYTE accepted = 0;

		if (readOK &&
			bytesRead >= sizeof(TCHAR) &&
			bytesRead % sizeof(TCHAR) == 0)
		{
			DWORD count = bytesRead / sizeof(TCHAR);

			if (buffer[count - 1] == _T('\0'))
			{
				LPTSTR copy = static_cast<LPTSTR>(
					::HeapAlloc(
						::GetProcessHeap(),
						0,
						bytesRead));

				if (copy != nullptr)
				{
					::CopyMemory(copy, buffer, bytesRead);

					BOOL posted = ::PostMessage(
						context->window,
						WM_APP + 1,
						0,
						reinterpret_cast<LPARAM>(copy));

					if (!posted)
					{
						::HeapFree(
							::GetProcessHeap(), 0, copy);
					}
				}
			}
		}

		::DisconnectNamedPipe(context->pipe);
	}

	return 0;
}

LRESULT CChatAppUsingIPC2Dlg::OnPipeMessage(
	WPARAM wParam, LPARAM lParam)
{
	LPTSTR buffer = reinterpret_cast<LPTSTR>(lParam);

	if (buffer == nullptr)
		return 0;

	CString text(buffer);

	::HeapFree(::GetProcessHeap(), 0, buffer);

	AddChatLine(
		_T("App 1"),
		text,
		_T("Named pipe"));

	return 0;
}

BOOL CChatAppUsingIPC2Dlg::SendByNamedPipe(
	const CString& text)
{
	LPCTSTR pipeName = _T("\\\\.\\pipe\\MFCChat1");

	HANDLE hPipe = ::CreateFile(
		pipeName,
		GENERIC_READ | GENERIC_WRITE,
		0,
		nullptr,
		OPEN_EXISTING,
		0,
		nullptr);

	if (hPipe == INVALID_HANDLE_VALUE)
		return FALSE;

	DWORD size = static_cast<DWORD>(
		(text.GetLength() + 1) * sizeof(TCHAR));

	DWORD bytesWritten = 0;

	BOOL sent = ::WriteFile(
		hPipe,
		text.GetString(),
		size,
		&bytesWritten,
		nullptr);

	::CloseHandle(hPipe);

	return sent && bytesWritten == size;
}

void CChatAppUsingIPC2Dlg::StopPipeReceiver()
{
	if (m_receiveThread != nullptr)
	{
		::SetEvent(m_pipe.stopEvent);

		while (::WaitForSingleObject(
			m_receiveThread, 20) == WAIT_TIMEOUT)
		{
			::CancelSynchronousIo(m_receiveThread);
		}

		::CloseHandle(m_receiveThread);
		m_receiveThread = nullptr;
	}

	if (m_pipe.pipe != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(m_pipe.pipe);
		m_pipe.pipe = INVALID_HANDLE_VALUE;
	}

	if (m_pipe.stopEvent != nullptr)
	{
		::CloseHandle(m_pipe.stopEvent);
		m_pipe.stopEvent = nullptr;
	}

	MSG message = {};

	while (::PeekMessage(
		&message,
		m_hWnd,
		WM_APP + 1,
		WM_APP + 1,
		PM_REMOVE))
	{
		if (message.message == WM_QUIT)
		{
			::PostQuitMessage(
				static_cast<int>(message.wParam));
			break;
		}

		::HeapFree(
			::GetProcessHeap(),
			0,
			reinterpret_cast<LPVOID>(message.lParam));
	}
}

void CChatAppUsingIPC2Dlg::OnDestroy()
{
	StopPipeReceiver();

	CDialogEx::OnDestroy();
}
