#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <windows.h>
#include "Database.h"
#include "SessionManager.h"

class ServerCore
{
public:
    ServerCore() = default;
    ~ServerCore();

    ServerCore(const ServerCore&) = delete;
    ServerCore& operator=(const ServerCore&) = delete;

    DWORD Start();
    DWORD Run(HANDLE stopEvent);

private:
    SOCKET m_listenSocket = INVALID_SOCKET;
    bool m_winsockStarted = false;

    Database m_database;
    SessionManager m_sessions;
};