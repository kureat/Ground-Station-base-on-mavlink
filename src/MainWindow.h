#pragma once

#include <QMainWindow>

#include "backend/BackendTypes.h"

class BackendAdapter;
class BatteryWidget;
class MapWidget;
class QLabel;
class QPushButton;
class QComboBox;
class QLineEdit;
class QListWidget;
class QFrame;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void connectOrDisconnect();
    void replaceMapImage();
    void appendEvent(const QString &message, const QString &level);
    void updateTelemetry(const TelemetrySnapshot &snapshot);
    void syncConnectionEditors();
    void configureMavId();

private:
    QWidget *createTopBar();
    QWidget *createLeftPanel();
    QWidget *createCenterPanel();
    QWidget *createRightPanel();
    QFrame *createPanelFrame();
    QPushButton *createControlButton(const QString &text, const QString &accent = QString(), int minimumHeight = 54);
    QWidget *createCommandCard(QPushButton *&button, const QString &title, const QString &description, const QString &accent = QString(), int minimumHeight = 56);
    QWidget *createMetricTile(const QString &title, QLabel *&valueLabel, const QString &initial = QStringLiteral("--"), const QString &subtitle = QString());
    void updateButtonStates(const TelemetrySnapshot &snapshot);
    void refreshMavSummary();
    void loadUiSettings();
    void saveUiSettings() const;
    ConnectionSettings currentSettingsFromUi() const;

private:
    BackendAdapter *m_backend{nullptr};
    TelemetrySnapshot m_snapshot;
    ConnectionSettings m_uiSettings;

    QComboBox *m_transportCombo{nullptr};
    QLabel *m_primaryFieldLabel{nullptr};
    QLabel *m_secondaryFieldLabel{nullptr};
    QLineEdit *m_primaryEdit{nullptr};
    QLineEdit *m_secondaryEdit{nullptr};
    QPushButton *m_connectButton{nullptr};
    QPushButton *m_replaceMapButton{nullptr};
    QPushButton *m_mavConfigButton{nullptr};

    QPushButton *m_startButton{nullptr};
    QPushButton *m_stopButton{nullptr};
    QPushButton *m_selfTestButton{nullptr};
    QPushButton *m_lockButton{nullptr};
    QPushButton *m_unlockButton{nullptr};
    QPushButton *m_emergencyButton{nullptr};

    MapWidget *m_mapWidget{nullptr};
    BatteryWidget *m_batteryWidget{nullptr};
    QListWidget *m_eventList{nullptr};

    QLabel *m_connectionBadge{nullptr};
    QLabel *m_mavSummaryLabel{nullptr};
    QLabel *m_linkValue{nullptr};
    QLabel *m_batteryStatusValue{nullptr};
    QLabel *m_batteryCapacityValue{nullptr};
    QLabel *m_batteryCountValue{nullptr};
    QLabel *m_voltageValue{nullptr};
    QLabel *m_currentValue{nullptr};
    QLabel *m_flightStateValue{nullptr};
    QLabel *m_selfTestValue{nullptr};
    QLabel *m_lockStateValue{nullptr};
    QLabel *m_heartbeatDelayValue{nullptr};
    QLabel *m_dropRateValue{nullptr};
    QLabel *m_positionValue{nullptr};
    QLabel *m_altitudeValue{nullptr};
};
