#define WIN32_LEAN_AND_MEAN
#include <WinSock2.h>
#include <windows.h>
#include <cwchar>

#pragma comment(lib, "Advapi32.lib")
#pragma comment(lib, "Ws2_32.lib")

wchar_t SERVICE_NAME[] = L"TCPEchoService";

SERVICE_STATUS_HANDLE g_statusHandle = nullptr;
HANDLE g_stopEvent = nullptr;

void ReportStatus(DWORD state, DWORD errorCode = NO_ERROR)
{
    SERVICE_STATUS status = {};

    status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    status.dwCurrentState = state;
    status.dwWin32ExitCode = errorCode;

    if (state == SERVICE_RUNNING)
        status.dwControlsAccepted = SERVICE_ACCEPT_STOP;

    if (state == SERVICE_START_PENDING ||
        state == SERVICE_STOP_PENDING)
    {
        status.dwCheckPoint = 1;
        status.dwWaitHint = 3000;
    }

    SetServiceStatus(g_statusHandle, &status);
}

void WINAPI ServiceHandler(DWORD control)
{
    switch (control)
    {
    case SERVICE_CONTROL_STOP:
        ReportStatus(SERVICE_STOP_PENDING);
        SetEvent(g_stopEvent);
        return;

    case SERVICE_CONTROL_INTERROGATE:
        return;

    default:
        return;
    }
}

DWORD RunEchoServer()
{
    WSADATA wsaData = {};

    DWORD errorCode = WSAStartup(MAKEWORD(2, 2), &wsaData);

    if (errorCode != 0)
        return errorCode;

    SOCKET listenSocket = INVALID_SOCKET;
    SOCKET clientSocket = INVALID_SOCKET;

    sockaddr_in address = {};
    address.sin_family = AF_INET;
    address.sin_port = htons(9999);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    u_long nonBlocking = 1;

    do
    {
        listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

        if (listenSocket == INVALID_SOCKET)
        {
            errorCode = WSAGetLastError();
            break;
        }

        if (ioctlsocket(listenSocket, FIONBIO, &nonBlocking)
            == SOCKET_ERROR ||
            bind(listenSocket,
                reinterpret_cast<sockaddr*>(&address),
                sizeof(address)) == SOCKET_ERROR ||
            listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
        {
            errorCode = WSAGetLastError();
            break;
        }

        ReportStatus(SERVICE_RUNNING);

        char buffer[512];
        int received = 0;
        int sent = 0;

        while (WaitForSingleObject(g_stopEvent, 0) == WAIT_TIMEOUT)
        {
            SOCKET activeSocket =
                (clientSocket == INVALID_SOCKET)
                ? listenSocket : clientSocket;

            bool writing =
                clientSocket != INVALID_SOCKET && sent < received;

            fd_set socketSet;
            FD_ZERO(&socketSet);
            FD_SET(activeSocket, &socketSet);

            timeval timeout = { 0, 200000 };

            int ready = select(
                0,
                writing ? nullptr : &socketSet,
                writing ? &socketSet : nullptr,
                nullptr,
                &timeout);

            if (ready == SOCKET_ERROR)
            {
                errorCode = WSAGetLastError();
                break;
            }

            if (ready == 0)
                continue;

            if (clientSocket == INVALID_SOCKET)
            {
                clientSocket = accept(listenSocket, nullptr, nullptr);

                if (clientSocket == INVALID_SOCKET)
                {
                    int error = WSAGetLastError();

                    if (error == WSAEWOULDBLOCK)
                        continue;

                    errorCode = error;
                    break;
                }

                if (ioctlsocket(clientSocket, FIONBIO, &nonBlocking)
                    == SOCKET_ERROR)
                {
                    errorCode = WSAGetLastError();
                    break;
                }

                received = 0;
                sent = 0;
                continue;
            }

            int result;

            if (writing)
            {
                result = send(
                    clientSocket,
                    buffer + sent,
                    received - sent,
                    0);
            }
            else
            {
                result = recv(
                    clientSocket,
                    buffer,
                    sizeof(buffer),
                    0);
            }

            if (result == SOCKET_ERROR &&
                WSAGetLastError() == WSAEWOULDBLOCK)
            {
                continue;
            }

            if (result <= 0)
            {
                closesocket(clientSocket);
                clientSocket = INVALID_SOCKET;

                received = 0;
                sent = 0;
                continue;
            }

            if (writing)
            {
                sent += result;
            }
            else
            {
                received = result;
                sent = 0;
            }
        }
    } while (false);

    if (clientSocket != INVALID_SOCKET)
        closesocket(clientSocket);

    if (listenSocket != INVALID_SOCKET)
        closesocket(listenSocket);

    WSACleanup();

    return errorCode;
}

void WINAPI ServiceMain(DWORD, LPWSTR*)
{
    g_statusHandle = RegisterServiceCtrlHandlerW(
        SERVICE_NAME, ServiceHandler);

    if (g_statusHandle == nullptr)
        return;

    ReportStatus(SERVICE_START_PENDING);

    g_stopEvent = CreateEventW(
        nullptr,
        TRUE,
        FALSE,
        nullptr);

    if (g_stopEvent == nullptr)
    {
        ReportStatus(SERVICE_STOPPED, GetLastError());
        return;
    }

    DWORD errorCode = RunEchoServer();

    CloseHandle(g_stopEvent);
    g_stopEvent = nullptr;

    ReportStatus(SERVICE_STOPPED, errorCode);
}

int wmain()
{
    SERVICE_TABLE_ENTRYW serviceTable[] =
    {
        { SERVICE_NAME, ServiceMain },
        { nullptr, nullptr }
    };

    if (!StartServiceCtrlDispatcherW(serviceTable))
    {
        DWORD errorCode = GetLastError();

        std::wprintf(
            L"StartServiceCtrlDispatcher failed. Error: %lu\n",
            errorCode);

        return static_cast<int>(errorCode);
    }

    return 0;
}