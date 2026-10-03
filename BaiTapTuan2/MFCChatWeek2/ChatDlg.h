#pragma once
#include "framework.h"
#include "FakeData.h"

class CChatDlg : public CDialogEx
{
public:
    explicit CChatDlg(const std::wstring& username);

protected:
    BOOL OnInitDialog() override;
    void OnOK() override;

    afx_msg void OnAccount();
    afx_msg void OnSend();
    afx_msg void OnViewChanged();
    afx_msg void OnTreeSelection(NMHDR* header, LRESULT* result);
    afx_msg void OnMessageDoubleClick(NMHDR* header, LRESULT* result);
    afx_msg void OnMessageSelection(NMHDR* header, LRESULT* result);
    DECLARE_MESSAGE_MAP()

private:
    std::wstring m_user;
    int m_room;
    int m_view; // 0 = Table (LVS_REPORT), 1 = List (LVS_LIST).

    // Chi la cac dong dang hien tren man hinh de doi Table/List.
    // Khong co lich su theo phong/tai khoan. Doi phong se thay toan bo danh sach.
    std::vector<ChatMessage> m_messages;

    void InitList();
    void InitTree();
    HTREEITEM InsertTreeItem(HTREEITEM parent, const wchar_t* text, int room);
    void RefreshAccount();
    void LoadFakeMessages();
    void RenderMessages();
    void AddRow(const ChatMessage& message);
    std::wstring RoomName();
};
