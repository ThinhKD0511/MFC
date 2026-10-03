#include "MFCFileWeek1Dlg.h"
#include <afxdlgs.h>
#include <atlconv.h>

BEGIN_MESSAGE_MAP(CMFCFileWeek1Dlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_FOLDER, &CMFCFileWeek1Dlg::OnChooseFolder)
    ON_BN_CLICKED(IDC_BTN_SAVE, &CMFCFileWeek1Dlg::OnSave)
    ON_BN_CLICKED(IDC_BTN_LOAD, &CMFCFileWeek1Dlg::OnLoad)
END_MESSAGE_MAP()

CMFCFileWeek1Dlg::CMFCFileWeek1Dlg(CWnd* parent)
    : CDialogEx(IDD, parent)
{
}

void CMFCFileWeek1Dlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Text(pDX, IDC_EDIT_CONTENT, m_content);
    DDX_Text(pDX, IDC_EDIT_PATH, m_filePath);
}

BOOL CMFCFileWeek1Dlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    m_content = L"Hello world MFC!";
    UpdateData(FALSE); 
    return TRUE;
}

void CMFCFileWeek1Dlg::OnOK()
{
}

void CMFCFileWeek1Dlg::OnChooseFolder()
{
    CFolderPickerDialog dlg(nullptr, OFN_PATHMUSTEXIST, this);
    if (dlg.DoModal() != IDOK) return;

    UpdateData(TRUE); 
    m_filePath = dlg.GetPathName();
    if (m_filePath.Right(1) != L"\\") m_filePath += L"\\";
    m_filePath += L"HelloWorld.txt";
    UpdateData(FALSE);
}

void CMFCFileWeek1Dlg::OnSave()
{
    UpdateData(TRUE); 
    if (m_filePath.IsEmpty())
    {
        AfxMessageBox(L"Hay chon folder truoc khi Save.");
        return;
    }

    // Chuyen chuoi Unicode cua MFC sang UTF-8 de luu tieng Viet.
    CStringA data(CW2A(m_content.GetString(), CP_UTF8));
    HANDLE file = CreateFileW(m_filePath.GetString(), GENERIC_WRITE, 0,
        nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        CString message;
        message.Format(L"Khong mo duoc file de ghi. Ma loi: %lu", GetLastError());
        AfxMessageBox(message);
        return;
    }

    DWORD written = 0;
    DWORD size = static_cast<DWORD>(data.GetLength());
    BOOL ok = WriteFile(file, data.GetString(), size, &written, nullptr);
    CloseHandle(file);
    if (!ok || written != size)
    {
        AfxMessageBox(L"Ghi file that bai.");
        return;
    }
    AfxMessageBox(L"Save thanh cong!");
}

void CMFCFileWeek1Dlg::OnLoad()
{
    UpdateData(TRUE);
    if (m_filePath.IsEmpty())
    {
        AfxMessageBox(L"Hay chon folder truoc khi Load.");
        return;
    }

    HANDLE file = CreateFileW(m_filePath.GetString(), GENERIC_READ, FILE_SHARE_READ,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        CString message;
        message.Format(L"Khong mo duoc file de doc. Ma loi: %lu", GetLastError());
        AfxMessageBox(message);
        return;
    }

    LARGE_INTEGER size = {};
    BOOL sizeOk = GetFileSizeEx(file, &size);
    if (!sizeOk || size.QuadPart > 1024 * 1024)
    {
        CloseHandle(file);
        AfxMessageBox(L"Khong lay duoc dung luong file hoac file lon hon 1 MiB.");
        return;
    }

    CStringA data;
    char* buffer = data.GetBuffer(static_cast<int>(size.QuadPart) + 1);
    DWORD read = 0;
    BOOL ok = ReadFile(file, buffer, static_cast<DWORD>(size.QuadPart), &read, nullptr);
    CloseHandle(file);
    data.ReleaseBuffer(static_cast<int>(read));
    if (!ok || read != static_cast<DWORD>(size.QuadPart))
    {
        AfxMessageBox(L"Doc file that bai.");
        return;
    }

    m_content = CA2W(data.GetString(), CP_UTF8);
    UpdateData(FALSE); 
}
