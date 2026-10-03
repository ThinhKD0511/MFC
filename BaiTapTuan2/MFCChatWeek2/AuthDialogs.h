#pragma once
#include "framework.h"
#include "FakeData.h"

class CRegisterDlg : public CDialogEx
{
public:
    CRegisterDlg(FakeData& data, CWnd* parent) : CDialogEx(IDD_REGISTER, parent), m_data(data) {}
    std::wstring RegisteredUser() const { return m_user; }
protected:
    BOOL OnInitDialog() override;
    void OnOK() override;
private:
    FakeData& m_data;
    std::wstring m_user;
};

class CLoginDlg : public CDialogEx
{
public:
    CLoginDlg(FakeData& data, CWnd* parent) : CDialogEx(IDD_LOGIN, parent), m_data(data) {}
    std::wstring LoggedUser() const { return m_user; }
protected:
    BOOL OnInitDialog() override;
    void OnOK() override;
    afx_msg void OnOpenRegister();
    DECLARE_MESSAGE_MAP()
private:
    FakeData& m_data;
    std::wstring m_user;
};

