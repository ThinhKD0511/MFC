// ChatClientDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "ChatClient.h"
#include "ChatClientDlg.h"
#include "afxdialogex.h"
#include "CLoginDlg.h"
#include <algorithm>

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


// CChatClientDlg dialog



CChatClientDlg::CChatClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CHATCLIENT_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CChatClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_FRIENDS, m_listFriends);
	DDX_Control(pDX, IDC_RICHEDIT_CHAT, m_richChat);
	DDX_Control(pDX, IDC_EDIT_MESSAGE, m_editMessage);
}

BEGIN_MESSAGE_MAP(CChatClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_FRIENDS, &CChatClientDlg::OnLvnItemchangedListFriends)
	ON_BN_CLICKED(IDC_BUTTON_LOGOUT, &CChatClientDlg::OnBnClickedButtonLogout)
	ON_EN_CHANGE(IDC_EDIT_MESSAGE, &CChatClientDlg::OnEnChangeEditMessage)
	ON_BN_CLICKED(IDC_BUTTON_SEND, &CChatClientDlg::OnBnClickedButtonSend)
END_MESSAGE_MAP()


// CChatClientDlg message handlers

BOOL CChatClientDlg::OnInitDialog()
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

	// Ẩn nút Button2 thừa ở góc (0, 0) trên resource
	if (CWnd* pBtn2 = GetDlgItem(IDC_BUTTON2))
	{
		pBtn2->ShowWindow(SW_HIDE);
	}

	CString windowTitle;
	windowTitle.Format(
		_T("ChatClient - Tài khoản: %s"),
		ClientConnection::Instance().GetCurrentUsername().GetString());
	SetWindowText(windowTitle);

	m_listFriends.ModifyStyle(
		LVS_TYPEMASK,
		LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS);
	m_listFriends.SetExtendedStyle(
		m_listFriends.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	m_listFriends.InsertColumn(0, _T("Người dùng"), LVCFMT_LEFT, 95);
	m_listFriends.InsertColumn(1, _T("Trạng thái"), LVCFMT_LEFT, 60);

	m_richChat.SetWindowText(_T("Hãy chọn một người dùng ở danh sách bên trái để bắt đầu trò chuyện.\r\n"));

	RefreshUserList();

	SetTimer(TIMER_POLL_ID, 1000, nullptr);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CChatClientDlg::OnDestroy()
{
	KillTimer(TIMER_POLL_ID);
	CDialogEx::OnDestroy();
}

void CChatClientDlg::OnOK()
{
	OnBnClickedButtonSend();
}

void CChatClientDlg::OnCancel()
{
	KillTimer(TIMER_POLL_ID);
	CString dummyError;
	ClientConnection::Instance().Logout(dummyError);
	ClientConnection::Instance().Disconnect();
	CDialogEx::OnCancel();
}

void CChatClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CChatClientDlg::OnPaint()
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

HCURSOR CChatClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

CString CChatClientDlg::FormatTimestamp(const CString& isoTimestamp)
{
	int tPos = isoTimestamp.Find(_T('T'));
	if (tPos > 0)
	{
		CString datePart = isoTimestamp.Left(tPos);
		CString timePart = isoTimestamp.Mid(tPos + 1);
		int dotPos = timePart.Find(_T('.'));
		if (dotPos > 0)
			timePart = timePart.Left(dotPos);
		else
			timePart.TrimRight(_T('Z'));

		return datePart + _T(" ") + timePart;
	}
	return isoTimestamp;
}

void CChatClientDlg::AppendMessageToView(const ClientMessageItem& msg)
{
	long long myUserId = ClientConnection::Instance().GetCurrentUserId();
	CString senderLabel = (msg.senderId == myUserId)
		? _T("Bạn")
		: m_selectedPeerName;

	CString timeLabel = FormatTimestamp(msg.createdAt);
	CString formattedLine;

	if (!timeLabel.IsEmpty())
	{
		formattedLine.Format(
			_T("[%s] %s: %s\r\n\r\n"),
			timeLabel.GetString(),
			senderLabel.GetString(),
			msg.content.GetString());
	}
	else
	{
		formattedLine.Format(
			_T("%s: %s\r\n\r\n"),
			senderLabel.GetString(),
			msg.content.GetString());
	}

	int textLength = m_richChat.GetWindowTextLength();
	m_richChat.SetSel(textLength, textLength);
	m_richChat.ReplaceSel(formattedLine);
	m_richChat.PostMessage(WM_VSCROLL, SB_BOTTOM, 0);
}

void CChatClientDlg::RefreshUserList()
{
	std::vector<ClientUserItem> latestUsers;
	CString errorMsg;

	if (!ClientConnection::Instance().GetUsers(latestUsers, errorMsg))
	{
		return;
	}

	m_updatingUserList = true;
	m_users = latestUsers;

	int existingCount = m_listFriends.GetItemCount();
	int newCount = static_cast<int>(m_users.size());

	for (int i = 0; i < newCount; ++i)
	{
		const ClientUserItem& u = m_users[i];
		CString statusText = u.online ? _T("Online") : _T("Offline");

		if (i < existingCount)
		{
			if (m_listFriends.GetItemText(i, 0) != u.username)
				m_listFriends.SetItemText(i, 0, u.username);

			if (m_listFriends.GetItemText(i, 1) != statusText)
				m_listFriends.SetItemText(i, 1, statusText);
		}
		else
		{
			m_listFriends.InsertItem(i, u.username);
			m_listFriends.SetItemText(i, 1, statusText);
		}

		m_listFriends.SetItemData(i, static_cast<DWORD_PTR>(u.userId));

		if (u.userId == m_selectedPeerId)
		{
			m_listFriends.SetItemState(
				i,
				LVIS_SELECTED | LVIS_FOCUSED,
				LVIS_SELECTED | LVIS_FOCUSED);
		}
	}

	while (m_listFriends.GetItemCount() > newCount)
	{
		m_listFriends.DeleteItem(m_listFriends.GetItemCount() - 1);
	}

	if (m_selectedPeerId == 0 && !m_users.empty())
	{
		m_selectedPeerId = m_users[0].userId;
		m_selectedPeerName = m_users[0].username;
		m_listFriends.SetItemState(
			0,
			LVIS_SELECTED | LVIS_FOCUSED,
			LVIS_SELECTED | LVIS_FOCUSED);
		m_updatingUserList = false;
		RefreshMessages(true);
		return;
	}

	m_updatingUserList = false;
}

void CChatClientDlg::RefreshMessages(bool fullReload)
{
	if (m_selectedPeerId <= 0)
		return;

	std::vector<ClientMessageItem> messages;
	CString errorMsg;

	if (!ClientConnection::Instance().GetMessages(
		m_selectedPeerId,
		0,
		100,
		messages,
		errorMsg))
	{
		return;
	}

	// Server trả về tin nhắn theo thứ tự id giảm dần (mới nhất trước),
	// nên cần đảo ngược lại để hiển thị từ cũ đến mới.
	std::reverse(messages.begin(), messages.end());

	if (fullReload)
	{
		m_richChat.SetWindowText(_T(""));
		m_lastMessageId = 0;

		if (messages.empty())
		{
			CString emptyNotice;
			emptyNotice.Format(
				_T("Chưa có tin nhắn nào với %s. Hãy gửi lời chào đầu tiên!\r\n\r\n"),
				m_selectedPeerName.GetString());
			m_richChat.SetWindowText(emptyNotice);
			return;
		}

		for (const auto& msg : messages)
		{
			AppendMessageToView(msg);
			if (msg.id > m_lastMessageId)
				m_lastMessageId = msg.id;
		}
	}
	else
	{
		for (const auto& msg : messages)
		{
			if (msg.id > m_lastMessageId)
			{
				if (m_lastMessageId == 0)
				{
					m_richChat.SetWindowText(_T(""));
				}
				AppendMessageToView(msg);
				m_lastMessageId = msg.id;
			}
		}
	}
}

void CChatClientDlg::OnLvnItemchangedListFriends(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;

	if (m_updatingUserList)
		return;

	if ((pNMLV->uChanged & LVIF_STATE) &&
		(pNMLV->uNewState & LVIS_SELECTED))
	{
		int selectedIndex = pNMLV->iItem;
		if (selectedIndex >= 0 && selectedIndex < m_listFriends.GetItemCount())
		{
			long long peerId = static_cast<long long>(
				m_listFriends.GetItemData(selectedIndex));
			CString peerName = m_listFriends.GetItemText(selectedIndex, 0);

			if (peerId > 0 && peerId != m_selectedPeerId)
			{
				m_selectedPeerId = peerId;
				m_selectedPeerName = peerName;
				m_lastMessageId = 0;
				RefreshMessages(true);
			}
		}
	}
}

void CChatClientDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == TIMER_POLL_ID)
	{
		++m_timerTickCount;

		if (m_selectedPeerId > 0)
		{
			RefreshMessages(false);
		}

		if (m_timerTickCount % 3 == 0)
		{
			RefreshUserList();
		}
	}

	CDialogEx::OnTimer(nIDEvent);
}

void CChatClientDlg::OnBnClickedButtonLogout()
{
	KillTimer(TIMER_POLL_ID);

	CString errorMsg;
	ClientConnection::Instance().Logout(errorMsg);

	EndDialog(IDRETRY);
}

void CChatClientDlg::OnEnChangeEditMessage()
{
}

void CChatClientDlg::OnBnClickedButtonSend()
{
	if (m_selectedPeerId <= 0)
	{
		MessageBox(
			_T("Vui lòng chọn một người dùng ở danh sách bên trái để gửi tin nhắn."),
			_T("Chưa chọn người nhận"),
			MB_OK | MB_ICONWARNING);
		return;
	}

	CString strMessage;
	m_editMessage.GetWindowText(strMessage);

	CString trimmed = strMessage;
	trimmed.Trim();
	if (trimmed.IsEmpty())
		return;

	ClientMessageItem sentMsg;
	CString errorMsg;

	if (!ClientConnection::Instance().SendChatMessage(
		m_selectedPeerId,
		trimmed,
		sentMsg,
		errorMsg))
	{
		MessageBox(errorMsg, _T("Gửi tin nhắn thất bại"), MB_OK | MB_ICONERROR);
		return;
	}

	if (m_lastMessageId == 0)
	{
		m_richChat.SetWindowText(_T(""));
	}

	AppendMessageToView(sentMsg);
	if (sentMsg.id > m_lastMessageId)
	{
		m_lastMessageId = sentMsg.id;
	}

	m_editMessage.SetWindowText(_T(""));
	m_editMessage.SetFocus();
}