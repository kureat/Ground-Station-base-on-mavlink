#pragma once

#include <QString>
#include <QtGlobal>

struct TelemetrySnapshot
{
    bool connected{false};
    bool safetyLocked{true};
    bool selfTestPassed{false};
    bool selfTestInProgress{false};

    QString connectionText{"未连接"};
    QString transportText{"UDP/WIFI"};
    QString linkStatusText{"断开"};
    QString batteryStatusText{"未知"};
    QString flightStateText{"待机"};
    QString selfTestText{"未执行"};
    QString lastMessage;

    double longitude{0.0};
    double latitude{0.0};
    double homeLongitude{113.3535};
    double homeLatitude{23.1247};

    float batteryPercent{0.0f};
    float averageVoltage{0.0f};
    float currentCurrent{0.0f};
    short batteryCount{0};

    float relativeAltitude{0.0f};
    float absoluteAltitude{0.0f};
    float headingDegrees{0.0f};
    float rollDegrees{0.0f};
    float pitchDegrees{0.0f};
    float yawDegrees{0.0f};

    long long heartbeatElapsedMs{-1};
    float dropRate{-1.0f};
};

struct ConnectionSettings
{
    enum class Mode
    {
        Serial,
        Udp
    } mode{Mode::Udp};

    QString serialPort{"COM3"};
    int baudRate{57600};
    QString ip{"192.168.4.1"};
    quint16 port{14550};

    quint8 gcsSystemId{255};
    quint8 gcsComponentId{0};
    quint8 targetSystemId{0};
    quint8 targetComponentId{1};
};

Q_DECLARE_METATYPE(TelemetrySnapshot)
