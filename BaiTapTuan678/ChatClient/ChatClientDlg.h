// ChatClientDlg.h : header file
//

#pragma once

#include <vector>
#include "ClientConnection.h"


// CChatClientDlg dialog
class CChatClientDlg : public CDialogEx
{
// Construction
public:
	CChatClientDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CHATCLIENT_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	static constexpr UINT_PTR TIMER_POLL_ID = 1001;

	std::vector<ClientUserItem> m_users;
	long long m_selectedPeerId = 0;
	CString m_selectedPeerName;
	long long m_lastMessageId = 0;
	int m_timerTickCount = 0;
	bool m_updatingUserList = false;

	void RefreshUserList();
	void RefreshMessages(bool fullReload);
	void AppendMessageToView(const ClientMessageItem& msg);
	static CString FormatTimestamp(const CString& isoTimestamp);

	void OnOK() override;
	void OnCancel() override;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnDestroy();
	afx_msg void OnLvnItemchangedListFriends(NMHDR* pNMHDR, LRESULT* pResult);
	DECLARE_MESSAGE_MAP()
public:
	CListCtrl m_listFriends;
	CRichEditCtrl m_richChat;
	CEdit m_editMessage;
	afx_msg void OnBnClickedButtonLogout();
	afx_msg void OnEnChangeEditMessage();
	afx_msg void OnBnClickedButtonSend();
};
