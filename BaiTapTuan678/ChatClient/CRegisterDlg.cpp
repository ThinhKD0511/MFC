// CRegisterDlg.cpp : implementation file
//

#include "pch.h"
#include "ChatClient.h"
#include "CRegisterDlg.h"
#include "afxdialogex.h"
#include "ClientConnection.h"


// CRegisterDlg dialog

IMPLEMENT_DYNAMIC(CRegisterDlg, CDialogEx)

CRegisterDlg::CRegisterDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REGISTER_DIALOG, pParent)
{

}

CRegisterDlg::~CRegisterDlg()
{
}

void CRegisterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

void CRegisterDlg::OnOK()
{
	OnBnClickedButtonRegister();
}


BEGIN_MESSAGE_MAP(CRegisterDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BUTTON_REGISTER, &CRegisterDlg::OnBnClickedButtonRegister)
	ON_BN_CLICKED(IDCANCEL, &CRegisterDlg::OnBnClickedCancel)
END_MESSAGE_MAP()


// CRegisterDlg message handlers


void CRegisterDlg::OnBnClickedButtonRegister()
{
	CString username;
	CString password;
	CString confirmPassword;

	GetDlgItemText(IDC_EDIT_USERNAME_REGISTER, username);
	GetDlgItemText(IDC_EDIT_REGISTER_PASSWORD, password);
	GetDlgItemText(IDC_EDIT_REGISTER_CONFIRM, confirmPassword);

	username.Trim();

	if (username.IsEmpty() || password.IsEmpty() || confirmPassword.IsEmpty())
	{
		MessageBox(
			_T("Vui lòng nhập đầy đủ Tên đăng nhập, Mật khẩu và Nhập lại mật khẩu."),
			_T("Đăng ký"),
			MB_OK | MB_ICONWARNING);
		return;
	}

	if (password != confirmPassword)
	{
		MessageBox(
			_T("Mật khẩu nhập lại không khớp. Vui lòng kiểm tra lại."),
			_T("Đăng ký"),
			MB_OK | MB_ICONWARNING);
		return;
	}

	CString errorMsg;
	if (!ClientConnection::Instance().Register(username, password, errorMsg))
	{
		MessageBox(errorMsg, _T("Đăng ký thất bại"), MB_OK | MB_ICONERROR);
		return;
	}

	m_registeredUsername = username;
	MessageBox(
		_T("Đăng ký tài khoản thành công! Bạn có thể đăng nhập ngay bây giờ."),
		_T("Thành công"),
		MB_OK | MB_ICONINFORMATION);

	EndDialog(IDOK);
}


void CRegisterDlg::OnBnClickedCancel()
{
	CDialogEx::OnCancel();
}
