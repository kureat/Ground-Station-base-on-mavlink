#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct WayPoint
{
    long double longitude{0.0L};
    long double latitude{0.0L};
    long double altitude{0.0L};
};

enum class PayloadEventType
{
    None,
    Heartbeat,
    BatteryInformation,
    BasicInformation,
    AttitudeInformation,
    StatusText,
    CommandFailed,
    PingReply,
};

struct PayloadResult
{
    PayloadEventType type{PayloadEventType::None};
    float value1{0.0f};
    float value2{0.0f};
    float value3{0.0f};
    float value4{0.0f};
    float value5{0.0f};
    float value6{0.0f};
    int32_t x{0};
    int32_t y{0};
    float z{0.0f};
    std::string message;
};

enum class LinkStatus
{
    Strong,
    Fair,
    Weak,
    Lost,
};

enum class BatteryStatus
{
    Normal,
    Fair,
    Weak,
};

enum class CommandId : uint16_t
{
    Heartbeat = 4000,
    Start = 4001,
    Stop = 4002,
    EmergencyStop = 4003,
    Unlock = 4004,
    Lock = 4005,
};
