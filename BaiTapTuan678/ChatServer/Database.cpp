#include "Database.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <cstdio>
#include <string>
#include <strsafe.h>

static bool ReadMessageRow(
    sqlite3_stmt* statement,
    MessageInfo& message)
{
    message.id =
        sqlite3_column_int64(statement, 0);

    message.senderId =
        sqlite3_column_int64(statement, 1);

    message.receiverId =
        sqlite3_column_int64(statement, 2);

    const unsigned char* content =
        sqlite3_column_text(statement, 3);

    const unsigned char* createdAt =
        sqlite3_column_text(statement, 4);

    if (content == nullptr || createdAt == nullptr)
        return false;

    if (FAILED(StringCchCopyA(
        message.content,
        sizeof(message.content),
        reinterpret_cast<const char*>(content))))
    {
        return false;
    }

    return SUCCEEDED(StringCchCopyA(
        message.createdAt,
        sizeof(message.createdAt),
        reinterpret_cast<const char*>(createdAt)));
}

Database::Database()
{
    InitializeCriticalSection(&m_lock);
}

Database::~Database()
{
    Close();
    DeleteCriticalSection(&m_lock);
}

bool Database::Open(const wchar_t* filePath)
{
    if (m_db != nullptr)
    {
        std::printf("Database is already open.\n");
        return false;
    }

    int required = WideCharToMultiByte(
        CP_UTF8,
        WC_ERR_INVALID_CHARS,
        filePath,
        -1,
        nullptr,
        0,
        nullptr,
        nullptr);

    if (required == 0)
    {
        std::printf(
            "Cannot convert database path. Error: %lu\n",
            GetLastError());

        return false;
    }

    std::string utf8Path(required, '\0');

    if (WideCharToMultiByte(
        CP_UTF8,
        WC_ERR_INVALID_CHARS,
        filePath,
        -1,
        utf8Path.data(),
        required,
        nullptr,
        nullptr) == 0)
    {
        std::printf(
            "Cannot convert database path. Error: %lu\n",
            GetLastError());

        return false;
    }

    int result = sqlite3_open_v2(
        utf8Path.c_str(),
        &m_db,
        SQLITE_OPEN_READWRITE |
        SQLITE_OPEN_CREATE |
        SQLITE_OPEN_FULLMUTEX,
        nullptr);

    if (result != SQLITE_OK)
    {
        std::printf(
            "Cannot open database (%d): %s\n",
            result,
            m_db != nullptr
            ? sqlite3_errmsg(m_db)
            : "No database handle");

        Close();
        return false;
    }

    const char* sql = R"SQL(
        PRAGMA foreign_keys = ON;
        PRAGMA busy_timeout = 3000;

        BEGIN;

        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT NOT NULL UNIQUE,
            password_hash TEXT NOT NULL,
            password_algorithm TEXT NOT NULL,
            created_at TEXT NOT NULL
                DEFAULT (
                    strftime('%Y-%m-%dT%H:%M:%fZ', 'now')
                )
        );

        CREATE TABLE IF NOT EXISTS messages (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            sender_id INTEGER NOT NULL,
            receiver_id INTEGER NOT NULL,
            content TEXT NOT NULL,
            created_at TEXT NOT NULL
                DEFAULT (
                    strftime('%Y-%m-%dT%H:%M:%fZ', 'now')
                ),

            FOREIGN KEY (sender_id) REFERENCES users(id),
            FOREIGN KEY (receiver_id) REFERENCES users(id),

            CHECK (
                length(CAST(content AS BLOB))
                BETWEEN 1 AND 4096
            )
        );

        CREATE INDEX IF NOT EXISTS idx_messages_pair_id
            ON messages(sender_id, receiver_id, id);

        COMMIT;
    )SQL";

    char* errorMessage = nullptr;

    result = sqlite3_exec(
        m_db,
        sql,
        nullptr,
        nullptr,
        &errorMessage);

    if (result != SQLITE_OK)
    {
        std::printf(
            "Cannot initialize database: %s\n",
            errorMessage != nullptr
            ? errorMessage
            : sqlite3_errmsg(m_db));

        sqlite3_free(errorMessage);

        Close();
        return false;
    }

    return true;
}

void Database::Close()
{
    if (m_db == nullptr)
        return;

    int result = sqlite3_close(m_db);

    if (result == SQLITE_OK)
    {
        m_db = nullptr;
    }
    else
    {
        std::printf(
            "Cannot close database: %s\n",
            sqlite3_errmsg(m_db));
    }
}

RegisterResult Database::RegisterUser(
    const std::string& username,
    const std::string& passwordHash,
    sqlite3_int64& userId)
{
    userId = 0;

    EnterCriticalSection(&m_lock);

    RegisterResult outcome =
        RegisterResult::DatabaseError;

    sqlite3_stmt* statement = nullptr;

    do
    {
        if (m_db == nullptr)
            break;

        const char* sql =
            "INSERT INTO users "
            "(username, password_hash, password_algorithm) "
            "VALUES (?, ?, 'md5');";

        int result = sqlite3_prepare_v2(
            m_db,
            sql,
            -1,
            &statement,
            nullptr);

        if (result != SQLITE_OK)
            break;

        result = sqlite3_bind_text(
            statement,
            1,
            username.c_str(),
            static_cast<int>(username.size()),
            SQLITE_TRANSIENT);

        if (result != SQLITE_OK)
            break;

        result = sqlite3_bind_text(
            statement,
            2,
            passwordHash.c_str(),
            static_cast<int>(passwordHash.size()),
            SQLITE_TRANSIENT);

        if (result != SQLITE_OK)
            break;

        result = sqlite3_step(statement);

        if (result == SQLITE_DONE)
        {
            userId = sqlite3_last_insert_rowid(m_db);
            outcome = RegisterResult::Success;
        }
        else if (sqlite3_extended_errcode(m_db) ==
            SQLITE_CONSTRAINT_UNIQUE)
        {
            outcome = RegisterResult::UsernameExists;
        }
    } while (false);

    if (outcome == RegisterResult::DatabaseError &&
        m_db != nullptr)
    {
        std::printf(
            "RegisterUser failed: %s\n",
            sqlite3_errmsg(m_db));
    }

    sqlite3_finalize(statement);

    LeaveCriticalSection(&m_lock);

    return outcome;
}

AuthResult Database::AuthenticateUser(
    const std::string& username,
    const std::string& passwordHash,
    sqlite3_int64& userId)
{
    userId = 0;

    EnterCriticalSection(&m_lock);

    AuthResult outcome = AuthResult::DatabaseError;
    sqlite3_stmt* statement = nullptr;

    do
    {
        if (m_db == nullptr)
            break;

        const char* sql =
            "SELECT id FROM users "
            "WHERE username = ? "
            "AND password_hash = ? "
            "AND password_algorithm = 'md5';";

        int result = sqlite3_prepare_v2(
            m_db,
            sql,
            -1,
            &statement,
            nullptr);

        if (result != SQLITE_OK)
            break;

        result = sqlite3_bind_text(
            statement,
            1,
            username.c_str(),
            static_cast<int>(username.size()),
            SQLITE_TRANSIENT);

        if (result != SQLITE_OK)
            break;

        result = sqlite3_bind_text(
            statement,
            2,
            passwordHash.c_str(),
            static_cast<int>(passwordHash.size()),
            SQLITE_TRANSIENT);

        if (result != SQLITE_OK)
            break;

        result = sqlite3_step(statement);

        if (result == SQLITE_ROW)
        {
            userId = sqlite3_column_int64(statement, 0);
            outcome = AuthResult::Success;
        }
        else if (result == SQLITE_DONE)
        {
            outcome = AuthResult::InvalidCredentials;
        }
    } while (false);

    if (outcome == AuthResult::DatabaseError &&
        m_db != nullptr)
    {
        std::printf(
            "AuthenticateUser failed: %s\n",
            sqlite3_errmsg(m_db));
    }

    sqlite3_finalize(statement);

    LeaveCriticalSection(&m_lock);

    return outcome;
}

bool Database::GetUsers(
    sqlite3_int64 excludeUserId,
    sqlite3_int64 afterUserId,
    std::vector<UserInfo>& users,
    bool& hasMore)
{
    constexpr int PAGE_SIZE = 50;

    users.clear();
    hasMore = false;

    users.reserve(PAGE_SIZE + 1);

    EnterCriticalSection(&m_lock);

    sqlite3_stmt* statement = nullptr;
    bool success = false;
    int result = SQLITE_ERROR;

    do
    {
        if (m_db == nullptr)
            break;

        const char* sql =
            "SELECT id, username FROM users "
            "WHERE id <> ? AND id > ? "
            "ORDER BY id ASC "
            "LIMIT ?;";

        result = sqlite3_prepare_v2(
            m_db,
            sql,
            -1,
            &statement,
            nullptr);

        if (result != SQLITE_OK)
            break;

        result = sqlite3_bind_int64(
            statement, 1, excludeUserId);

        if (result != SQLITE_OK)
            break;

        result = sqlite3_bind_int64(
            statement, 2, afterUserId);

        if (result != SQLITE_OK)
            break;

        result = sqlite3_bind_int(
            statement, 3, PAGE_SIZE + 1);

        if (result != SQLITE_OK)
            break;

        while ((result = sqlite3_step(statement)) ==
            SQLITE_ROW)
        {
            UserInfo user;

            user.userId =
                sqlite3_column_int64(statement, 0);

            const unsigned char* name =
                sqlite3_column_text(statement, 1);

            if (name == nullptr ||
                FAILED(StringCchCopyA(
                    user.username,
                    sizeof(user.username),
                    reinterpret_cast<const char*>(name))))
            {
                std::printf(
                    "GetUsers: invalid username in database.\n");

                result = SQLITE_ERROR;
                break;
            }

            users.push_back(user);
        }

        success = (result == SQLITE_DONE);
    } while (false);

    if (!success && m_db != nullptr)
    {
        std::printf(
            "GetUsers failed (%d): %s\n",
            result,
            sqlite3_errmsg(m_db));
    }

    sqlite3_finalize(statement);

    LeaveCriticalSection(&m_lock);

    if (!success)
    {
        users.clear();
        return false;
    }

    hasMore = users.size() > PAGE_SIZE;

    if (hasMore)
        users.resize(PAGE_SIZE);

    return true;
}

SendResult Database::SaveMessage(
    sqlite3_int64 senderId,
    sqlite3_int64 receiverId,
    const std::string& content,
    MessageInfo& message)
{
    message = {};

    if (content.empty() ||
        content.size() > 4096 ||
        content.find('\0') != std::string::npos)
    {
        return SendResult::DatabaseError;
    }

    EnterCriticalSection(&m_lock);

    sqlite3_stmt* statement = nullptr;
    SendResult outcome = SendResult::DatabaseError;
    int result = SQLITE_ERROR;

    do
    {
        if (m_db == nullptr)
            break;

        const char* sql =
            "INSERT INTO messages "
            "(sender_id, receiver_id, content) "
            "SELECT ?1, ?2, ?3 "
            "WHERE EXISTS "
            "(SELECT 1 FROM users WHERE id = ?2) "
            "RETURNING "
            "id, sender_id, receiver_id, content, created_at;";

        result = sqlite3_prepare_v2(
            m_db,
            sql,
            -1,
            &statement,
            nullptr);

        if (result != SQLITE_OK)
            break;

        result = sqlite3_bind_int64(
            statement, 1, senderId);

        if (result != SQLITE_OK)
            break;

        result = sqlite3_bind_int64(
            statement, 2, receiverId);

        if (result != SQLITE_OK)
            break;

        result = sqlite3_bind_text(
            statement,
            3,
            content.c_str(),
            static_cast<int>(content.size()),
            SQLITE_TRANSIENT);

        if (result != SQLITE_OK)
            break;

        result = sqlite3_step(statement);

        if (result == SQLITE_DONE)
        {
            outcome = SendResult::ReceiverNotFound;
            break;
        }

        if (result != SQLITE_ROW)
            break;

        if (!ReadMessageRow(statement, message))
        {
            result = SQLITE_ERROR;
            break;
        }

        result = sqlite3_step(statement);

        if (result == SQLITE_DONE)
            outcome = SendResult::Success;

    } while (false);

    if (outcome == SendResult::DatabaseError &&
        m_db != nullptr)
    {
        std::printf(
            "SaveMessage failed (%d): %s\n",
            result,
            sqlite3_errmsg(m_db));
    }

    sqlite3_finalize(statement);

    LeaveCriticalSection(&m_lock);

    if (outcome != SendResult::Success)
        message = {};

    return outcome;
}

bool Database::GetMessages(
    sqlite3_int64 userId,
    sqlite3_int64 peerId,
    sqlite3_int64 beforeId,
    int limit,
    std::vector<MessageInfo>& messages,
    bool& hasMore)
{
    messages.clear();
    hasMore = false;

    if (limit < 1 || limit > 100 || beforeId < 0)
        return false;

    messages.reserve(limit + 1);

    EnterCriticalSection(&m_lock);

    sqlite3_stmt* statement = nullptr;
    bool success = false;
    int result = SQLITE_ERROR;

    do
    {
        if (m_db == nullptr)
            break;

        const char* sql =
            "SELECT "
            "id, sender_id, receiver_id, content, created_at "
            "FROM messages "
            "WHERE "
            "((sender_id = ?1 AND receiver_id = ?2) "
            "OR (sender_id = ?2 AND receiver_id = ?1)) "
            "AND (?3 = 0 OR id < ?3) "
            "ORDER BY id DESC "
            "LIMIT ?4;";

        result = sqlite3_prepare_v2(
            m_db,
            sql,
            -1,
            &statement,
            nullptr);

        if (result != SQLITE_OK)
            break;

        result = sqlite3_bind_int64(
            statement, 1, userId);

        if (result != SQLITE_OK)
            break;

        result = sqlite3_bind_int64(
            statement, 2, peerId);

        if (result != SQLITE_OK)
            break;

        result = sqlite3_bind_int64(
            statement, 3, beforeId);

        if (result != SQLITE_OK)
            break;

        result = sqlite3_bind_int(
            statement, 4, limit + 1);

        if (result != SQLITE_OK)
            break;

        while ((result = sqlite3_step(statement)) ==
            SQLITE_ROW)
        {
            MessageInfo message;

            if (!ReadMessageRow(statement, message))
            {
                result = SQLITE_ERROR;
                break;
            }

            messages.push_back(message);
        }

        success = (result == SQLITE_DONE);

    } while (false);

    if (!success && m_db != nullptr)
    {
        std::printf(
            "GetMessages failed (%d): %s\n",
            result,
            sqlite3_errmsg(m_db));
    }

    sqlite3_finalize(statement);

    LeaveCriticalSection(&m_lock);

    if (!success)
    {
        messages.clear();
        return false;
    }

    hasMore =
        messages.size() > static_cast<size_t>(limit);

    if (hasMore)
        messages.resize(limit);

    return true;
}