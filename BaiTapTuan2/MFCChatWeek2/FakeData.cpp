#include "FakeData.h"

FakeData::FakeData() : m_accounts{{L"admin", L"123"}, {L"alice", L"123"}, {L"bob", L"123"}} {}

bool FakeData::CheckLogin(const std::wstring& user, const std::wstring& password) const
{
    for (const auto& account : m_accounts)
        if (account.username == user && account.password == password) return true;
    return false;
}

bool FakeData::UserExists(const std::wstring& user) const
{
    for (const auto& account : m_accounts)
        if (account.username == user) return true;
    return false;
}

bool FakeData::ValidUsername(const std::wstring& user)
{
    if (user.size() < 3 || user.size() > 32) return false;
    for (wchar_t c : user)
        if (!(c >= L'a' && c <= L'z') && !(c >= L'A' && c <= L'Z') &&
            !(c >= L'0' && c <= L'9') && c != L'_') return false;
    return true;
}

bool FakeData::RegisterAccount(const std::wstring& user, const std::wstring& password)
{
    if (!ValidUsername(user) || password.size() < 3 || password.size() > 64 || UserExists(user))
        return false;
    m_accounts.push_back({user, password});
    return true;
}

std::vector<ChatMessage> FakeData::SeedMessages(int room)
{
    if (room == 1) return {{L"09:00", L"Alice", L"Hello!"}, {L"09:01", L"Alice", L"How are you?"}};
    if (room == 2) return {{L"09:05", L"Bob", L"Hi!"}};
    if (room == 3) return {{L"09:10", L"Charlie", L"Good morning."}};
    return {{L"09:00", L"A", L"Hello"}, {L"09:01", L"B", L"Hi"},
            {L"09:02", L"Alice", L"Hello everyone!"}};
}

