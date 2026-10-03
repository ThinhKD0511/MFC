#include "ChatDlg.h"
#include "WinHelpers.h"

BEGIN_MESSAGE_MAP(CChatDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_ACCOUNT, &CChatDlg::OnAccount)
    ON_BN_CLICKED(IDC_BTN_SEND, &CChatDlg::OnSend)
    ON_CBN_SELCHANGE(IDC_COMBO_VIEW, &CChatDlg::OnViewChanged)
    ON_NOTIFY(TVN_SELCHANGEDW, IDC_TREE_CONTACTS, &CChatDlg::OnTreeSelection)
    ON_NOTIFY(NM_DBLCLK, IDC_LIST_CHAT, &CChatDlg::OnMessageDoubleClick)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_CHAT, &CChatDlg::OnMessageSelection)
END_MESSAGE_MAP()

CChatDlg::CChatDlg(const std::wstring& username)
    : CDialogEx(IDD_CHAT), m_user(username)
{
    m_room = 4;
    m_view = 0;
}

BOOL CChatDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    CenterWindow();
    Win::Limit(m_hWnd, IDC_EDIT_MESSAGE, 1000);

    HWND combo = ::GetDlgItem(m_hWnd, IDC_COMBO_VIEW);
    ::SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)L"Table");
    ::SendMessageW(combo, CB_ADDSTRING, 0, (LPARAM)L"List");
    ::SendMessageW(combo, CB_SETCURSEL, 0, 0);

    InitList();
    InitTree();
    RefreshAccount();
    return TRUE;
}

void CChatDlg::InitList()
{
    HWND list = ::GetDlgItem(m_hWnd, IDC_LIST_CHAT);
    // LVS_REPORT da duoc dat trong file .rc.
    DWORD extended = LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES;
    ::SendMessageW(list, LVM_SETEXTENDEDLISTVIEWSTYLE, extended, extended);

    RECT client = {};
    ::GetClientRect(list, &client);
    int widths[3] = {client.right * 15 / 100, client.right * 22 / 100, 0};
    widths[2] = client.right - widths[0] - widths[1] - 24;
    const wchar_t* names[3] = {L"Time", L"User", L"Message"};

    for (int i = 0; i < 3; ++i)
    {
        LVCOLUMNW column = {};
        column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
        column.fmt = LVCFMT_LEFT;
        column.cx = widths[i];
        column.pszText = (LPWSTR)names[i];
        ::SendMessageW(list, LVM_INSERTCOLUMNW, i, (LPARAM)&column);
    }
}

HTREEITEM CChatDlg::InsertTreeItem(HTREEITEM parent, const wchar_t* text, int room)
{
    HWND tree = ::GetDlgItem(m_hWnd, IDC_TREE_CONTACTS);
    TVINSERTSTRUCTW item = {};
    item.hParent = parent;
    item.hInsertAfter = TVI_LAST;
    item.item.mask = TVIF_TEXT | TVIF_PARAM;
    item.item.pszText = (LPWSTR)text;
    item.item.lParam = room; // 0 = nhom cha, 1..4 = nguoi/nhom chat.
    return (HTREEITEM)::SendMessageW(tree, TVM_INSERTITEMW, 0, (LPARAM)&item);
}

void CChatDlg::InitTree()
{
    HWND tree = ::GetDlgItem(m_hWnd, IDC_TREE_CONTACTS);
    ::SendMessageW(tree, CCM_SETUNICODEFORMAT, TRUE, 0);

    HTREEITEM friends = InsertTreeItem(TVI_ROOT, L"Friends", 0);
    InsertTreeItem(friends, L"Alice", 1);
    InsertTreeItem(friends, L"Bob", 2);
    InsertTreeItem(friends, L"Charlie", 3);

    HTREEITEM groups = InsertTreeItem(TVI_ROOT, L"Groups", 0);
    HTREEITEM team = InsertTreeItem(groups, L"MFC Team", 4);

    ::SendMessageW(tree, TVM_EXPAND, TVE_EXPAND, (LPARAM)friends);
    ::SendMessageW(tree, TVM_EXPAND, TVE_EXPAND, (LPARAM)groups);
    ::SendMessageW(tree, TVM_SELECTITEM, TVGN_CARET, (LPARAM)team);
}

std::wstring CChatDlg::RoomName()
{
    switch (m_room)
    {
    case 1: return L"Alice";
    case 2: return L"Bob";
    case 3: return L"Charlie";
    default: return L"MFC Team";
    }
}

void CChatDlg::LoadFakeMessages()
{
    // Gan lai danh sach: khong luu/khoi phuc lich su tung phong.
    m_messages = FakeData::SeedMessages(m_room);
    RenderMessages();
    std::wstring title = L"Chat: " + RoomName();
    ::SetDlgItemTextW(m_hWnd, IDC_STATIC_CURRENT_CHAT, title.c_str());
    ::SetDlgItemTextW(m_hWnd, IDC_STATIC_STATUS, L"Ready");
}

void CChatDlg::RenderMessages()
{
    HWND list = ::GetDlgItem(m_hWnd, IDC_LIST_CHAT);
    ::SendMessageW(list, LVM_DELETEALLITEMS, 0, 0);
    for (size_t i = 0; i < m_messages.size(); ++i)
        AddRow(m_messages[i]);
}

void CChatDlg::AddRow(const ChatMessage& message)
{
    HWND list = ::GetDlgItem(m_hWnd, IDC_LIST_CHAT);
    std::wstring label = message.time;
    if (m_view == 1)
        label = message.sender + L": " + message.text;

    LVITEMW item = {};
    item.mask = LVIF_TEXT;
    item.iItem = (int)::SendMessageW(list, LVM_GETITEMCOUNT, 0, 0);
    item.pszText = (LPWSTR)label.c_str();
    int row = (int)::SendMessageW(list, LVM_INSERTITEMW, 0, (LPARAM)&item);
    if (row < 0) return;

    LVITEMW sub = {};
    sub.iSubItem = 1;
    sub.pszText = (LPWSTR)message.sender.c_str();
    ::SendMessageW(list, LVM_SETITEMTEXTW, row, (LPARAM)&sub);

    sub.iSubItem = 2;
    sub.pszText = (LPWSTR)message.text.c_str();
    ::SendMessageW(list, LVM_SETITEMTEXTW, row, (LPARAM)&sub);
    ::SendMessageW(list, LVM_ENSUREVISIBLE, row, FALSE);
}

void CChatDlg::RefreshAccount()
{
    std::wstring label = L"Logged in: " + m_user;
    ::SetDlgItemTextW(m_hWnd, IDC_STATIC_ACCOUNT, label.c_str());
    ::SetDlgItemTextW(m_hWnd, IDC_BTN_ACCOUNT, L"Logout");
}

void CChatDlg::OnAccount()
{
    if (AfxGetApp()->m_pMainWnd == this)
        AfxGetApp()->m_pMainWnd = nullptr;
    EndDialog(IDRETRY);
}

void CChatDlg::OnSend()
{
    if (m_user.empty())
    {
        Win::Notice(m_hWnd, L"Please log in before sending a message.");
        return;
    }

    std::wstring text = Win::Trim(Win::Text(m_hWnd, IDC_EDIT_MESSAGE));
    if (text.empty())
    {
        Win::Notice(m_hWnd, L"Please enter a message.");
        return;
    }

    SYSTEMTIME time = {};
    ::GetLocalTime(&time);
    wchar_t stamp[16] = {};
    ::GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &time, L"HH':'mm", stamp, 16);

    ChatMessage message = {stamp, m_user, text};
    m_messages.push_back(message);
    AddRow(message);
    ::SetDlgItemTextW(m_hWnd, IDC_EDIT_MESSAGE, L"");
    ::SetDlgItemTextW(m_hWnd, IDC_STATIC_STATUS, L"Message added");
}

void CChatDlg::OnViewChanged()
{
    HWND combo = ::GetDlgItem(m_hWnd, IDC_COMBO_VIEW);
    int selected = (int)::SendMessageW(combo, CB_GETCURSEL, 0, 0);
    if (selected != 0 && selected != 1) return;
    m_view = selected;

    HWND list = ::GetDlgItem(m_hWnd, IDC_LIST_CHAT);
    LONG_PTR style = ::GetWindowLongPtrW(list, GWL_STYLE);
    style &= ~LVS_TYPEMASK;
    if (m_view == 0) style |= LVS_REPORT;
    else style |= LVS_LIST;
    ::SetWindowLongPtrW(list, GWL_STYLE, style);
    ::SetWindowPos(list, nullptr, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    RenderMessages();
}

void CChatDlg::OnTreeSelection(NMHDR* header, LRESULT* result)
{
    *result = 0;
    NMTREEVIEWW* change = (NMTREEVIEWW*)header;
    int room = (int)change->itemNew.lParam;
    if (room < 1 || room > 4) return;
    m_room = room;
    LoadFakeMessages();
}

void CChatDlg::OnMessageDoubleClick(NMHDR* header, LRESULT* result)
{
    *result = 0;
    NMITEMACTIVATE* click = (NMITEMACTIVATE*)header;
    int index = click->iItem;
    if (index < 0 || (size_t)index >= m_messages.size()) return;

    ChatMessage message = m_messages[index];
    std::wstring detail = L"Time: " + message.time + L"\nUser: " + message.sender;
    detail += L"\n\n" + message.text;
    Win::Notice(m_hWnd, detail.c_str());
}

void CChatDlg::OnMessageSelection(NMHDR* header, LRESULT* result)
{
    *result = 0;
    NMLISTVIEW* change = (NMLISTVIEW*)header;
    if (!(change->uChanged & LVIF_STATE)) return;
    if (!(change->uNewState & LVIS_SELECTED)) return;
    if (change->uOldState & LVIS_SELECTED) return;
    if (change->iItem < 0) return;

    std::wstring status = L"Selected row: " + std::to_wstring(change->iItem + 1);
    ::SetDlgItemTextW(m_hWnd, IDC_STATIC_STATUS, status.c_str());
}

void CChatDlg::OnOK()
{
}
