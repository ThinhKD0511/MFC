#include "framework.h"
#include "AuthDialogs.h"
#include "ChatDlg.h"

#pragma comment(lib, "Comctl32.lib")
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

class CMFCChatWeek2App : public CWinApp
{
public:
    BOOL InitInstance() override
    {
        INITCOMMONCONTROLSEX controls{};
        controls.dwSize = sizeof(controls);
        controls.dwICC = ICC_WIN95_CLASSES | ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES;
        if (!::InitCommonControlsEx(&controls)) return FALSE;
        CWinApp::InitInstance();
        FakeData data;

        while (true)
        {
            CLoginDlg login(data, nullptr);
            m_pMainWnd = &login;
            INT_PTR loginResult = login.DoModal();
            m_pMainWnd = nullptr;

            if (loginResult == -1)
            {
                ::MessageBoxW(nullptr, L"Could not create Login dialog. Check resources.",
                    L"MFC Chat", MB_OK | MB_ICONERROR);
                break;
            }
            if (loginResult != IDOK) break;

            CChatDlg chat(login.LoggedUser());
            m_pMainWnd = &chat;
            INT_PTR chatResult = chat.DoModal();
            m_pMainWnd = nullptr;

            if (chatResult == -1)
            {
                ::MessageBoxW(nullptr, L"Could not create Chat dialog. Check resources.",
                    L"MFC Chat", MB_OK | MB_ICONERROR);
                break;
            }
            if (chatResult != IDRETRY) break;
        }
        return FALSE;
    }
};

CMFCChatWeek2App theApp;
