#pragma once
#include "framework.h"

namespace Win
{
    inline std::wstring Text(HWND dialog, int id)
    {
        HWND edit = ::GetDlgItem(dialog, id);
        int length = ::GetWindowTextLengthW(edit);
        std::vector<wchar_t> buffer(static_cast<size_t>(length) + 1, L'\0');
        ::GetDlgItemTextW(dialog, id, buffer.data(), length + 1);
        return buffer.data();
    }
    inline std::wstring Trim(const std::wstring& value)
    {
        const auto first = value.find_first_not_of(L" \t\r\n");
        if (first == std::wstring::npos) return L"";
        return value.substr(first, value.find_last_not_of(L" \t\r\n") - first + 1);
    }
    inline void Notice(HWND owner, const wchar_t* text)
    {
        ::MessageBoxW(owner, text, L"MFC Chat", MB_OK | MB_ICONINFORMATION);
    }
    inline void Limit(HWND dialog, int id, int count)
    {
        ::SendDlgItemMessageW(dialog, id, EM_SETLIMITTEXT, count, 0);
    }
}

