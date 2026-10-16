
// ChatAppUsingIPC2Dlg.h : header file
//

#pragma once


// CChatAppUsingIPC2Dlg dialog
class CChatAppUsingIPC2Dlg : public CDialogEx
{
// Construction
public:
	CChatAppUsingIPC2Dlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CHATAPPUSINGIPC2_DIALOG };
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
	void AddChatLine(
		const CString& sender,
		const CString& text,
		const CString& method);

	BOOL SendByWindowMessage(const CString& text);
	BOOL SendByNamedPipe(const CString& text);

	afx_msg BOOL OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pData);
	afx_msg LRESULT OnPipeMessage(WPARAM wParam, LPARAM lParam);
	afx_msg void OnDestroy();

	struct PipeContext
	{
		HWND window = nullptr;
		HANDLE pipe = INVALID_HANDLE_VALUE;
		HANDLE stopEvent = nullptr;
	};

	PipeContext m_pipe;
	HANDLE m_receiveThread = nullptr;

	BOOL StartPipeReceiver();
	void StopPipeReceiver();

	static DWORD WINAPI PipeThreadProc(LPVOID parameter);
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnLvnItemchangedList2(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEnChangeEditMessage();
	CEdit m_editMessage;
	CComboBox m_comboMethod;
	CListCtrl m_listChat;
	afx_msg void OnBnClickedBtnSend();
};
