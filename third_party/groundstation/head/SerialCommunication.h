#pragma once

#include "ICommunication.h"

#include <windows.h>

class SerialCommunication : public ICommunication
{
public:
    SerialCommunication(const std::string &portName, int baudrate = 57600);
    ~SerialCommunication() override;

    bool isConnected() const override;

protected:
    bool sendMavlinkPacket(const mavlink_message_t &msg) override;
    void receiveAndParse() override;

private:
    HANDLE m_serialHandle{INVALID_HANDLE_VALUE};
    bool sendRaw(const uint8_t *buffer, uint16_t length);
    static std::string normalizePortName(const std::string &portName);
};
