#include "MFCFileWeek1.h"
#include "MFCFileWeek1Dlg.h"
#include <afxole.h>
#include <commctrl.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "ole32.lib")

CMFCFileWeek1App theApp;

BOOL CMFCFileWeek1App::InitInstance()
{
    INITCOMMONCONTROLSEX controls = { sizeof(controls), ICC_WIN95_CLASSES };
    InitCommonControlsEx(&controls);
    CWinApp::InitInstance();
    if (!AfxOleInit())
    {
        AfxMessageBox(L"Khong khoi tao duoc hop thoai chon folder.");
        return FALSE;
    }

    CMFCFileWeek1Dlg dlg;
    m_pMainWnd = &dlg;
    dlg.DoModal();
    m_pMainWnd = nullptr;
    return FALSE;
}
