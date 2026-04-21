#include "../head/UdpCommunication.h"

#include <iostream>

#pragma comment(lib, "ws2_32.lib")

UdpCommunication::UdpCommunication(const std::string &ip, uint16_t port)
{
    WSADATA wsaData{};
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        return;
    }

    m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_socket == INVALID_SOCKET)
    {
        WSACleanup();
        return;
    }

    sockaddr_in localAddress{};
    localAddress.sin_family = AF_INET;
    localAddress.sin_port = htons(port);
    localAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(m_socket, reinterpret_cast<sockaddr *>(&localAddress), sizeof(localAddress)) == SOCKET_ERROR)
    {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        WSACleanup();
        return;
    }

    m_remoteAddress.sin_family = AF_INET;
    m_remoteAddress.sin_port = htons(port);
    m_remoteAddress.sin_addr.s_addr = inet_addr(ip.c_str());

    u_long nonBlocking = 1;
    ioctlsocket(m_socket, FIONBIO, &nonBlocking);
}

UdpCommunication::~UdpCommunication()
{
    stopReceive();
    stopHeartbeat();
    if (m_socket != INVALID_SOCKET)
    {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
    WSACleanup();
}

bool UdpCommunication::isConnected() const
{
    return m_socket != INVALID_SOCKET;
}

std::string UdpCommunication::ip() const
{
    return inet_ntoa(m_remoteAddress.sin_addr);
}

uint16_t UdpCommunication::port() const
{
    return ntohs(m_remoteAddress.sin_port);
}

bool UdpCommunication::sendRaw(const uint8_t *buffer, uint16_t length)
{
    if (!isConnected())
    {
        return false;
    }
    return sendto(m_socket, reinterpret_cast<const char *>(buffer), length, 0,
                  reinterpret_cast<const sockaddr *>(&m_remoteAddress), sizeof(m_remoteAddress)) > 0;
}

bool UdpCommunication::sendMavlinkPacket(const mavlink_message_t &msg)
{
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN]{};
    const uint16_t length = mavlink_msg_to_send_buffer(buffer, &msg);
    return sendRaw(buffer, length);
}

void UdpCommunication::receiveAndParse()
{
    if (!isConnected())
    {
        return;
    }

    char buffer[2048]{};
    int addressLength = sizeof(m_remoteAddress);
    const int bytesReceived = recvfrom(m_socket, buffer, sizeof(buffer), 0,
                                       reinterpret_cast<sockaddr *>(&m_remoteAddress), &addressLength);
    if (bytesReceived <= 0)
    {
        const int errorCode = WSAGetLastError();
        if (errorCode != WSAEWOULDBLOCK)
        {
            std::cerr << "recvfrom error: " << errorCode << std::endl;
        }
        return;
    }

    for (int i = 0; i < bytesReceived; ++i)
    {
        mavlink_message_t msg{};
        if (mavlink_parse_char(MAVLINK_COMM_0, static_cast<uint8_t>(buffer[i]), &msg, &m_mavStatus))
        {
            const auto result = parseMessage(msg);
            pushResult(result);
        }
    }
}
