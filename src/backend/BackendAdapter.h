#pragma once

#include <QObject>
#include <QTimer>

#include <memory>

#include "BackendTypes.h"

class ICommunication;

class BackendAdapter : public QObject
{
    Q_OBJECT
public:
    explicit BackendAdapter(QObject *parent = nullptr);
    ~BackendAdapter() override;

    bool connectDevice(const ConnectionSettings &settings);
    void disconnectDevice();
    bool isConnected() const;
    TelemetrySnapshot snapshot() const;
    void applyMavId(quint8 gcsSystemId, quint8 gcsComponentId, quint8 targetSystemId, quint8 targetComponentId);

public slots:
    void sendStart();
    void sendStop();
    void sendEmergencyStop();
    void sendLock();
    void sendUnlock();
    void requestSelfTest();

signals:
    void telemetryUpdated(const TelemetrySnapshot &snapshot);
    void eventOccurred(const QString &message, const QString &level);
    void connectionChanged(bool connected);

private slots:
    void pollBackend();

private:
    void emitEvent(const QString &message, const QString &level = QStringLiteral("info"));
    void refreshSnapshot(bool emitSignal = true);
    void finalizeSelfTest(bool passed, const QString &message);

private:
    std::unique_ptr<ICommunication> m_comm;
    ConnectionSettings m_settings;
    TelemetrySnapshot m_snapshot;
    QTimer m_pollTimer;
    qint64 m_selfTestStartMs{0};
};
