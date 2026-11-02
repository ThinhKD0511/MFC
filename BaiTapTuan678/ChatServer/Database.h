#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <string>
#include <vector>
#include "sqlite3.h"

enum class RegisterResult
{
    Success,
    UsernameExists,
    DatabaseError
};

enum class AuthResult
{
    Success,
    InvalidCredentials,
    DatabaseError
};

struct UserInfo
{
    sqlite3_int64 userId = 0;
    char username[33] = {};
};

enum class SendResult
{
    Success,
    ReceiverNotFound,
    DatabaseError
};

struct MessageInfo
{
    sqlite3_int64 id = 0;
    sqlite3_int64 senderId = 0;
    sqlite3_int64 receiverId = 0;

    char content[4097] = {};
    char createdAt[32] = {};
};

class Database
{
public:
    Database();
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    bool Open(const wchar_t* filePath);
    void Close();

    RegisterResult RegisterUser(
        const std::string& username,
        const std::string& passwordHash,
        sqlite3_int64& userId);
    AuthResult AuthenticateUser(
        const std::string& username,
        const std::string& passwordHash,
        sqlite3_int64& userId);
    bool GetUsers(
        sqlite3_int64 excludeUserId,
        sqlite3_int64 afterUserId,
        std::vector<UserInfo>& users,
        bool& hasMore);
    SendResult SaveMessage(
        sqlite3_int64 senderId,
        sqlite3_int64 receiverId,
        const std::string& content,
        MessageInfo& message);

    bool GetMessages(
        sqlite3_int64 userId,
        sqlite3_int64 peerId,
        sqlite3_int64 beforeId,
        int limit,
        std::vector<MessageInfo>& messages,
        bool& hasMore);
private:
    sqlite3* m_db = nullptr;
    CRITICAL_SECTION m_lock = {};
};