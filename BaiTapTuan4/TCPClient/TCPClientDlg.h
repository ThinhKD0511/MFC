
// TCPClientDlg.h : header file
//

#pragma once
#define WM_CLIENT_TEXT (WM_APP + 2)


// CTCPClientDlg dialog
class CTCPClientDlg : public CDialogEx
{
// Construction
public:
	CTCPClientDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_TCPCLIENT_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnStart();

private:
	SOCKET m_connectSocket = INVALID_SOCKET;
	bool m_winsockStarted = false;

	void AddLog(const CString& text);

protected:
	afx_msg void OnDestroy();
public:
	afx_msg void OnBnClickedBtnSend();

private:
	CWinThread* m_receiveThread = nullptr;
	static UINT ReceiveThread(LPVOID parameter);

protected:
	afx_msg LRESULT OnClientText(WPARAM wParam, LPARAM lParam);
};
