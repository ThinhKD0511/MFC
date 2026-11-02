// CLoginDlg.cpp : implementation file
//

#include "pch.h"
#include "ChatClient.h"
#include "CLoginDlg.h"
#include "afxdialogex.h"
#include "CRegisterDlg.h"
#include "ClientConnection.h"


// CLoginDlg dialog

IMPLEMENT_DYNAMIC(CLoginDlg, CDialogEx)

CLoginDlg::CLoginDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_LOGIN_DIALOG, pParent)
{

}

CLoginDlg::~CLoginDlg()
{
}

void CLoginDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

void CLoginDlg::OnOK()
{
	OnBnClickedButtonLogin();
}


BEGIN_MESSAGE_MAP(CLoginDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_LOGIN, &CLoginDlg::OnBnClickedButtonLogin)
	ON_BN_CLICKED(IDC_BUTTON_REGISTER, &CLoginDlg::OnBnClickedButtonRegister)
	ON_BN_CLICKED(IDCANCEL, &CLoginDlg::OnBnClickedCancel)
END_MESSAGE_MAP()


// CLoginDlg message handlers


void CLoginDlg::OnBnClickedButtonLogin()
{
	CString username;
	CString password;

	GetDlgItemText(IDC_EDIT_USERNAME_LOGIN, username);
	GetDlgItemText(IDC_EDIT_PASSWORD_LOGIN, password);

	username.Trim();

	if (username.IsEmpty() || password.IsEmpty())
	{
		MessageBox(
			_T("Vui lòng nhập Tên đăng nhập và Mật khẩu."),
			_T("Đăng nhập"),
			MB_OK | MB_ICONWARNING);
		return;
	}

	CString errorMsg;
	if (!ClientConnection::Instance().Login(username, password, errorMsg))
	{
		MessageBox(errorMsg, _T("Đăng nhập thất bại"), MB_OK | MB_ICONERROR);
		return;
	}

	EndDialog(IDOK);
}


void CLoginDlg::OnBnClickedButtonRegister()
{
	CRegisterDlg registerDlg(this);
	if (registerDlg.DoModal() == IDOK && !registerDlg.m_registeredUsername.IsEmpty())
	{
		SetDlgItemText(IDC_EDIT_USERNAME_LOGIN, registerDlg.m_registeredUsername);
		SetDlgItemText(IDC_EDIT_PASSWORD_LOGIN, _T(""));
		if (CWnd* pPwd = GetDlgItem(IDC_EDIT_PASSWORD_LOGIN))
			pPwd->SetFocus();
	}
}


void CLoginDlg::OnBnClickedCancel()
{
	CDialogEx::OnCancel();
}
