#pragma once
#include <string>
#include <vector>

struct Account { std::wstring username, password; };
struct ChatMessage { std::wstring time, sender, text; };

class FakeData
{
public:
    FakeData();
    bool CheckLogin(const std::wstring& user, const std::wstring& password) const;
    bool UserExists(const std::wstring& user) const;
    bool RegisterAccount(const std::wstring& user, const std::wstring& password);
    static bool ValidUsername(const std::wstring& user);
    static std::vector<ChatMessage> SeedMessages(int room);
private:
    std::vector<Account> m_accounts;
};

