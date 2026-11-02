#pragma once

#include <winsock2.h>
#include <string>
#include <vector>

struct ClientUserItem
{
    long long userId = 0;
    CString username;
    bool online = false;
};

struct ClientMessageItem
{
    long long id = 0;
    long long senderId = 0;
    long long receiverId = 0;
    CString content;
    CString createdAt;
};

class ClientConnection
{
public:
    static ClientConnection& Instance();

    ClientConnection(const ClientConnection&) = delete;
    ClientConnection& operator=(const ClientConnection&) = delete;

    bool EnsureConnected(CString& errorMsg);
    void Disconnect();

    bool Register(
        const CString& username,
        const CString& password,
        CString& errorMsg);

    bool Login(
        const CString& username,
        const CString& password,
        CString& errorMsg);

    bool Logout(CString& errorMsg);

    bool GetUsers(
        std::vector<ClientUserItem>& users,
        CString& errorMsg);

    bool SendChatMessage(
        long long receiverId,
        const CString& content,
        ClientMessageItem& sentMessage,
        CString& errorMsg);

    bool GetMessages(
        long long peerId,
        long long beforeId,
        int limit,
        std::vector<ClientMessageItem>& messages,
        CString& errorMsg);

    long long GetCurrentUserId() const { return m_userId; }
    const CString& GetCurrentUsername() const { return m_username; }
    bool IsLoggedIn() const { return m_userId != 0; }

    static std::string ToUtf8(const CString& wideStr);
    static CString FromUtf8(const std::string& utf8Str);

private:
    ClientConnection() = default;
    ~ClientConnection();

    static CString TranslateErrorCode(const std::string& code);

    SOCKET m_socket = INVALID_SOCKET;
    long long m_nextRequestId = 1;
    long long m_userId = 0;
    CString m_username;
};
