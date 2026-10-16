
// TCPServerDlg.h : header file
//

#pragma once
#define WM_SERVER_SOCKET (WM_APP + 1)
#define WM_SERVER_TEXT (WM_APP + 2)


// CTCPServerDlg dialog
class CTCPServerDlg : public CDialogEx
{
// Construction
public:
	CTCPServerDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_TCPSERVER_DIALOG };
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
	afx_msg void OnBnClickedBtnSend();
	afx_msg void OnBnClickedBtnStart();

private:
	SOCKET m_listenSocket = INVALID_SOCKET;
	bool m_winsockStarted = false;

	void AddLog(const CString& text);

protected:
	afx_msg void OnDestroy();

private:
	SOCKET m_clientSocket = INVALID_SOCKET;

protected:
	afx_msg LRESULT OnServerSocket(WPARAM wParam, LPARAM lParam);

private:
	CWinThread* m_receiveThread = nullptr;
	static UINT ReceiveThread(LPVOID parameter);

protected:
	afx_msg LRESULT OnServerText(WPARAM wParam, LPARAM lParam);
};
