#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <ws2tcpip.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <queue>
#include <shared_mutex>
#include <string>
#include <thread>
#include <vector>

#include "ProtocolTypes.h"
#include "../generated/include/mavlink/c_library_v2-master/all/mavlink.h"

class ICommunication
{
public:
    ICommunication();
    virtual ~ICommunication();

    virtual bool isConnected() const = 0;

    void setMavId(uint8_t gcsSystemId, uint8_t gcsComponentId, uint8_t targetSystemId, uint8_t targetComponentId);
    void addWayPoint(long double longitude, long double latitude, long double altitude);
    void clearWayPointList();
    void clearPingSequence();

    void startReceive();
    void stopReceive();
    void startHeartbeat();
    void stopHeartbeat();

    void sendHeartbeat();
    void sendStart();
    void sendStop();
    void sendEmergencyStop();
    void sendUnlock();
    void sendLock();
    void sendPing(uint32_t seq);
    void sendWayPoint(uint16_t seq, const WayPoint &wayPoint);
    void sendMissionCount(uint16_t totalCount);

    float getDropRate() const;
    long long getHeartbeatElapsedMs() const;
    bool hasRecentHeartbeat() const;
    LinkStatus getLinkStatus() const;

    float getBatteryCapacity() const;
    BatteryStatus getBatteryStatus() const;
    short getBatteryNum() const;
    float getAverageVoltage() const;
    float getCurrentCurrent() const;

    double getLongitude() const;
    double getLatitude() const;
    float getRelativeAltitude() const;
    float getAbsoluteAltitude() const;
    float getHeadingDegrees() const;
    float getRollDegrees() const;
    float getPitchDegrees() const;
    float getYawDegrees() const;

    std::vector<PayloadResult> consumeResults();

protected:
    uint8_t m_gcsSystemId{255};
    uint8_t m_gcsComponentId{0};
    uint8_t m_targetSystemId{0};
    uint8_t m_targetComponentId{1};

    mutable std::shared_mutex m_stateMutex;
    std::mutex m_queueMutex;
    std::queue<PayloadResult> m_resultQueue;
    std::vector<WayPoint> m_wayPointList;

    std::atomic<bool> m_isHeartbeatRunning{false};
    std::atomic<bool> m_isReceiveRunning{false};
    std::thread m_receiveThread;
    std::thread m_heartbeatThread;

    float m_batteryCapacity{0.0f};
    short m_batteryNum{0};
    float m_averageVoltage{0.0f};
    float m_currentCurrent{0.0f};

    double m_longitude{0.0};
    double m_latitude{0.0};
    float m_relativeAltitude{0.0f};
    float m_absoluteAltitude{0.0f};
    float m_headingDegrees{0.0f};
    float m_rollDegrees{0.0f};
    float m_pitchDegrees{0.0f};
    float m_yawDegrees{0.0f};

    uint32_t m_pingSequence{0};
    int m_connectionTimeoutMs{3000};
    std::chrono::steady_clock::time_point m_lastHeartbeat{};
    mavlink_status_t m_mavStatus{};

    virtual bool sendMavlinkPacket(const mavlink_message_t &msg) = 0;
    virtual void receiveAndParse() = 0;

    void receiveLoop();
    void heartbeatLoop();
    PayloadResult parseMessage(const mavlink_message_t &msg);
    void pushResult(const PayloadResult &result);
    void sendCommand(CommandId commandId);
};
