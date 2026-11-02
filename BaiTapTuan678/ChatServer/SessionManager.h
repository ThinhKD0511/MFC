#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <string>

#include "sqlite3.h"

struct ClientSession
{
    sqlite3_int64 userId = 0;
    std::string username;
};

enum class SessionResult
{
    Success,
    AlreadyOnline,
    Full
};

class SessionManager
{
public:
    static constexpr int MAX_SESSIONS = 16;

    SessionManager()
    {
        InitializeCriticalSection(&m_lock);
    }

    ~SessionManager()
    {
        DeleteCriticalSection(&m_lock);
    }

    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;

    SessionResult TryLogin(
        int clientId,
        sqlite3_int64 userId)
    {
        EnterCriticalSection(&m_lock);

        SessionResult outcome = SessionResult::Full;
        int freeIndex = -1;

        for (int i = 0; i < MAX_SESSIONS; ++i)
        {
            if (m_entries[i].userId == userId)
            {
                outcome = SessionResult::AlreadyOnline;
                break;
            }

            if (m_entries[i].userId == 0 &&
                freeIndex == -1)
            {
                freeIndex = i;
            }
        }

        if (outcome != SessionResult::AlreadyOnline &&
            freeIndex != -1)
        {
            m_entries[freeIndex].clientId = clientId;
            m_entries[freeIndex].userId = userId;

            outcome = SessionResult::Success;
        }

        LeaveCriticalSection(&m_lock);

        return outcome;
    }

    void Logout(int clientId)
    {
        EnterCriticalSection(&m_lock);

        for (int i = 0; i < MAX_SESSIONS; ++i)
        {
            if (m_entries[i].clientId == clientId)
            {
                m_entries[i].clientId = 0;
                m_entries[i].userId = 0;
                break;
            }
        }

        LeaveCriticalSection(&m_lock);
    }

    bool IsOnline(sqlite3_int64 userId)
    {
        if (userId <= 0)
            return false;

        EnterCriticalSection(&m_lock);

        bool online = false;

        for (int i = 0; i < MAX_SESSIONS; ++i)
        {
            if (m_entries[i].userId == userId)
            {
                online = true;
                break;
            }
        }

        LeaveCriticalSection(&m_lock);

        return online;
    }

private:
    struct Entry
    {
        int clientId = 0;
        sqlite3_int64 userId = 0;
    };

    Entry m_entries[MAX_SESSIONS] = {};
    CRITICAL_SECTION m_lock = {};
};