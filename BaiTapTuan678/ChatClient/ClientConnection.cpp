#include "pch.h"
#include "ClientConnection.h"
#include "Protocol.h"
#include "json.hpp"

#pragma comment(lib, "Ws2_32.lib")

using json = nlohmann::json;

ClientConnection& ClientConnection::Instance()
{
    static ClientConnection instance;
    return instance;
}

ClientConnection::~ClientConnection()
{
    Disconnect();
}

std::string ClientConnection::ToUtf8(const CString& wideStr)
{
    if (wideStr.IsEmpty())
        return std::string();

    int required = WideCharToMultiByte(
        CP_UTF8,
        0,
        wideStr.GetString(),
        wideStr.GetLength(),
        nullptr,
        0,
        nullptr,
        nullptr);

    if (required <= 0)
        return std::string();

    std::string utf8(required, '\0');
    WideCharToMultiByte(
        CP_UTF8,
        0,
        wideStr.GetString(),
        wideStr.GetLength(),
        utf8.data(),
        required,
        nullptr,
        nullptr);

    return utf8;
}

CString ClientConnection::FromUtf8(const std::string& utf8Str)
{
    if (utf8Str.empty())
        return CString();

    int required = MultiByteToWideChar(
        CP_UTF8,
        0,
        utf8Str.data(),
        static_cast<int>(utf8Str.size()),
        nullptr,
        0);

    if (required <= 0)
        return CString();

    CString wide;
    LPWSTR buffer = wide.GetBuffer(required);
    MultiByteToWideChar(
        CP_UTF8,
        0,
        utf8Str.data(),
        static_cast<int>(utf8Str.size()),
        buffer,
        required);
    wide.ReleaseBuffer(required);

    return wide;
}

CString ClientConnection::TranslateErrorCode(const std::string& code)
{
    if (code == "INVALID_USERNAME")
        return _T("Tên đăng nhập phải từ 3 đến 32 ký tự và chỉ chứa chữ cái, số hoặc dấu gạch dưới (_).");
    if (code == "INVALID_PASSWORD")
        return _T("Mật khẩu phải có độ dài từ 6 đến 128 ký tự.");
    if (code == "USERNAME_EXISTS")
        return _T("Tên đăng nhập này đã tồn tại. Vui lòng chọn tên khác.");
    if (code == "INVALID_CREDENTIALS")
        return _T("Tên đăng nhập hoặc mật khẩu không chính xác.");
    if (code == "ALREADY_ONLINE")
        return _T("Tài khoản này đang được đăng nhập ở một nơi khác.");
    if (code == "ALREADY_LOGGED_IN")
        return _T("Phiên hiện tại đã đăng nhập.");
    if (code == "SERVER_BUSY")
        return _T("Server đã đầy (tối đa 16 người dùng cùng lúc).");
    if (code == "NOT_AUTHENTICATED")
        return _T("Phiên đăng nhập đã hết hạn hoặc mất kết nối. Vui lòng đăng nhập lại.");
    if (code == "RECEIVER_NOT_FOUND")
        return _T("Không tìm thấy người nhận.");
    if (code == "INVALID_CONTENT")
        return _T("Nội dung tin nhắn không hợp lệ hoặc quá dài.");
    if (code == "DATABASE_ERROR")
        return _T("Lỗi cơ sở dữ liệu trên Server.");

    return _T("Lỗi từ Server: ") + FromUtf8(code);
}

bool ClientConnection::EnsureConnected(CString& errorMsg)
{
    if (m_socket != INVALID_SOCKET)
        return true;

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET)
    {
        errorMsg = _T("Không thể khởi tạo socket kết nối.");
        return false;
    }

    sockaddr_in address = {};
    address.sin_family = AF_INET;
    address.sin_port = htons(9999);
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    if (connect(
        sock,
        reinterpret_cast<sockaddr*>(&address),
        static_cast<int>(sizeof(address))) == SOCKET_ERROR)
    {
        closesocket(sock);
        errorMsg = _T("Không thể kết nối tới ChatServer (127.0.0.1:9999). Hãy kiểm tra xem Server đã chạy chưa.");
        return false;
    }

    u_long nonBlocking = 1;
    if (ioctlsocket(sock, FIONBIO, &nonBlocking) == SOCKET_ERROR)
    {
        closesocket(sock);
        errorMsg = _T("Không thể cấu hình chế độ non-blocking cho socket.");
        return false;
    }

    m_socket = sock;
    return true;
}

void ClientConnection::Disconnect()
{
    if (m_socket != INVALID_SOCKET)
    {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }

    m_userId = 0;
    m_username.Empty();
}

static bool SendAndReceiveJson(
    SOCKET& sock,
    const json& request,
    json& response,
    CString& errorMsg)
{
    std::string requestText = request.dump();

    if (!Protocol::SendFrame(sock, requestText))
    {
        closesocket(sock);
        sock = INVALID_SOCKET;
        errorMsg = _T("Mất kết nối khi gửi yêu cầu tới Server.");
        return false;
    }

    std::string responseText;
    if (!Protocol::ReceiveFrame(sock, responseText))
    {
        closesocket(sock);
        sock = INVALID_SOCKET;
        errorMsg = _T("Mất kết nối khi nhận phản hồi từ Server.");
        return false;
    }

    response = json::parse(responseText, nullptr, false);
    if (response.is_discarded() || !response.is_object())
    {
        errorMsg = _T("Dữ liệu phản hồi từ Server không đúng định dạng JSON.");
        return false;
    }

    return true;
}

bool ClientConnection::Register(
    const CString& username,
    const CString& password,
    CString& errorMsg)
{
    if (!EnsureConnected(errorMsg))
        return false;

    json request = {
        {"action", "REGISTER"},
        {"request_id", m_nextRequestId++},
        {"data", {
            {"username", ToUtf8(username)},
            {"password", ToUtf8(password)}
        }}
    };

    json response;
    if (!SendAndReceiveJson(m_socket, request, response, errorMsg))
    {
        if (!EnsureConnected(errorMsg) ||
            !SendAndReceiveJson(m_socket, request, response, errorMsg))
        {
            return false;
        }
    }

    if (!response.value("ok", false))
    {
        std::string code = "UNKNOWN_ERROR";
        if (response.contains("error") &&
            response["error"].is_object() &&
            response["error"].contains("code") &&
            response["error"]["code"].is_string())
        {
            code = response["error"]["code"].get<std::string>();
        }
        errorMsg = TranslateErrorCode(code);
        return false;
    }

    return true;
}

bool ClientConnection::Login(
    const CString& username,
    const CString& password,
    CString& errorMsg)
{
    if (!EnsureConnected(errorMsg))
        return false;

    json request = {
        {"action", "LOGIN"},
        {"request_id", m_nextRequestId++},
        {"data", {
            {"username", ToUtf8(username)},
            {"password", ToUtf8(password)}
        }}
    };

    json response;
    if (!SendAndReceiveJson(m_socket, request, response, errorMsg))
    {
        if (!EnsureConnected(errorMsg) ||
            !SendAndReceiveJson(m_socket, request, response, errorMsg))
        {
            return false;
        }
    }

    if (!response.value("ok", false))
    {
        std::string code = "UNKNOWN_ERROR";
        if (response.contains("error") &&
            response["error"].is_object() &&
            response["error"].contains("code") &&
            response["error"]["code"].is_string())
        {
            code = response["error"]["code"].get<std::string>();
        }
        errorMsg = TranslateErrorCode(code);
        return false;
    }

    if (response.contains("data") && response["data"].is_object())
    {
        const json& data = response["data"];
        m_userId = data.value("user_id", 0LL);
        m_username = FromUtf8(data.value("username", std::string()));
    }

    return m_userId != 0;
}

bool ClientConnection::Logout(CString& errorMsg)
{
    if (m_socket == INVALID_SOCKET)
    {
        m_userId = 0;
        m_username.Empty();
        return true;
    }

    json request = {
        {"action", "LOGOUT"},
        {"request_id", m_nextRequestId++},
        {"data", json::object()}
    };

    json response;
    SendAndReceiveJson(m_socket, request, response, errorMsg);

    m_userId = 0;
    m_username.Empty();
    return true;
}

bool ClientConnection::GetUsers(
    std::vector<ClientUserItem>& users,
    CString& errorMsg)
{
    users.clear();

    if (!EnsureConnected(errorMsg))
        return false;

    long long afterId = 0;

    while (true)
    {
        json request = {
            {"action", "GET_USERS"},
            {"request_id", m_nextRequestId++},
            {"data", {
                {"after_id", afterId}
            }}
        };

        json response;
        if (!SendAndReceiveJson(m_socket, request, response, errorMsg))
        {
            m_userId = 0;
            return false;
        }

        if (!response.value("ok", false))
        {
            std::string code = "UNKNOWN_ERROR";
            if (response.contains("error") &&
                response["error"].is_object() &&
                response["error"].contains("code") &&
                response["error"]["code"].is_string())
            {
                code = response["error"]["code"].get<std::string>();
            }
            errorMsg = TranslateErrorCode(code);
            return false;
        }

        if (!response.contains("data") || !response["data"].is_object())
            break;

        const json& data = response["data"];
        if (data.contains("users") && data["users"].is_array())
        {
            for (const auto& item : data["users"])
            {
                ClientUserItem user;
                user.userId = item.value("user_id", 0LL);
                user.username = FromUtf8(item.value("username", std::string()));
                user.online = item.value("online", false);
                if (user.userId > 0)
                    users.push_back(user);
            }
        }

        bool hasMore = data.value("has_more", false);
        if (!hasMore || !data.contains("next_after_id") || !data["next_after_id"].is_number_integer())
            break;

        afterId = data["next_after_id"].get<long long>();
    }

    return true;
}

bool ClientConnection::SendChatMessage(
    long long receiverId,
    const CString& content,
    ClientMessageItem& sentMessage,
    CString& errorMsg)
{
    if (!EnsureConnected(errorMsg))
        return false;

    json request = {
        {"action", "SEND_MESSAGE"},
        {"request_id", m_nextRequestId++},
        {"data", {
            {"receiver_id", receiverId},
            {"content", ToUtf8(content)}
        }}
    };

    json response;
    if (!SendAndReceiveJson(m_socket, request, response, errorMsg))
    {
        m_userId = 0;
        return false;
    }

    if (!response.value("ok", false))
    {
        std::string code = "UNKNOWN_ERROR";
        if (response.contains("error") &&
            response["error"].is_object() &&
            response["error"].contains("code") &&
            response["error"]["code"].is_string())
        {
            code = response["error"]["code"].get<std::string>();
        }
        errorMsg = TranslateErrorCode(code);
        return false;
    }

    if (response.contains("data") &&
        response["data"].is_object() &&
        response["data"].contains("message") &&
        response["data"]["message"].is_object())
    {
        const json& msg = response["data"]["message"];
        sentMessage.id = msg.value("id", 0LL);
        sentMessage.senderId = msg.value("sender_id", 0LL);
        sentMessage.receiverId = msg.value("receiver_id", 0LL);
        sentMessage.content = FromUtf8(msg.value("content", std::string()));
        sentMessage.createdAt = FromUtf8(msg.value("created_at", std::string()));
    }

    return true;
}

bool ClientConnection::GetMessages(
    long long peerId,
    long long beforeId,
    int limit,
    std::vector<ClientMessageItem>& messages,
    CString& errorMsg)
{
    messages.clear();

    if (!EnsureConnected(errorMsg))
        return false;

    json request = {
        {"action", "GET_MESSAGE"},
        {"request_id", m_nextRequestId++},
        {"data", {
            {"peer_id", peerId},
            {"before_id", beforeId},
            {"limit", limit}
        }}
    };

    json response;
    if (!SendAndReceiveJson(m_socket, request, response, errorMsg))
    {
        m_userId = 0;
        return false;
    }

    if (!response.value("ok", false))
    {
        std::string code = "UNKNOWN_ERROR";
        if (response.contains("error") &&
            response["error"].is_object() &&
            response["error"].contains("code") &&
            response["error"]["code"].is_string())
        {
            code = response["error"]["code"].get<std::string>();
        }
        errorMsg = TranslateErrorCode(code);
        return false;
    }

    if (response.contains("data") &&
        response["data"].is_object() &&
        response["data"].contains("messages") &&
        response["data"]["messages"].is_array())
    {
        for (const auto& item : response["data"]["messages"])
        {
            ClientMessageItem msg;
            msg.id = item.value("id", 0LL);
            msg.senderId = item.value("sender_id", 0LL);
            msg.receiverId = item.value("receiver_id", 0LL);
            msg.content = FromUtf8(item.value("content", std::string()));
            msg.createdAt = FromUtf8(item.value("created_at", std::string()));
            messages.push_back(msg);
        }
    }

    return true;
}
