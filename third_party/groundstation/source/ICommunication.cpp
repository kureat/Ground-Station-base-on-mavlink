#include "../head/ICommunication.h"

#include <cmath>
#include <cstring>
#include <iostream>

namespace
{
constexpr float kRadToDeg = 57.2957795f;
}

ICommunication::ICommunication()
{
    std::memset(&m_mavStatus, 0, sizeof(m_mavStatus));
}

ICommunication::~ICommunication()
{
    stopReceive();
    stopHeartbeat();
}

void ICommunication::setMavId(uint8_t gcsSystemId, uint8_t gcsComponentId, uint8_t targetSystemId, uint8_t targetComponentId)
{
    std::unique_lock<std::shared_mutex> lock(m_stateMutex);
    m_gcsSystemId = gcsSystemId;
    m_gcsComponentId = gcsComponentId;
    m_targetSystemId = targetSystemId;
    m_targetComponentId = targetComponentId;
}

void ICommunication::addWayPoint(long double longitude, long double latitude, long double altitude)
{
    std::unique_lock<std::shared_mutex> lock(m_stateMutex);
    m_wayPointList.push_back({longitude, latitude, altitude});
}

void ICommunication::clearWayPointList()
{
    std::unique_lock<std::shared_mutex> lock(m_stateMutex);
    m_wayPointList.clear();
}

void ICommunication::clearPingSequence()
{
    std::unique_lock<std::shared_mutex> lock(m_stateMutex);
    m_pingSequence = 0;
}

void ICommunication::startReceive()
{
    stopReceive();
    m_isReceiveRunning = true;
    m_receiveThread = std::thread(&ICommunication::receiveLoop, this);
}

void ICommunication::stopReceive()
{
    m_isReceiveRunning = false;
    if (m_receiveThread.joinable())
    {
        m_receiveThread.join();
    }
}

void ICommunication::startHeartbeat()
{
    stopHeartbeat();
    m_isHeartbeatRunning = true;
    m_heartbeatThread = std::thread(&ICommunication::heartbeatLoop, this);
}

void ICommunication::stopHeartbeat()
{
    m_isHeartbeatRunning = false;
    if (m_heartbeatThread.joinable())
    {
        m_heartbeatThread.join();
    }
}

void ICommunication::receiveLoop()
{
    while (m_isReceiveRunning)
    {
        try
        {
            receiveAndParse();
        }
        catch (const std::exception &e)
        {
            pushResult({PayloadEventType::StatusText, 0, 0, 0, 0, 0, 0, 0, 0, 0, std::string("接收异常: ") + e.what()});
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

void ICommunication::heartbeatLoop()
{
    while (m_isHeartbeatRunning)
    {
        sendHeartbeat();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

void ICommunication::pushResult(const PayloadResult &result)
{
    if (result.type == PayloadEventType::None)
    {
        return;
    }
    std::lock_guard<std::mutex> guard(m_queueMutex);
    m_resultQueue.push(result);
}

std::vector<PayloadResult> ICommunication::consumeResults()
{
    std::vector<PayloadResult> items;
    std::lock_guard<std::mutex> guard(m_queueMutex);
    while (!m_resultQueue.empty())
    {
        items.push_back(m_resultQueue.front());
        m_resultQueue.pop();
    }
    return items;
}

void ICommunication::sendHeartbeat()
{
    mavlink_heartbeat_t hb{};
    hb.custom_mode = 0;
    hb.type = MAV_TYPE_GCS;
    hb.autopilot = MAV_AUTOPILOT_INVALID;
    hb.base_mode = 0;
    hb.system_status = MAV_STATE_ACTIVE;
    hb.mavlink_version = 3;

    mavlink_message_t msg{};
    mavlink_msg_heartbeat_encode(m_gcsSystemId, m_gcsComponentId, &msg, &hb);
    sendMavlinkPacket(msg);
}

void ICommunication::sendCommand(CommandId commandId)
{
    mavlink_command_int_t cmd{};
    cmd.target_system = m_targetSystemId;
    cmd.target_component = m_targetComponentId;
    cmd.command = static_cast<uint16_t>(commandId);
    cmd.frame = MAV_FRAME_GLOBAL_RELATIVE_ALT;
    cmd.current = 0;
    cmd.autocontinue = 0;

    mavlink_message_t msg{};
    mavlink_msg_command_int_encode(m_gcsSystemId, m_gcsComponentId, &msg, &cmd);
    sendMavlinkPacket(msg);
}

void ICommunication::sendStart()
{
    sendCommand(CommandId::Start);
}

void ICommunication::sendStop()
{
    sendCommand(CommandId::Stop);
}

void ICommunication::sendEmergencyStop()
{
    sendCommand(CommandId::EmergencyStop);
}

void ICommunication::sendUnlock()
{
    sendCommand(CommandId::Unlock);
}

void ICommunication::sendLock()
{
    sendCommand(CommandId::Lock);
}

void ICommunication::sendPing(uint32_t seq)
{
    mavlink_ping_t ping{};
    ping.time_usec = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
                      std::chrono::steady_clock::now().time_since_epoch())
                      .count());
    ping.seq = seq;
    ping.target_system = m_targetSystemId;
    ping.target_component = m_targetComponentId;

    mavlink_message_t msg{};
    mavlink_msg_ping_encode(m_gcsSystemId, m_gcsComponentId, &msg, &ping);
    sendMavlinkPacket(msg);
}

void ICommunication::sendMissionCount(uint16_t totalCount)
{
    mavlink_mission_count_t mc{};
    mc.count = totalCount;
    mc.target_system = m_targetSystemId;
    mc.target_component = m_targetComponentId;
    mc.mission_type = MAV_MISSION_TYPE_MISSION;

    mavlink_message_t msg{};
    mavlink_msg_mission_count_encode(m_gcsSystemId, m_gcsComponentId, &msg, &mc);
    sendMavlinkPacket(msg);
}

void ICommunication::sendWayPoint(uint16_t seq, const WayPoint &wayPoint)
{
    mavlink_mission_item_int_t item{};
    item.target_system = m_targetSystemId;
    item.target_component = m_targetComponentId;
    item.seq = seq;
    item.frame = MAV_FRAME_GLOBAL_RELATIVE_ALT;
    item.command = MAV_CMD_NAV_WAYPOINT;
    item.current = (seq == 0) ? 1 : 0;
    item.autocontinue = 1;
    item.x = static_cast<int32_t>(wayPoint.latitude * 1e7);
    item.y = static_cast<int32_t>(wayPoint.longitude * 1e7);
    item.z = static_cast<float>(wayPoint.altitude);
    item.param2 = 2.0f;

    mavlink_message_t msg{};
    mavlink_msg_mission_item_int_encode(m_gcsSystemId, m_gcsComponentId, &msg, &item);
    sendMavlinkPacket(msg);
}

PayloadResult ICommunication::parseMessage(const mavlink_message_t &msg)
{
    PayloadResult result{};

    switch (msg.msgid)
    {
    case MAVLINK_MSG_ID_HEARTBEAT:
    {
        std::unique_lock<std::shared_mutex> lock(m_stateMutex);
        m_lastHeartbeat = std::chrono::steady_clock::now();
        result.type = PayloadEventType::Heartbeat;
        result.message = "收到飞控心跳";
        break;
    }
    case MAVLINK_MSG_ID_SYS_STATUS:
    {
        mavlink_sys_status_t sys{};
        mavlink_msg_sys_status_decode(&msg, &sys);
        std::unique_lock<std::shared_mutex> lock(m_stateMutex);
        m_batteryCapacity = static_cast<float>(sys.battery_remaining);
        m_averageVoltage = static_cast<float>(sys.voltage_battery) / 1000.0f;
        m_currentCurrent = static_cast<float>(sys.current_battery) / 100.0f;
        if (m_batteryNum <= 0 && m_averageVoltage > 0.0f)
        {
            m_batteryNum = 1;
        }
        result.type = PayloadEventType::BatteryInformation;
        result.value1 = m_batteryCapacity;
        result.value2 = m_averageVoltage;
        result.value3 = m_currentCurrent;
        break;
    }
    case MAVLINK_MSG_ID_BATTERY_STATUS:
    {
        mavlink_battery_status_t status{};
        mavlink_msg_battery_status_decode(&msg, &status);
        std::unique_lock<std::shared_mutex> lock(m_stateMutex);
        m_batteryCapacity = static_cast<float>(status.battery_remaining);
        m_currentCurrent = static_cast<float>(status.current_battery) / 100.0f;
        if (status.voltages[0] != UINT16_MAX)
        {
            float totalVoltage = 0.0f;
            short cells = 0;
            for (int i = 0; i < 10; ++i)
            {
                const uint16_t voltage = status.voltages[i];
                if (voltage == UINT16_MAX)
                {
                    continue;
                }
                totalVoltage += static_cast<float>(voltage) / 1000.0f;
                ++cells;
            }
            if (cells > 0)
            {
                m_batteryNum = 1;
                m_averageVoltage = totalVoltage;
            }
        }
        result.type = PayloadEventType::BatteryInformation;
        result.value1 = m_batteryCapacity;
        result.value2 = m_averageVoltage;
        result.value3 = m_currentCurrent;
        break;
    }
    case MAVLINK_MSG_ID_GLOBAL_POSITION_INT:
    {
        mavlink_global_position_int_t pos{};
        mavlink_msg_global_position_int_decode(&msg, &pos);
        std::unique_lock<std::shared_mutex> lock(m_stateMutex);
        m_latitude = pos.lat / 10000000.0;
        m_longitude = pos.lon / 10000000.0;
        m_absoluteAltitude = pos.alt / 1000.0f;
        m_relativeAltitude = pos.relative_alt / 1000.0f;
        if (pos.hdg != UINT16_MAX)
        {
            m_headingDegrees = pos.hdg / 100.0f;
        }
        result.type = PayloadEventType::BasicInformation;
        result.value1 = static_cast<float>(m_longitude);
        result.value2 = static_cast<float>(m_latitude);
        result.value3 = m_absoluteAltitude;
        result.value4 = m_relativeAltitude;
        result.value5 = m_headingDegrees;
        break;
    }
    case MAVLINK_MSG_ID_ATTITUDE:
    {
        mavlink_attitude_t attitude{};
        mavlink_msg_attitude_decode(&msg, &attitude);
        std::unique_lock<std::shared_mutex> lock(m_stateMutex);
        m_rollDegrees = attitude.roll * kRadToDeg;
        m_pitchDegrees = attitude.pitch * kRadToDeg;
        m_yawDegrees = attitude.yaw * kRadToDeg;
        result.type = PayloadEventType::AttitudeInformation;
        result.value1 = m_rollDegrees;
        result.value2 = m_pitchDegrees;
        result.value3 = m_yawDegrees;
        break;
    }
    case MAVLINK_MSG_ID_STATUSTEXT:
    {
        mavlink_statustext_t text{};
        mavlink_msg_statustext_decode(&msg, &text);
        result.type = PayloadEventType::StatusText;
        result.message = std::string(reinterpret_cast<const char *>(text.text));
        break;
    }
    case MAVLINK_MSG_ID_COMMAND_ACK:
    {
        mavlink_command_ack_t ack{};
        mavlink_msg_command_ack_decode(&msg, &ack);
        if (ack.result != MAV_RESULT_ACCEPTED)
        {
            result.type = PayloadEventType::CommandFailed;
            result.value1 = static_cast<float>(ack.command);
            result.value2 = static_cast<float>(ack.result);
            result.message = "飞控返回命令拒绝";
        }
        break;
    }
    case MAVLINK_MSG_ID_PING:
    {
        mavlink_ping_t ping{};
        mavlink_msg_ping_decode(&msg, &ping);
        result.type = PayloadEventType::PingReply;
        result.value1 = static_cast<float>(ping.seq);
        break;
    }
    default:
        break;
    }

    return result;
}

float ICommunication::getDropRate() const
{
    const uint32_t totalPackets = m_mavStatus.packet_rx_success_count + m_mavStatus.packet_rx_drop_count;
    if (totalPackets == 0)
    {
        return 0.0f;
    }

    return static_cast<float>(m_mavStatus.packet_rx_drop_count) * 100.0f / static_cast<float>(totalPackets);
}

long long ICommunication::getHeartbeatElapsedMs() const
{
    std::shared_lock<std::shared_mutex> lock(m_stateMutex);
    if (m_lastHeartbeat == std::chrono::steady_clock::time_point{})
    {
        return -1;
    }
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - m_lastHeartbeat).count();
}

bool ICommunication::hasRecentHeartbeat() const
{
    const auto elapsed = getHeartbeatElapsedMs();
    return elapsed >= 0 && elapsed <= m_connectionTimeoutMs;
}

LinkStatus ICommunication::getLinkStatus() const
{
    const auto elapsed = getHeartbeatElapsedMs();
    const auto dropRate = getDropRate();

    if (elapsed < 0 || elapsed > 5000)
    {
        return LinkStatus::Lost;
    }
    if (elapsed > 3000 || dropRate > 10.0f)
    {
        return LinkStatus::Weak;
    }
    if (elapsed > 1500 || dropRate > 2.0f)
    {
        return LinkStatus::Fair;
    }
    return LinkStatus::Strong;
}

float ICommunication::getBatteryCapacity() const
{
    std::shared_lock<std::shared_mutex> lock(m_stateMutex);
    return m_batteryCapacity;
}

BatteryStatus ICommunication::getBatteryStatus() const
{
    const auto capacity = getBatteryCapacity();
    if (capacity >= 60.0f)
    {
        return BatteryStatus::Normal;
    }
    if (capacity >= 30.0f)
    {
        return BatteryStatus::Fair;
    }
    return BatteryStatus::Weak;
}

short ICommunication::getBatteryNum() const
{
    std::shared_lock<std::shared_mutex> lock(m_stateMutex);
    return m_batteryNum;
}

float ICommunication::getAverageVoltage() const
{
    std::shared_lock<std::shared_mutex> lock(m_stateMutex);
    return m_averageVoltage;
}

float ICommunication::getCurrentCurrent() const
{
    std::shared_lock<std::shared_mutex> lock(m_stateMutex);
    return m_currentCurrent;
}

double ICommunication::getLongitude() const
{
    std::shared_lock<std::shared_mutex> lock(m_stateMutex);
    return m_longitude;
}

double ICommunication::getLatitude() const
{
    std::shared_lock<std::shared_mutex> lock(m_stateMutex);
    return m_latitude;
}

float ICommunication::getRelativeAltitude() const
{
    std::shared_lock<std::shared_mutex> lock(m_stateMutex);
    return m_relativeAltitude;
}

float ICommunication::getAbsoluteAltitude() const
{
    std::shared_lock<std::shared_mutex> lock(m_stateMutex);
    return m_absoluteAltitude;
}

float ICommunication::getHeadingDegrees() const
{
    std::shared_lock<std::shared_mutex> lock(m_stateMutex);
    return m_headingDegrees;
}

float ICommunication::getRollDegrees() const
{
    std::shared_lock<std::shared_mutex> lock(m_stateMutex);
    return m_rollDegrees;
}

float ICommunication::getPitchDegrees() const
{
    std::shared_lock<std::shared_mutex> lock(m_stateMutex);
    return m_pitchDegrees;
}

float ICommunication::getYawDegrees() const
{
    std::shared_lock<std::shared_mutex> lock(m_stateMutex);
    return m_yawDegrees;
}
