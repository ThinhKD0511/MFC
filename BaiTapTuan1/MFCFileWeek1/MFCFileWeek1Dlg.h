#pragma once
#include <afxwin.h>
#include <afxdialogex.h>
#include "resource.h"

class CMFCFileWeek1Dlg : public CDialogEx
{
public:
    enum { IDD = IDD_MFCFILEWEEK1_DIALOG };
    CMFCFileWeek1Dlg(CWnd* parent = nullptr);

protected:
    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;
    void OnOK() override; // Tranh dong cua so khi nhan Enter.

    afx_msg void OnChooseFolder();
    afx_msg void OnSave();
    afx_msg void OnLoad();
    DECLARE_MESSAGE_MAP()

private:
    CString m_content;
    CString m_filePath;
};
