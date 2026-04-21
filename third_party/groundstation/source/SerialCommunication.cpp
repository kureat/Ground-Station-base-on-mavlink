#include "../head/SerialCommunication.h"

#include <iostream>

namespace
{
DWORD toWinBaud(int baudrate)
{
    return static_cast<DWORD>(baudrate);
}
}

std::string SerialCommunication::normalizePortName(const std::string &portName)
{
    if (portName.rfind("\\\\.\\", 0) == 0)
    {
        return portName;
    }
    if (portName.size() > 4 && portName.rfind("COM", 0) == 0)
    {
        return std::string("\\\\.\\") + portName;
    }
    return portName;
}

SerialCommunication::SerialCommunication(const std::string &portName, int baudrate)
{
    const auto normalized = normalizePortName(portName);
    m_serialHandle = CreateFileA(normalized.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (m_serialHandle == INVALID_HANDLE_VALUE)
    {
        return;
    }

    SetupComm(m_serialHandle, 4096, 4096);

    DCB dcb{};
    dcb.DCBlength = sizeof(dcb);
    if (!GetCommState(m_serialHandle, &dcb))
    {
        CloseHandle(m_serialHandle);
        m_serialHandle = INVALID_HANDLE_VALUE;
        return;
    }

    dcb.BaudRate = toWinBaud(baudrate);
    dcb.ByteSize = 8;
    dcb.StopBits = ONESTOPBIT;
    dcb.Parity = NOPARITY;
    dcb.fBinary = TRUE;

    if (!SetCommState(m_serialHandle, &dcb))
    {
        CloseHandle(m_serialHandle);
        m_serialHandle = INVALID_HANDLE_VALUE;
        return;
    }

    COMMTIMEOUTS timeouts{};
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 1;
    SetCommTimeouts(m_serialHandle, &timeouts);
}

SerialCommunication::~SerialCommunication()
{
    stopReceive();
    stopHeartbeat();
    if (m_serialHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_serialHandle);
        m_serialHandle = INVALID_HANDLE_VALUE;
    }
}

bool SerialCommunication::isConnected() const
{
    return m_serialHandle != INVALID_HANDLE_VALUE;
}

bool SerialCommunication::sendRaw(const uint8_t *buffer, uint16_t length)
{
    if (!isConnected())
    {
        return false;
    }
    DWORD bytesWritten = 0;
    const BOOL ok = WriteFile(m_serialHandle, buffer, length, &bytesWritten, nullptr);
    return ok && bytesWritten == length;
}

bool SerialCommunication::sendMavlinkPacket(const mavlink_message_t &msg)
{
    uint8_t buffer[MAVLINK_MAX_PACKET_LEN]{};
    const uint16_t length = mavlink_msg_to_send_buffer(buffer, &msg);
    return sendRaw(buffer, length);
}

void SerialCommunication::receiveAndParse()
{
    if (!isConnected())
    {
        return;
    }

    uint8_t buffer[1024]{};
    DWORD bytesRead = 0;
    if (!ReadFile(m_serialHandle, buffer, sizeof(buffer), &bytesRead, nullptr) || bytesRead == 0)
    {
        return;
    }

    for (DWORD i = 0; i < bytesRead; ++i)
    {
        mavlink_message_t msg{};
        if (mavlink_parse_char(MAVLINK_COMM_0, buffer[i], &msg, &m_mavStatus))
        {
            const auto result = parseMessage(msg);
            pushResult(result);
        }
    }
}
