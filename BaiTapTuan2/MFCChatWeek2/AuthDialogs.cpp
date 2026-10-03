#include "AuthDialogs.h"
#include "WinHelpers.h"

BEGIN_MESSAGE_MAP(CLoginDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_OPEN_REGISTER, &CLoginDlg::OnOpenRegister)
END_MESSAGE_MAP()

BOOL CLoginDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    CenterWindow(GetParent());
    Win::Limit(m_hWnd, IDC_EDIT_USERNAME, 32);
    Win::Limit(m_hWnd, IDC_EDIT_PASSWORD, 64);
    return TRUE;
}

void CLoginDlg::OnOK()
{
    const auto user = Win::Trim(Win::Text(m_hWnd, IDC_EDIT_USERNAME));
    const auto password = Win::Text(m_hWnd, IDC_EDIT_PASSWORD);
    if (!m_data.CheckLogin(user, password))
    {
        Win::Notice(m_hWnd, L"Wrong username or password. Try admin / 123.");
        return;
    }
    m_user = user;
    if (AfxGetApp()->m_pMainWnd == this)
        AfxGetApp()->m_pMainWnd = nullptr;
    EndDialog(IDOK);
}

void CLoginDlg::OnOpenRegister()
{
    CRegisterDlg dialog(m_data, this);
    if (dialog.DoModal() == IDOK)
    {
        ::SetDlgItemTextW(m_hWnd, IDC_EDIT_USERNAME, dialog.RegisteredUser().c_str());
        ::SetDlgItemTextW(m_hWnd, IDC_EDIT_PASSWORD, L"");
        ::SetFocus(::GetDlgItem(m_hWnd, IDC_EDIT_PASSWORD));
    }
}

BOOL CRegisterDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    CenterWindow(GetParent());
    Win::Limit(m_hWnd, IDC_EDIT_USERNAME, 32);
    Win::Limit(m_hWnd, IDC_EDIT_PASSWORD, 64);
    Win::Limit(m_hWnd, IDC_EDIT_CONFIRM, 64);
    return TRUE;
}

void CRegisterDlg::OnOK()
{
    const auto user = Win::Trim(Win::Text(m_hWnd, IDC_EDIT_USERNAME));
    const auto password = Win::Text(m_hWnd, IDC_EDIT_PASSWORD);
    const auto confirm = Win::Text(m_hWnd, IDC_EDIT_CONFIRM);
    if (!FakeData::ValidUsername(user))
    {
        Win::Notice(m_hWnd, L"Username must contain 3-32 letters, digits or underscore.");
        return;
    }
    if (password.size() < 3 || password.size() > 64)
    {
        Win::Notice(m_hWnd, L"Demo password must contain 3-64 characters.");
        return;
    }
    if (password != confirm)
    {
        Win::Notice(m_hWnd, L"The passwords do not match.");
        return;
    }
    if (!m_data.RegisterAccount(user, password))
    {
        Win::Notice(m_hWnd, L"This username already exists (case-sensitive).");
        return;
    }
    m_user = user;
    Win::Notice(m_hWnd, L"Account created. Please log in.");
    EndDialog(IDOK);
}

