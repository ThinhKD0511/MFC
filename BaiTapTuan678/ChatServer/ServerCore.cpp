#include "ServerCore.h"

#include <process.h>
#include <cstdio>
#include <cerrno>
#include "Protocol.h"
#include "json.hpp"
#include <string>
#include "Database.h"
#include "Password.h"


#pragma comment(lib, "Ws2_32.lib")

constexpr int MAX_CLIENTS = SessionManager::MAX_SESSIONS;

struct ClientSlot
{
    HANDLE thread = nullptr;
    SOCKET client = INVALID_SOCKET;
    HANDLE stopEvent = nullptr;
    int id = 0;

    Database* database = nullptr;
    SessionManager* sessionManager = nullptr;
};

static int WaitSocket(SOCKET socket, bool writing)
{
    fd_set socketSet;
    FD_ZERO(&socketSet);
    FD_SET(socket, &socketSet);

    timeval timeout = { 0, 200000 };

    return select(
        0,
        writing ? nullptr : &socketSet,
        writing ? &socketSet : nullptr,
        nullptr,
        &timeout);
}

static bool IsValidUsername(
    const std::string& username)
{
    if (username.size() < 3 || username.size() > 32)
        return false;

    for (char ch : username)
    {
        bool allowed =
            (ch >= 'a' && ch <= 'z') ||
            (ch >= 'A' && ch <= 'Z') ||
            (ch >= '0' && ch <= '9') ||
            ch == '_';

        if (!allowed)
            return false;
    }

    return true;
}

static bool ReadNonNegativeId(
    const nlohmann::json& value,
    sqlite3_int64& id)
{
    if (!value.is_number_integer())
        return false;

    if (value.is_number_unsigned() &&
        value.get<unsigned long long>() >
        9223372036854775807ULL)
    {
        return false;
    }

    id = value.get<sqlite3_int64>();

    return id >= 0;
}

static nlohmann::json MessageToJson(
    const MessageInfo& message)
{
    return {
        {"id", message.id},
        {"sender_id", message.senderId},
        {"receiver_id", message.receiverId},
        {"content", message.content},
        {"created_at", message.createdAt}
    };
}

static nlohmann::json ProcessRequest(
    const std::string& payload,
    Database& database,
    ClientSession& session,
    SessionManager& sessionManager,
    int clientId)
{
    using json = nlohmann::json;

    json response = {
        {"action", nullptr},
        {"request_id", nullptr},
        {"ok", false},
        {"data", nullptr},
        {"error", nullptr}
    };

    json request = json::parse(payload, nullptr, false);

    if (request.is_discarded())
    {
        response["error"] = {
            {"code", "INVALID_JSON"}
        };

        return response;
    }

    if (!request.is_object() ||
        !request.contains("action") ||
        !request["action"].is_string() ||
        !request.contains("request_id") ||
        !request["request_id"].is_number_integer())
    {
        response["error"] = {
            {"code", "INVALID_REQUEST"}
        };

        return response;
    }

    std::string action =
        request["action"].get<std::string>();

    if (action.empty() || action.size() > 32)
    {
        response["error"] = {
            {"code", "INVALID_ACTION"}
        };

        return response;
    }

    response["action"] = action;
    response["request_id"] = request["request_id"];

    if (!request.contains("data") ||
        !request["data"].is_object())
    {
        response["error"] = {
            {"code", "INVALID_DATA"}
        };

        return response;
    }

    if (action == "REGISTER")
    {
        const json& data = request["data"];

        if (!data.contains("username") ||
            !data["username"].is_string() ||
            !data.contains("password") ||
            !data["password"].is_string())
        {
            response["error"] = {
                {"code", "INVALID_DATA"}
            };

            return response;
        }

        std::string username =
            data["username"].get<std::string>();

        std::string password =
            data["password"].get<std::string>();

        if (!IsValidUsername(username))
        {
            response["error"] = {
                {"code", "INVALID_USERNAME"}
            };

            return response;
        }

        if (password.size() < 6 || password.size() > 128)
        {
            response["error"] = {
                {"code", "INVALID_PASSWORD"}
            };

            return response;
        }

        std::string passwordHash =
            HashPassword(password);

        sqlite3_int64 userId = 0;

        RegisterResult result = database.RegisterUser(
            username,
            passwordHash,
            userId);

        if (result == RegisterResult::Success)
        {
            response["ok"] = true;

            response["data"] = {
                {"user_id", userId},
                {"username", username}
            };
        }
        else if (result == RegisterResult::UsernameExists)
        {
            response["error"] = {
                {"code", "USERNAME_EXISTS"}
            };
        }
        else
        {
            response["error"] = {
                {"code", "DATABASE_ERROR"}
            };
        }

        return response;
    }

    if (action == "LOGIN")
    {
        if (session.userId != 0)
        {
            response["error"] = {
                {"code", "ALREADY_LOGGED_IN"}
            };

            return response;
        }

        const json& data = request["data"];

        if (!data.contains("username") ||
            !data["username"].is_string() ||
            !data.contains("password") ||
            !data["password"].is_string())
        {
            response["error"] = {
                {"code", "INVALID_DATA"}
            };

            return response;
        }

        std::string username =
            data["username"].get<std::string>();

        std::string password =
            data["password"].get<std::string>();

        if (!IsValidUsername(username) ||
            password.size() < 6 ||
            password.size() > 128)
        {
            response["error"] = {
                {"code", "INVALID_CREDENTIALS"}
            };

            return response;
        }

        std::string passwordHash =
            HashPassword(password);

        sqlite3_int64 userId = 0;

        AuthResult authResult = database.AuthenticateUser(
            username,
            passwordHash,
            userId);

        if (authResult != AuthResult::Success)
        {
            response["error"] = {
                {"code",
                    authResult == AuthResult::InvalidCredentials
                        ? "INVALID_CREDENTIALS"
                        : "DATABASE_ERROR"}
            };

            return response;
        }

        SessionResult sessionResult =
            sessionManager.TryLogin(clientId, userId);

        if (sessionResult != SessionResult::Success)
        {
            response["error"] = {
                {"code",
                    sessionResult == SessionResult::AlreadyOnline
                        ? "ALREADY_ONLINE"
                        : "SERVER_BUSY"}
            };

            return response;
        }

        session.userId = userId;
        session.username = username;

        response["ok"] = true;

        response["data"] = {
            {"user_id", session.userId},
            {"username", session.username}
        };

        return response;
    }

    if (action == "LOGOUT")
    {
        if (session.userId == 0)
        {
            response["error"] = {
                {"code", "NOT_AUTHENTICATED"}
            };

            return response;
        }

        sessionManager.Logout(clientId);

        session.userId = 0;
        session.username.clear();

        response["ok"] = true;
        response["data"] = json::object();

        return response;
    }

    if (action == "GET_USERS")
    {
        if (session.userId == 0)
        {
            response["error"] = {
                {"code", "NOT_AUTHENTICATED"}
            };

            return response;
        }

        const json& data = request["data"];
        sqlite3_int64 afterUserId = 0;

        if (data.contains("after_id") &&
            !ReadNonNegativeId(
                data["after_id"], afterUserId))
        {
            response["error"] = {
                {"code", "INVALID_DATA"}
            };

            return response;
        }

        std::vector<UserInfo> users;
        bool hasMore = false;

        if (!database.GetUsers(
            session.userId,
            afterUserId,
            users,
            hasMore))
        {
            response["error"] = {
                {"code", "DATABASE_ERROR"}
            };

            return response;
        }

        json userList = json::array();

        for (const UserInfo& user : users)
        {
            userList.push_back({
                {"user_id", user.userId},
                {"username", user.username},
                {"online", sessionManager.IsOnline(user.userId)}
                });
        }

        json nextAfterId = nullptr;

        if (hasMore && !users.empty())
            nextAfterId = users.back().userId;

        response["ok"] = true;

        response["data"] = {
            {"users", userList},
            {"has_more", hasMore},
            {"next_after_id", nextAfterId}
        };

        return response;
    }

    if (action == "SEND_MESSAGE")
    {
        if (session.userId == 0)
        {
            response["error"] = {
                {"code", "NOT_AUTHENTICATED"}
            };

            return response;
        }

        const json& data = request["data"];
        sqlite3_int64 receiverId = 0;

        if (!data.contains("receiver_id") ||
            !ReadNonNegativeId(
                data["receiver_id"], receiverId) ||
            receiverId == 0 ||
            receiverId == session.userId)
        {
            response["error"] = {
                {"code", "INVALID_RECEIVER"}
            };

            return response;
        }

        if (!data.contains("content") ||
            !data["content"].is_string())
        {
            response["error"] = {
                {"code", "INVALID_CONTENT"}
            };

            return response;
        }

        std::string content =
            data["content"].get<std::string>();

        if (content.empty() ||
            content.size() > 4096 ||
            content.find('\0') != std::string::npos)
        {
            response["error"] = {
                {"code", "INVALID_CONTENT"}
            };

            return response;
        }

        MessageInfo message;

        SendResult result = database.SaveMessage(
            session.userId,
            receiverId,
            content,
            message);

        if (result != SendResult::Success)
        {
            response["error"] = {
                {"code",
                    result == SendResult::ReceiverNotFound
                        ? "RECEIVER_NOT_FOUND"
                        : "DATABASE_ERROR"}
            };

            return response;
        }

        response["ok"] = true;

        response["data"] = {
            {"status", "stored"},
            {"message", MessageToJson(message)}
        };

        return response;
    }

    if (action == "GET_MESSAGE")
    {
        if (session.userId == 0)
        {
            response["error"] = {
                {"code", "NOT_AUTHENTICATED"}
            };

            return response;
        }

        const json& data = request["data"];

        sqlite3_int64 peerId = 0;
        sqlite3_int64 beforeId = 0;
        sqlite3_int64 requestedLimit = 50;

        if (!data.contains("peer_id") ||
            !ReadNonNegativeId(
                data["peer_id"], peerId) ||
            peerId == 0 ||
            peerId == session.userId)
        {
            response["error"] = {
                {"code", "INVALID_PEER"}
            };

            return response;
        }

        if (data.contains("before_id") &&
            !ReadNonNegativeId(
                data["before_id"], beforeId))
        {
            response["error"] = {
                {"code", "INVALID_DATA"}
            };

            return response;
        }

        if (data.contains("limit") &&
            (!ReadNonNegativeId(
                data["limit"], requestedLimit) ||
                requestedLimit < 1 ||
                requestedLimit > 100))
        {
            response["error"] = {
                {"code", "INVALID_DATA"}
            };

            return response;
        }

        std::vector<MessageInfo> messages;
        bool hasMore = false;

        if (!database.GetMessages(
            session.userId,
            peerId,
            beforeId,
            static_cast<int>(requestedLimit),
            messages,
            hasMore))
        {
            response["error"] = {
                {"code", "DATABASE_ERROR"}
            };

            return response;
        }

        response["ok"] = true;

        response["data"] = {
            {"messages", json::array()},
            {"has_more", false},
            {"next_before_id", nullptr}
        };

        json& messageList =
            response["data"]["messages"];

        size_t usedBytes = response.dump().size() + 32;
        sqlite3_int64 nextBeforeId = 0;

        for (const MessageInfo& message : messages)
        {
            json item = MessageToJson(message);

            size_t itemBytes = item.dump().size() + 1;

            if (usedBytes + itemBytes >
                Protocol::MAX_FRAME_SIZE)
            {
                hasMore = true;
                break;
            }

            usedBytes += itemBytes;
            messageList.push_back(item);

            nextBeforeId = message.id;
        }

        response["data"]["has_more"] = hasMore;

        if (hasMore && nextBeforeId != 0)
        {
            response["data"]["next_before_id"] =
                nextBeforeId;
        }

        return response;
    }

    if (action != "ECHO")
    {
        response["error"] = {
            {"code", "UNKNOWN_ACTION"}
        };

        return response;
    }

    const json& data = request["data"];

    if (!data.contains("content") ||
        !data["content"].is_string())
    {
        response["error"] = {
            {"code", "INVALID_CONTENT"}
        };

        return response;
    }

    const std::string& content =
        data["content"].get_ref<const std::string&>();

    if (content.size() > 4096)
    {
        response["error"] = {
            {"code", "CONTENT_TOO_LONG"}
        };

        return response;
    }

    response["ok"] = true;
    response["data"] = {
        {"content", content}
    };

    return response;
}

static unsigned __stdcall HandleClient(void* parameter)
{
    ClientSlot* slot =
        static_cast<ClientSlot*>(parameter);

    SOCKET client = slot->client;
    HANDLE stopEvent = slot->stopEvent;
    int clientId = slot->id;

    Database* database = slot->database;
    SessionManager* sessionManager = slot->sessionManager;

    ClientSession session;

    std::printf("[Client %d] Connected.\n", clientId);

    while (WaitForSingleObject(
        stopEvent, 0) == WAIT_TIMEOUT)
    {
        std::string payload;

        if (!Protocol::ReceiveFrame(
            client, payload, stopEvent))
        {
            break;
        }

        nlohmann::json response = ProcessRequest(
            payload,
            *database,
            session,
            *sessionManager,
            clientId);

        std::printf(
            "[Client %d] user_id=%lld\n",
            clientId,
            static_cast<long long>(session.userId));

        std::string responseText = response.dump();

        if (!Protocol::SendFrame(
            client, responseText, stopEvent))
        {
            break;
        }
    }

    sessionManager->Logout(clientId);

    closesocket(client);

    std::printf("[Client %d] Closed.\n", clientId);

    return 0;
}

DWORD ServerCore::Start()
{
    if (m_winsockStarted)
        return ERROR_ALREADY_INITIALIZED;

    constexpr DWORD BUFFER_SIZE = 32768;
    wchar_t programData[BUFFER_SIZE] = {};

    DWORD length = GetEnvironmentVariableW(
        L"ProgramData",
        programData,
        BUFFER_SIZE);

    if (length == 0)
    {
        return GetLastError();
    }

    if (length >= BUFFER_SIZE)
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    std::wstring databaseDir = programData;
    databaseDir += L"\\ChatFinal";
    CreateDirectoryW(databaseDir.c_str(), nullptr);

    std::wstring databasePath = databaseDir + L"\\chat.db";

    if (!m_database.Open(databasePath.c_str()))
    {
        return ERROR_OPEN_FAILED;
    }

    printf("Database ready.\n");

    WSADATA wsaData = {};

    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);

    if (result != 0)
        return result;

    m_winsockStarted = true;

    m_listenSocket = socket(
        AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (m_listenSocket == INVALID_SOCKET)
        return WSAGetLastError();

    sockaddr_in address = {};
    address.sin_family = AF_INET;
    address.sin_port = htons(9999);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    u_long nonBlocking = 1;

    if (ioctlsocket(
        m_listenSocket, FIONBIO, &nonBlocking) == SOCKET_ERROR ||
        bind(
            m_listenSocket,
            reinterpret_cast<sockaddr*>(&address),
            static_cast<int>(sizeof(address))) == SOCKET_ERROR ||
        listen(m_listenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        return WSAGetLastError();
    }

    return NO_ERROR;
}

DWORD ServerCore::Run(HANDLE stopEvent)
{
    ClientSlot clients[MAX_CLIENTS] = {};

    int nextClientId = 1;
    DWORD errorCode = NO_ERROR;

    while (WaitForSingleObject(stopEvent, 0) == WAIT_TIMEOUT)
    {
        int ready = WaitSocket(m_listenSocket, false);

        if (ready == SOCKET_ERROR)
        {
            errorCode = WSAGetLastError();
            break;
        }

        if (ready == 0)
            continue;

        if (WaitForSingleObject(stopEvent, 0) != WAIT_TIMEOUT)
            break;

        SOCKET client = accept(
            m_listenSocket, nullptr, nullptr);

        if (client == INVALID_SOCKET)
        {
            int error = WSAGetLastError();

            if (error == WSAEWOULDBLOCK ||
                error == WSAECONNRESET)
            {
                continue;
            }

            errorCode = error;
            break;
        }

        u_long nonBlocking = 1;

        if (ioctlsocket(client, FIONBIO, &nonBlocking)
            == SOCKET_ERROR)
        {
            std::printf("Cannot configure Client socket.\n");
            closesocket(client);
            continue;
        }

        ClientSlot* freeSlot = nullptr;

        for (int i = 0; i < MAX_CLIENTS; ++i)
        {
            if (clients[i].thread != nullptr)
            {
                DWORD state = WaitForSingleObject(
                    clients[i].thread, 0);

                if (state == WAIT_OBJECT_0)
                {
                    CloseHandle(clients[i].thread);

                    clients[i].thread = nullptr;
                    clients[i].client = INVALID_SOCKET;
                }
            }

            if (clients[i].thread == nullptr)
            {
                freeSlot = &clients[i];
                break;
            }
        }

        if (freeSlot == nullptr)
        {
            std::printf("Client limit reached.\n");
            closesocket(client);
            continue;
        }

        freeSlot->client = client;
        freeSlot->stopEvent = stopEvent;
        freeSlot->id = nextClientId++;
        freeSlot->database = &m_database;
        freeSlot->sessionManager = &m_sessions;

        auto threadValue = _beginthreadex(
            nullptr,
            0,
            HandleClient,
            freeSlot,
            0,
            nullptr);

        if (threadValue == 0)
        {
            std::printf(
                "Cannot create Client thread. errno: %d\n",
                errno);

            closesocket(client);
            freeSlot->client = INVALID_SOCKET;
            continue;
        }

        freeSlot->thread =
            reinterpret_cast<HANDLE>(threadValue);
    }

    SetEvent(stopEvent);

    closesocket(m_listenSocket);
    m_listenSocket = INVALID_SOCKET;

    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (clients[i].thread != nullptr)
        {
            WaitForSingleObject(
                clients[i].thread, INFINITE);

            CloseHandle(clients[i].thread);
            clients[i].thread = nullptr;
        }
    }

    return errorCode;
}

ServerCore::~ServerCore()
{
    if (m_listenSocket != INVALID_SOCKET)
        closesocket(m_listenSocket);

    if (m_winsockStarted)
        WSACleanup();
}