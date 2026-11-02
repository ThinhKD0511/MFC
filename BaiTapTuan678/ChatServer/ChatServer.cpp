#include "ServerCore.h"

#include <process.h>
#include <cstdio>
#include <cerrno>
#include <stdlib.h>

struct ServerContext
{
    ServerCore* server;
    HANDLE stopEvent;
};

static unsigned __stdcall ServerThreadProc(void* parameter)
{
    ServerContext* context =
        static_cast<ServerContext*>(parameter);

    DWORD errorCode = context->server->Run(
        context->stopEvent);

    if (errorCode != NO_ERROR)
    {
        std::printf(
            "Server loop failed. Error: %lu\n",
            errorCode);
    }

    return static_cast<unsigned>(errorCode);
}

int RunConsole()
{
    ServerCore server;

    DWORD errorCode = server.Start();

    if (errorCode != NO_ERROR)
    {
        std::printf(
            "Start failed. Error: %lu\n",
            errorCode);

        return static_cast<int>(errorCode);
    }

    HANDLE stopEvent = CreateEventW(
        nullptr, TRUE, FALSE, nullptr);

    if (stopEvent == nullptr)
    {
        errorCode = GetLastError();

        std::printf(
            "CreateEvent failed: %lu\n",
            errorCode);

        return static_cast<int>(errorCode);
    }

    ServerContext context = { &server, stopEvent };

    auto threadValue = _beginthreadex(
        nullptr,
        0,
        ServerThreadProc,
        &context,
        0,
        nullptr);

    if (threadValue == 0)
    {
        std::printf(
            "Cannot start Server thread. errno: %d\n",
            errno);

        CloseHandle(stopEvent);
        return 1;
    }

    HANDLE serverThread =
        reinterpret_cast<HANDLE>(threadValue);

    std::printf("ChatServer listening on 127.0.0.1:9999\n");
    std::printf("Maximum Clients: %d\n", 16);
    std::printf("Press Enter to stop.\n");

    wchar_t input[2] = {};
    DWORD charactersRead = 0;

    if (!ReadConsoleW(
        GetStdHandle(STD_INPUT_HANDLE),
        input,
        _countof(input),
        &charactersRead,
        nullptr))
    {
        std::printf(
            "ReadConsole failed: %lu\n",
            GetLastError());
    }

    SetEvent(stopEvent);

    WaitForSingleObject(serverThread, INFINITE);

    DWORD runError = NO_ERROR;

    if (!GetExitCodeThread(serverThread, &runError))
        runError = GetLastError();

    CloseHandle(serverThread);
    CloseHandle(stopEvent);

    std::printf("Server stopped.\n");

    return static_cast<int>(runError);
}

#pragma comment(lib, "Advapi32.lib")

static wchar_t SERVICE_NAME[] = L"ChatFinalService";

static SERVICE_STATUS_HANDLE g_statusHandle = nullptr;
static SERVICE_STATUS g_serviceStatus = {};

static HANDLE g_serviceStopEvent = nullptr;
static SRWLOCK g_serviceLock = SRWLOCK_INIT;


static void SetStatusLocked(
    DWORD state,
    DWORD error = NO_ERROR,
    DWORD waitHint = 0)
{
    g_serviceStatus.dwServiceType =
        SERVICE_WIN32_OWN_PROCESS;

    g_serviceStatus.dwCurrentState = state;

    g_serviceStatus.dwControlsAccepted =
        state == SERVICE_RUNNING
        ? SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN
        : 0;

    g_serviceStatus.dwWin32ExitCode = error;
    g_serviceStatus.dwServiceSpecificExitCode = 0;

    bool pending =
        state == SERVICE_START_PENDING ||
        state == SERVICE_STOP_PENDING;

    g_serviceStatus.dwCheckPoint = pending ? 1 : 0;
    g_serviceStatus.dwWaitHint = pending ? waitHint : 0;

    SetServiceStatus(
        g_statusHandle,
        &g_serviceStatus);
}


static void ReportServiceStatus(
    DWORD state,
    DWORD error = NO_ERROR,
    DWORD waitHint = 0)
{
    AcquireSRWLockExclusive(&g_serviceLock);

    SetStatusLocked(state, error, waitHint);

    ReleaseSRWLockExclusive(&g_serviceLock);
}


static DWORD WINAPI ServiceControlHandler(
    DWORD control,
    DWORD,
    LPVOID,
    LPVOID)
{
    if (control == SERVICE_CONTROL_INTERROGATE)
    {
        return NO_ERROR;
    }

    if (control != SERVICE_CONTROL_STOP &&
        control != SERVICE_CONTROL_SHUTDOWN)
    {
        return ERROR_CALL_NOT_IMPLEMENTED;
    }

    AcquireSRWLockExclusive(&g_serviceLock);

    if (g_serviceStatus.dwCurrentState == SERVICE_RUNNING &&
        g_serviceStopEvent != nullptr)
    {
        SetStatusLocked(
            SERVICE_STOP_PENDING,
            NO_ERROR,
            60000);

        SetEvent(g_serviceStopEvent);
    }

    ReleaseSRWLockExclusive(&g_serviceLock);

    return NO_ERROR;
}


static void WINAPI ServiceMain(DWORD, LPWSTR*)
{
    g_statusHandle = RegisterServiceCtrlHandlerExW(
        SERVICE_NAME,
        ServiceControlHandler,
        nullptr);

    if (g_statusHandle == nullptr)
    {
        return;
    }

    ReportServiceStatus(
        SERVICE_START_PENDING,
        NO_ERROR,
        10000);

    DWORD error = NO_ERROR;

    {
        ServerCore server;

        HANDLE stopEvent = CreateEventW(
            nullptr,
            TRUE,
            FALSE,
            nullptr);

        if (stopEvent == nullptr)
        {
            error = GetLastError();
        }
        else
        {
            AcquireSRWLockExclusive(&g_serviceLock);

            g_serviceStopEvent = stopEvent;

            ReleaseSRWLockExclusive(&g_serviceLock);

            error = server.Start();

            if (error == NO_ERROR)
            {
                ReportServiceStatus(SERVICE_RUNNING);

                error = server.Run(stopEvent);

                AcquireSRWLockExclusive(&g_serviceLock);

                if (g_serviceStatus.dwCurrentState ==
                    SERVICE_RUNNING)
                {
                    SetStatusLocked(
                        SERVICE_STOP_PENDING,
                        error,
                        60000);
                }

                ReleaseSRWLockExclusive(&g_serviceLock);
            }
        }

    } 

    AcquireSRWLockExclusive(&g_serviceLock);

    if (g_serviceStopEvent != nullptr)
    {
        CloseHandle(g_serviceStopEvent);
        g_serviceStopEvent = nullptr;
    }

    SetStatusLocked(SERVICE_STOPPED, error);

    ReleaseSRWLockExclusive(&g_serviceLock);
}


int main()
{
    SERVICE_TABLE_ENTRYW serviceTable[] =
    {
        { SERVICE_NAME, ServiceMain },
        { nullptr, nullptr }
    };

    if (StartServiceCtrlDispatcherW(serviceTable))
    {
        return 0;
    }

    DWORD error = GetLastError();

    if (error == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT)
    {
        return RunConsole();
    }

    printf(
        "StartServiceCtrlDispatcher failed. Error: %lu\n",
        error);

    return static_cast<int>(error);
}