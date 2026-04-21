#include "BackendAdapter.h"

#include <QDateTime>

#ifdef WIN32
#include "../../third_party/groundstation/head/ICommunication.h"
#include "../../third_party/groundstation/head/SerialCommunication.h"
#include "../../third_party/groundstation/head/UdpCommunication.h"
#else
enum class LinkStatus { Strong, Fair, Weak, Lost };
enum class BatteryStatus { Normal, Fair, Weak };
#endif

namespace
{
QString linkStatusToText(LinkStatus status)
{
    switch (status)
    {
    case LinkStatus::Strong:
        return QStringLiteral("强");
    case LinkStatus::Fair:
        return QStringLiteral("一般");
    case LinkStatus::Weak:
        return QStringLiteral("弱");
    case LinkStatus::Lost:
    default:
        return QStringLiteral("断开");
    }
}

QString batteryStatusToText(BatteryStatus status)
{
    switch (status)
    {
    case BatteryStatus::Normal:
        return QStringLiteral("正常");
    case BatteryStatus::Fair:
        return QStringLiteral("一般");
    case BatteryStatus::Weak:
    default:
        return QStringLiteral("电量低");
    }
}

QString transportToText(ConnectionSettings::Mode mode)
{
    return mode == ConnectionSettings::Mode::Serial ? QStringLiteral("YRRC 串口") : QStringLiteral("UDP/WIFI");
}
}

BackendAdapter::BackendAdapter(QObject *parent)
    : QObject(parent)
{
    m_pollTimer.setInterval(200);
    connect(&m_pollTimer, &QTimer::timeout, this, &BackendAdapter::pollBackend);
    m_snapshot.transportText = transportToText(m_settings.mode);
    m_snapshot.connectionText = QStringLiteral("未连接");
    m_snapshot.flightStateText = QStringLiteral("待机");
    m_snapshot.selfTestText = QStringLiteral("未执行");
}

BackendAdapter::~BackendAdapter()
{
    disconnectDevice();
}

bool BackendAdapter::connectDevice(const ConnectionSettings &settings)
{
    disconnectDevice();
    m_settings = settings;
    m_snapshot = TelemetrySnapshot{};
    m_snapshot.transportText = transportToText(settings.mode);
    m_snapshot.connectionText = QStringLiteral("连接中...");
    m_snapshot.flightStateText = QStringLiteral("待机");
    m_snapshot.selfTestText = QStringLiteral("开机待自检");
    m_snapshot.safetyLocked = true;

#ifdef WIN32
    if (settings.mode == ConnectionSettings::Mode::Serial)
    {
        m_comm = std::make_unique<SerialCommunication>(settings.serialPort.toStdString(), settings.baudRate);
    }
    else
    {
        m_comm = std::make_unique<UdpCommunication>(settings.ip.toStdString(), settings.port);
    }

    if (!m_comm || !m_comm->isConnected())
    {
        m_comm.reset();
        m_snapshot.connectionText = QStringLiteral("连接失败");
        emitEvent(QStringLiteral("连接失败，请检查串口号/IP 和端口。"), QStringLiteral("error"));
        emit telemetryUpdated(m_snapshot);
        emit connectionChanged(false);
        return false;
    }

    m_comm->setMavId(settings.gcsSystemId, settings.gcsComponentId, settings.targetSystemId, settings.targetComponentId);
    m_comm->startHeartbeat();
    m_comm->startReceive();
    m_pollTimer.start();
    refreshSnapshot();
    emitEvent(QStringLiteral("通信链路已建立，安全锁默认处于打开状态。"));
    requestSelfTest();
    emit connectionChanged(true);
    return true;
#else
    Q_UNUSED(settings)
    m_snapshot.connectionText = QStringLiteral("当前平台不支持" );
    emitEvent(QStringLiteral("该示例工程中的通信层当前仅实现了 Windows 版。"), QStringLiteral("warning"));
    emit telemetryUpdated(m_snapshot);
    emit connectionChanged(false);
    return false;
#endif
}

void BackendAdapter::disconnectDevice()
{
    m_pollTimer.stop();
#ifdef WIN32
    if (m_comm)
    {
        m_comm->stopReceive();
        m_comm->stopHeartbeat();
        m_comm.reset();
    }
#endif
    m_snapshot.connected = false;
    m_snapshot.connectionText = QStringLiteral("未连接");
    m_snapshot.linkStatusText = QStringLiteral("断开");
    m_snapshot.selfTestInProgress = false;
    m_snapshot.selfTestPassed = false;
    m_snapshot.selfTestText = QStringLiteral("未执行");
    m_snapshot.safetyLocked = true;
    emit telemetryUpdated(m_snapshot);
    emit connectionChanged(false);
}

bool BackendAdapter::isConnected() const
{
    return m_snapshot.connected;
}

TelemetrySnapshot BackendAdapter::snapshot() const
{
    return m_snapshot;
}

void BackendAdapter::applyMavId(quint8 gcsSystemId, quint8 gcsComponentId, quint8 targetSystemId, quint8 targetComponentId)
{
    m_settings.gcsSystemId = gcsSystemId;
    m_settings.gcsComponentId = gcsComponentId;
    m_settings.targetSystemId = targetSystemId;
    m_settings.targetComponentId = targetComponentId;

#ifdef WIN32
    if (m_comm)
    {
        m_comm->setMavId(gcsSystemId, gcsComponentId, targetSystemId, targetComponentId);
        emitEvent(QStringLiteral("MAVID 已同步到当前通信对象。"), QStringLiteral("success"));
    }
#endif
}

void BackendAdapter::emitEvent(const QString &message, const QString &level)
{
    m_snapshot.lastMessage = message;
    emit eventOccurred(message, level);
}

void BackendAdapter::finalizeSelfTest(bool passed, const QString &message)
{
    m_snapshot.selfTestInProgress = false;
    m_snapshot.selfTestPassed = passed;
    m_snapshot.selfTestText = passed ? QStringLiteral("已通过") : QStringLiteral("失败");
    emitEvent(message, passed ? QStringLiteral("success") : QStringLiteral("warning"));
}

void BackendAdapter::requestSelfTest()
{
    if (!m_comm)
    {
        emitEvent(QStringLiteral("请先建立通信连接后再自检。"), QStringLiteral("warning"));
        return;
    }

    m_snapshot.selfTestInProgress = true;
    m_snapshot.selfTestPassed = false;
    m_snapshot.selfTestText = QStringLiteral("自检中...");
    m_snapshot.safetyLocked = true;
    m_selfTestStartMs = QDateTime::currentMSecsSinceEpoch();
#ifdef WIN32
    m_comm->clearPingSequence();
    m_comm->sendPing(0);
#endif
    emit telemetryUpdated(m_snapshot);
    emitEvent(QStringLiteral("已启动链路自检：等待心跳、定位或姿态数据。"));
}

void BackendAdapter::sendStart()
{
#ifdef WIN32
    if (!m_comm)
    {
        emitEvent(QStringLiteral("未建立通信连接。"), QStringLiteral("warning"));
        return;
    }
    if (!m_snapshot.selfTestPassed)
    {
        emitEvent(QStringLiteral("启动前必须先完成自检。"), QStringLiteral("warning"));
        return;
    }
    if (m_snapshot.safetyLocked)
    {
        emitEvent(QStringLiteral("当前处于安全锁状态，请先执行“安全锁解锁”。"), QStringLiteral("warning"));
        return;
    }
    m_comm->sendStart();
    m_snapshot.flightStateText = QStringLiteral("运行中");
    emit telemetryUpdated(m_snapshot);
    emitEvent(QStringLiteral("已发送启动指令。"), QStringLiteral("success"));
#endif
}

void BackendAdapter::sendStop()
{
#ifdef WIN32
    if (!m_comm)
    {
        emitEvent(QStringLiteral("未建立通信连接。"), QStringLiteral("warning"));
        return;
    }
    m_comm->sendStop();
    m_snapshot.flightStateText = QStringLiteral("已停止");
    emit telemetryUpdated(m_snapshot);
    emitEvent(QStringLiteral("已发送停止指令。"), QStringLiteral("success"));
#endif
}

void BackendAdapter::sendEmergencyStop()
{
#ifdef WIN32
    if (!m_comm)
    {
        emitEvent(QStringLiteral("未建立通信连接。"), QStringLiteral("warning"));
        return;
    }
    m_comm->sendEmergencyStop();
    m_snapshot.flightStateText = QStringLiteral("紧急停止");
    m_snapshot.safetyLocked = true;
    emit telemetryUpdated(m_snapshot);
    emitEvent(QStringLiteral("已发送紧急停止指令。"), QStringLiteral("error"));
#endif
}

void BackendAdapter::sendLock()
{
#ifdef WIN32
    if (!m_comm)
    {
        emitEvent(QStringLiteral("未建立通信连接。"), QStringLiteral("warning"));
        return;
    }
    m_comm->sendLock();
    m_snapshot.safetyLocked = true;
    emit telemetryUpdated(m_snapshot);
    emitEvent(QStringLiteral("安全锁已打开。"), QStringLiteral("success"));
#endif
}

void BackendAdapter::sendUnlock()
{
#ifdef WIN32
    if (!m_comm)
    {
        emitEvent(QStringLiteral("未建立通信连接。"), QStringLiteral("warning"));
        return;
    }
    if (!m_snapshot.selfTestPassed)
    {
        emitEvent(QStringLiteral("请先完成自检，再执行安全锁解锁。"), QStringLiteral("warning"));
        return;
    }
    m_comm->sendUnlock();
    m_snapshot.safetyLocked = false;
    emit telemetryUpdated(m_snapshot);
    emitEvent(QStringLiteral("已发送安全锁解锁指令。"), QStringLiteral("success"));
#endif
}

void BackendAdapter::refreshSnapshot(bool emitSignal)
{
#ifdef WIN32
    if (!m_comm)
    {
        return;
    }

    m_snapshot.connected = m_comm->isConnected();
    m_snapshot.connectionText = m_snapshot.connected ? QStringLiteral("已连接") : QStringLiteral("未连接");
    m_snapshot.linkStatusText = linkStatusToText(m_comm->getLinkStatus());
    m_snapshot.batteryStatusText = batteryStatusToText(m_comm->getBatteryStatus());
    m_snapshot.batteryPercent = m_comm->getBatteryCapacity();
    m_snapshot.averageVoltage = m_comm->getAverageVoltage();
    m_snapshot.currentCurrent = m_comm->getCurrentCurrent();
    m_snapshot.batteryCount = m_comm->getBatteryNum();
    m_snapshot.longitude = m_comm->getLongitude();
    m_snapshot.latitude = m_comm->getLatitude();
    m_snapshot.relativeAltitude = m_comm->getRelativeAltitude();
    m_snapshot.absoluteAltitude = m_comm->getAbsoluteAltitude();
    m_snapshot.headingDegrees = m_comm->getHeadingDegrees();
    m_snapshot.rollDegrees = m_comm->getRollDegrees();
    m_snapshot.pitchDegrees = m_comm->getPitchDegrees();
    m_snapshot.yawDegrees = m_comm->getYawDegrees();
    m_snapshot.heartbeatElapsedMs = m_comm->getHeartbeatElapsedMs();
    m_snapshot.dropRate = m_comm->getDropRate();
#endif

    if (emitSignal)
    {
        emit telemetryUpdated(m_snapshot);
    }
}

void BackendAdapter::pollBackend()
{
#ifdef WIN32
    if (!m_comm)
    {
        return;
    }

    refreshSnapshot(false);

    const auto results = m_comm->consumeResults();
    for (const auto &item : results)
    {
        switch (item.type)
        {
        case PayloadEventType::StatusText:
            if (!item.message.empty())
            {
                emitEvent(QString::fromStdString(item.message), QStringLiteral("warning"));
            }
            break;
        case PayloadEventType::CommandFailed:
            emitEvent(QStringLiteral("命令执行失败，command=%1, result=%2")
                          .arg(item.value1)
                          .arg(item.value2),
                      QStringLiteral("error"));
            break;
        case PayloadEventType::Heartbeat:
            if (m_snapshot.flightStateText == QStringLiteral("待机") && m_snapshot.connected)
            {
                m_snapshot.flightStateText = QStringLiteral("已联机");
            }
            break;
        default:
            break;
        }
    }

    if (m_snapshot.selfTestInProgress)
    {
        const qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - m_selfTestStartMs;
        const bool heartbeatOk = m_snapshot.heartbeatElapsedMs >= 0 && m_snapshot.heartbeatElapsedMs <= 2500;
        const bool positionOk = qAbs(m_snapshot.longitude) > 0.0001 || qAbs(m_snapshot.latitude) > 0.0001;
        const bool attitudeOk = qAbs(m_snapshot.rollDegrees) > 0.01f || qAbs(m_snapshot.pitchDegrees) > 0.01f || qAbs(m_snapshot.yawDegrees) > 0.01f;
        if (heartbeatOk && (positionOk || attitudeOk || m_snapshot.averageVoltage > 0.1f))
        {
            finalizeSelfTest(true, QStringLiteral("开机自检通过，可执行安全锁解锁。"));
        }
        else if (elapsed > 5000)
        {
            finalizeSelfTest(false, QStringLiteral("自检超时：未在 5 秒内收到足够遥测。请检查飞控心跳、定位/姿态或电池包。"));
        }
    }

    emit telemetryUpdated(m_snapshot);
#else
    emit telemetryUpdated(m_snapshot);
#endif
}
