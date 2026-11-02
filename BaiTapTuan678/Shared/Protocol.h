#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <windows.h>
#include <string>

namespace Protocol
{
    constexpr u_long MAX_FRAME_SIZE = 64 * 1024;

    inline bool WaitReady(
        SOCKET socket,
        bool writing,
        HANDLE stopEvent)
    {
        while (stopEvent == nullptr ||
            WaitForSingleObject(stopEvent, 0) == WAIT_TIMEOUT)
        {
            fd_set socketSet;
            FD_ZERO(&socketSet);
            FD_SET(socket, &socketSet);

            timeval timeout = { 0, 200000 };

            int ready = select(
                0,
                writing ? nullptr : &socketSet,
                writing ? &socketSet : nullptr,
                nullptr,
                &timeout);

            if (ready == SOCKET_ERROR)
                return false;

            if (ready > 0)
                return true;
        }

        return false;
    }

    inline bool SendAll(
        SOCKET socket,
        const char* buffer,
        int length,
        HANDLE stopEvent)
    {
        int offset = 0;

        while (offset < length)
        {
            if (!WaitReady(socket, true, stopEvent))
                return false;

            int sent = send(
                socket,
                buffer + offset,
                length - offset,
                0);

            if (sent == SOCKET_ERROR)
            {
                if (WSAGetLastError() == WSAEWOULDBLOCK)
                    continue;

                return false;
            }

            if (sent == 0)
                return false;

            offset += sent;
        }

        return true;
    }

    inline bool ReceiveAll(
        SOCKET socket,
        char* buffer,
        int length,
        HANDLE stopEvent)
    {
        int offset = 0;

        while (offset < length)
        {
            if (!WaitReady(socket, false, stopEvent))
                return false;

            int received = recv(
                socket,
                buffer + offset,
                length - offset,
                0);

            if (received == SOCKET_ERROR)
            {
                if (WSAGetLastError() == WSAEWOULDBLOCK)
                    continue;

                return false;
            }

            if (received == 0)
                return false;

            offset += received;
        }

        return true;
    }

    inline bool SendFrame(
        SOCKET socket,
        const std::string& payload,
        HANDLE stopEvent = nullptr)
    {
        if (payload.empty() ||
            payload.size() > MAX_FRAME_SIZE)
        {
            return false;
        }

        u_long networkLength =
            htonl(static_cast<u_long>(payload.size()));

        if (!SendAll(
            socket,
            reinterpret_cast<const char*>(&networkLength),
            static_cast<int>(sizeof(networkLength)),
            stopEvent))
        {
            return false;
        }

        return SendAll(
            socket,
            payload.data(),
            static_cast<int>(payload.size()),
            stopEvent);
    }

    inline bool ReceiveFrame(
        SOCKET socket,
        std::string& payload,
        HANDLE stopEvent = nullptr)
    {
        u_long networkLength = 0;

        if (!ReceiveAll(
            socket,
            reinterpret_cast<char*>(&networkLength),
            static_cast<int>(sizeof(networkLength)),
            stopEvent))
        {
            return false;
        }

        u_long length = ntohl(networkLength);

        if (length == 0 || length > MAX_FRAME_SIZE)
            return false;

        payload.resize(length);

        return ReceiveAll(
            socket,
            payload.data(),
            static_cast<int>(length),
            stopEvent);
    }
}