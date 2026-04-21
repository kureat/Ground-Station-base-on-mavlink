#pragma once

#include "ICommunication.h"

class UdpCommunication : public ICommunication
{
public:
    UdpCommunication(const std::string &ip, uint16_t port);
    ~UdpCommunication() override;

    bool isConnected() const override;
    std::string ip() const;
    uint16_t port() const;

protected:
    bool sendMavlinkPacket(const mavlink_message_t &msg) override;
    void receiveAndParse() override;

private:
    SOCKET m_socket{INVALID_SOCKET};
    sockaddr_in m_remoteAddress{};
    bool sendRaw(const uint8_t *buffer, uint16_t length);
};
