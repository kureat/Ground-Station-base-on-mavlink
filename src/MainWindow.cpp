#include "MainWindow.h"

#include <QApplication>
#include <QComboBox>
#include <QDateTime>
#include <QDialog>
#include <QFileDialog>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QSpinBox>
#include <QVBoxLayout>

#include "backend/BackendAdapter.h"
#include "widgets/BatteryWidget.h"
#include "widgets/MapWidget.h"

namespace
{
QString badgeStyle(const QString &background, const QString &foreground = QStringLiteral("#F8FAFC"))
{
    return QStringLiteral("QLabel{background:%1;color:%2;border-radius:13px;padding:7px 14px;font-size:13px;font-weight:800;}")
        .arg(background, foreground);
}

QString titleStyle()
{
    return QStringLiteral("font-size:22px;font-weight:800;color:#F8FAFC;");
}

QString subtleStyle()
{
    return QStringLiteral("color:#94A3B8;font-size:11px;line-height:1.25;background:transparent;");
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_backend(new BackendAdapter(this))
{
    setWindowTitle(QStringLiteral("NexusPilot - 抛秧装置地面站"));
    resize(1650, 980);
    setMinimumSize(1460, 900);

    loadUiSettings();

    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(18, 16, 18, 16);
    root->setSpacing(14);
    root->addWidget(createTopBar());

    auto *body = new QHBoxLayout();
    body->setSpacing(14);

    auto *leftPanel = createLeftPanel();
    auto *centerPanel = createCenterPanel();
    auto *rightPanel = createRightPanel();

    auto *rightScroll = new QScrollArea(this);
    rightScroll->setWidget(rightPanel);
    rightScroll->setWidgetResizable(true);
    rightScroll->setFrameShape(QFrame::NoFrame);
    rightScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    rightScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    rightScroll->setMinimumWidth(430);
    rightScroll->setStyleSheet(QStringLiteral(
        "QScrollArea{background:transparent;border:none;}"
        "QScrollArea > QWidget > QWidget{background:transparent;}"
        "QScrollBar:vertical{background:#0B1220;width:10px;margin:4px 0 4px 0;border-radius:5px;}"
        "QScrollBar::handle:vertical{background:#334155;min-height:28px;border-radius:5px;}"
        "QScrollBar::handle:vertical:hover{background:#475569;}"
        "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0px;}"
        "QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical{background:transparent;}"));

    body->addWidget(leftPanel, 0);
    body->addWidget(centerPanel, 1);
    body->addWidget(rightScroll, 0);
    root->addLayout(body, 1);

    central->setStyleSheet(QStringLiteral(
        "QWidget{background:#08111F;color:#E5EEF9;font-family:'Microsoft YaHei UI','Segoe UI';}"
        "QFrame#panelFrame{background:#0F172A;border:1px solid #1E293B;border-radius:24px;}"
        "QPushButton{background:#172033;border:1px solid #334155;border-radius:14px;padding:10px 14px;color:#E2E8F0;font-weight:700;}"
        "QPushButton:hover{border-color:#60A5FA;background:#1A2740;}"
        "QPushButton:disabled{color:#64748B;border-color:#1F2937;background:#0B1220;}"
        "QLineEdit,QComboBox,QSpinBox{background:#111827;border:1px solid #334155;border-radius:12px;padding:9px 12px;min-height:22px;}"
        "QListWidget{background:#0B1220;border:1px solid #1E293B;border-radius:16px;padding:8px;}"
        "QScrollBar:vertical{background:#0F172A;width:10px;margin:8px 2px;border-radius:5px;}"
        "QScrollBar::handle:vertical{background:#334155;border-radius:5px;min-height:24px;}"
        "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0;}"));

    connect(m_connectButton, &QPushButton::clicked, this, &MainWindow::connectOrDisconnect);
    connect(m_replaceMapButton, &QPushButton::clicked, this, &MainWindow::replaceMapImage);
    connect(m_transportCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::syncConnectionEditors);
    connect(m_mavConfigButton, &QPushButton::clicked, this, &MainWindow::configureMavId);

    connect(m_startButton, &QPushButton::clicked, m_backend, &BackendAdapter::sendStart);
    connect(m_stopButton, &QPushButton::clicked, m_backend, &BackendAdapter::sendStop);
    connect(m_emergencyButton, &QPushButton::clicked, m_backend, &BackendAdapter::sendEmergencyStop);
    connect(m_lockButton, &QPushButton::clicked, m_backend, &BackendAdapter::sendLock);
    connect(m_unlockButton, &QPushButton::clicked, m_backend, &BackendAdapter::sendUnlock);
    connect(m_selfTestButton, &QPushButton::clicked, m_backend, &BackendAdapter::requestSelfTest);

    connect(m_backend, &BackendAdapter::telemetryUpdated, this, &MainWindow::updateTelemetry);
    connect(m_backend, &BackendAdapter::eventOccurred, this, &MainWindow::appendEvent);

    m_connectButton->setToolTip(QStringLiteral("串口: new SerialCommunication(COM, baud)；WiFi: new UdpCommunication(ip, port)"));
    m_mavConfigButton->setToolTip(QStringLiteral("setMAVID"));
    m_emergencyButton->setToolTip(QStringLiteral("send_emergency_stop"));
    if (m_linkValue) m_linkValue->setToolTip(QStringLiteral("get_link_status"));
    if (m_batteryStatusValue) m_batteryStatusValue->setToolTip(QStringLiteral("get_battery_status"));
    if (m_batteryCapacityValue) m_batteryCapacityValue->setToolTip(QStringLiteral("get_battery_capacity"));
    if (m_batteryCountValue) m_batteryCountValue->setToolTip(QStringLiteral("get_battery_num"));
    if (m_voltageValue) m_voltageValue->setToolTip(QStringLiteral("get_battery_voltage"));
    if (m_currentValue) m_currentValue->setToolTip(QStringLiteral("get_current_current"));
    if (m_positionValue) m_positionValue->setToolTip(QStringLiteral("get_longitude / get_latitude"));
    if (m_altitudeValue) m_altitudeValue->setToolTip(QStringLiteral("get_absolute_altitude / get_relative_altitude"));
    if (m_heartbeatDelayValue) m_heartbeatDelayValue->setToolTip(QStringLiteral("getHeartbeatElapsed"));
    if (m_dropRateValue) m_dropRateValue->setToolTip(QStringLiteral("getDropRate"));

    m_transportCombo->setCurrentIndex(m_uiSettings.mode == ConnectionSettings::Mode::Serial ? 0 : 1);
    syncConnectionEditors();
    refreshMavSummary();

    QSettings settings(QStringLiteral("OpenAI"), QStringLiteral("NexusPilotQt"));
    const QString mapPath = settings.value(QStringLiteral("map/path")).toString();
    if (!mapPath.isEmpty())
    {
        m_mapWidget->setMapImage(mapPath);
    }
    m_mapWidget->setGeoBounds(113.3475, 113.3597, 23.1188, 23.1304);

    appendEvent(QStringLiteral("界面已更新为精简版。"), QStringLiteral("info"));
    appendEvent(QStringLiteral("默认上锁，连接后自动自检。"), QStringLiteral("info"));
    updateButtonStates(m_snapshot);
    updateTelemetry(m_snapshot);
}

QWidget *MainWindow::createTopBar()
{
    auto *frame = createPanelFrame();
    frame->setFixedHeight(136);

    auto *layout = new QHBoxLayout(frame);
    layout->setContentsMargins(18, 14, 18, 14);
    layout->setSpacing(14);

    auto *brandBox = new QVBoxLayout();
    brandBox->setSpacing(2);
    auto *brand = new QLabel(QStringLiteral("NexusPilot"), frame);
    brand->setStyleSheet(QStringLiteral("font-size:24px;font-weight:900;color:#7DB3FF;"));
    auto *subBrand = new QLabel(QStringLiteral("抛秧装置地面站"), frame);
    subBrand->setStyleSheet(subtleStyle());
    brandBox->addWidget(brand);
    brandBox->addWidget(subBrand);
    brandBox->addStretch(1);

    m_connectionBadge = new QLabel(QStringLiteral("未连接"), frame);
    m_connectionBadge->setStyleSheet(badgeStyle(QStringLiteral("#334155")));
    m_connectionBadge->setFixedHeight(32);

    auto *connectScroll = new QScrollArea(frame);
    connectScroll->setFrameShape(QFrame::NoFrame);
    connectScroll->setWidgetResizable(true);
    connectScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    connectScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    connectScroll->setMinimumWidth(700);
    connectScroll->setStyleSheet(QStringLiteral(
        "QScrollArea{background:#0B1220;border:1px solid #1E293B;border-radius:18px;}"
        "QScrollArea > QWidget > QWidget{background:#0B1220;}"
        "QScrollBar:vertical{background:#0B1220;width:8px;margin:6px 4px 6px 0;border-radius:4px;}"
        "QScrollBar::handle:vertical{background:#334155;min-height:24px;border-radius:4px;}"
        "QScrollBar::handle:vertical:hover{background:#475569;}"
        "QScrollBar::add-line:vertical,QScrollBar::sub-line:vertical{height:0px;}"
        "QScrollBar::add-page:vertical,QScrollBar::sub-page:vertical{background:transparent;}"));

    auto *connectCard = new QWidget(connectScroll);
    connectCard->setMinimumHeight(128);
    auto *connectLayout = new QVBoxLayout(connectCard);
    connectLayout->setContentsMargins(16, 12, 16, 12);
    connectLayout->setSpacing(8);

    auto *connectTitle = new QLabel(QStringLiteral("设备连接"), connectCard);
    connectTitle->setStyleSheet(QStringLiteral("font-size:16px;font-weight:800;color:#F8FAFC;"));
    auto *connectHint = new QLabel(QStringLiteral("串口 / WiFi，可滚轮查看"), connectCard);
    connectHint->setWordWrap(false);
    connectHint->setMaximumHeight(18);
    connectHint->setStyleSheet(subtleStyle());

    auto *form = new QGridLayout();
    form->setHorizontalSpacing(12);
    form->setVerticalSpacing(6);
    form->setColumnStretch(0, 2);
    form->setColumnStretch(1, 3);
    form->setColumnStretch(2, 2);
    form->setColumnStretch(3, 0);

    auto *transportLabel = new QLabel(QStringLiteral("连接方式"), connectCard);
    transportLabel->setStyleSheet(subtleStyle());
    m_primaryFieldLabel = new QLabel(connectCard);
    m_primaryFieldLabel->setStyleSheet(subtleStyle());
    m_secondaryFieldLabel = new QLabel(connectCard);
    m_secondaryFieldLabel->setStyleSheet(subtleStyle());

    m_transportCombo = new QComboBox(connectCard);
    m_transportCombo->addItems({QStringLiteral("数传串口"), QStringLiteral("WiFi / UDP")});
    m_transportCombo->setFixedHeight(38);
    m_transportCombo->setMinimumWidth(150);

    m_primaryEdit = new QLineEdit(connectCard);
    m_primaryEdit->setFixedHeight(38);
    m_primaryEdit->setClearButtonEnabled(true);

    m_secondaryEdit = new QLineEdit(connectCard);
    m_secondaryEdit->setFixedHeight(38);
    m_secondaryEdit->setClearButtonEnabled(true);

    m_connectButton = new QPushButton(QStringLiteral("连接设备"), connectCard);
    m_connectButton->setFixedSize(112, 38);
    m_connectButton->setStyleSheet(QStringLiteral(
        "QPushButton{background:#2563EB;border:1px solid #3B82F6;border-radius:12px;padding:6px 14px;color:white;font-weight:900;}"
        "QPushButton:hover{background:#1D4ED8;border-color:#60A5FA;}"));

    form->addWidget(transportLabel, 0, 0);
    form->addWidget(m_primaryFieldLabel, 0, 1);
    form->addWidget(m_secondaryFieldLabel, 0, 2);
    form->addWidget(m_transportCombo, 1, 0);
    form->addWidget(m_primaryEdit, 1, 1);
    form->addWidget(m_secondaryEdit, 1, 2);
    form->addWidget(m_connectButton, 1, 3, Qt::AlignBottom);

    connectLayout->addWidget(connectTitle);
    connectLayout->addWidget(connectHint);
    connectLayout->addLayout(form);
    connectScroll->setWidget(connectCard);

    auto *mavCard = createPanelFrame();
    mavCard->setFixedWidth(280);
    mavCard->setStyleSheet(QStringLiteral("QFrame#panelFrame{background:#0B1220;border:1px solid #1E293B;border-radius:18px;}"));
    auto *mavLayout = new QVBoxLayout(mavCard);
    mavLayout->setContentsMargins(16, 14, 16, 14);
    mavLayout->setSpacing(8);
    auto *mavTitle = new QLabel(QStringLiteral("无人机配置"), mavCard);
    mavTitle->setStyleSheet(QStringLiteral("font-size:16px;font-weight:800;color:#F8FAFC;"));
    auto *mavHint = new QLabel(QStringLiteral("设置 MAVID"), mavCard);
    mavHint->setWordWrap(false);
    mavHint->setMaximumHeight(18);
    mavHint->setStyleSheet(subtleStyle());
    m_mavSummaryLabel = new QLabel(mavCard);
    m_mavSummaryLabel->setWordWrap(false);
    m_mavSummaryLabel->setStyleSheet(QStringLiteral("color:#D7E5FF;font-size:12px;font-weight:700;background:transparent;border:none;padding:0px;"));
    m_mavConfigButton = new QPushButton(QStringLiteral("设置 MAVID"), mavCard);
    m_mavConfigButton->setFixedHeight(38);
    mavLayout->addWidget(mavTitle);
    mavLayout->addWidget(mavHint);
    mavLayout->addWidget(m_mavSummaryLabel);
    mavLayout->addStretch(1);
    mavLayout->addWidget(m_mavConfigButton, 0, Qt::AlignLeft);

    layout->addLayout(brandBox);
    layout->addWidget(m_connectionBadge, 0, Qt::AlignTop);
    layout->addWidget(connectScroll, 1);
    layout->addWidget(mavCard);

    return frame;
}

QFrame *MainWindow::createPanelFrame()
{
    auto *frame = new QFrame(this);
    frame->setObjectName(QStringLiteral("panelFrame"));
    return frame;
}

QPushButton *MainWindow::createControlButton(const QString &text, const QString &accent, int minimumHeight)
{
    auto *button = new QPushButton(text, this);
    button->setMinimumHeight(minimumHeight);
    if (!accent.isEmpty())
    {
        button->setStyleSheet(QStringLiteral(
            "QPushButton{background:%1;border:1px solid %2;border-radius:14px;padding:10px 12px;color:white;font-size:15px;font-weight:900;}"
            "QPushButton:hover{background:%3;border-color:#E2E8F0;}"
            "QPushButton:disabled{background:#1E293B;border-color:#334155;color:#64748B;}")
                                  .arg(accent, accent, accent));
    }
    return button;
}

QWidget *MainWindow::createCommandCard(QPushButton *&button, const QString &title, const QString &description, const QString &accent, int minimumHeight)
{
    auto *card = createPanelFrame();
    card->setStyleSheet(QStringLiteral("QFrame#panelFrame{background:#111827;border:1px solid #1F2937;border-radius:18px;}"));

    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    button = createControlButton(title, accent, minimumHeight);
    auto *desc = new QLabel(description, card);
    desc->setWordWrap(true);
    desc->setStyleSheet(QStringLiteral("color:#94A3B8;font-size:11px;background:transparent;"));

    layout->addWidget(button);
    layout->addWidget(desc);
    return card;
}

QWidget *MainWindow::createMetricTile(const QString &title, QLabel *&valueLabel, const QString &initial, const QString &subtitle)
{
    auto *frame = createPanelFrame();
    frame->setStyleSheet(QStringLiteral("QFrame#panelFrame{background:#111827;border:1px solid #1F2937;border-radius:16px;}"));
    auto *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(14, 12, 14, 12);
    layout->setSpacing(4);

    auto *titleLabel = new QLabel(title, frame);
    titleLabel->setStyleSheet(QStringLiteral("color:#94A3B8;font-size:12px;font-weight:700;"));
    valueLabel = new QLabel(initial, frame);
    valueLabel->setStyleSheet(QStringLiteral("color:#F8FAFC;font-size:20px;font-weight:800;"));
    valueLabel->setWordWrap(true);

    layout->addWidget(titleLabel);
    layout->addWidget(valueLabel);
    if (!subtitle.isEmpty())
    {
        auto *sub = new QLabel(subtitle, frame);
        sub->setWordWrap(true);
        sub->setStyleSheet(subtleStyle());
        layout->addWidget(sub);
    }
    return frame;
}

QWidget *MainWindow::createLeftPanel()
{
    auto *frame = createPanelFrame();
    frame->setFixedWidth(360);

    auto *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(14);

    auto *title = new QLabel(QStringLiteral("飞控控制"), frame);
    title->setStyleSheet(titleStyle());
    auto *hint = new QLabel(QStringLiteral("保留 5 个飞控指令和紧急停止。"), frame);
    hint->setWordWrap(true);
    hint->setStyleSheet(subtleStyle());
    layout->addWidget(title);
    layout->addWidget(hint);

    auto *grid = new QGridLayout();
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(10);
    grid->addWidget(createCommandCard(m_startButton, QStringLiteral("启动"), QStringLiteral("开始作业"), QStringLiteral("#2563EB")), 0, 0);
    grid->addWidget(createCommandCard(m_stopButton, QStringLiteral("停止"), QStringLiteral("正常停止")), 0, 1);
    grid->addWidget(createCommandCard(m_selfTestButton, QStringLiteral("自检"), QStringLiteral("执行设备自检"), QStringLiteral("#0EA5E9")), 1, 0);
    grid->addWidget(createCommandCard(m_lockButton, QStringLiteral("安全锁"), QStringLiteral("执行上锁")), 1, 1);
    grid->addWidget(createCommandCard(m_unlockButton, QStringLiteral("安全锁解锁"), QStringLiteral("解锁后可启动"), QStringLiteral("#16A34A")), 2, 0, 1, 2);
    layout->addLayout(grid);

    auto *emergencyCard = createPanelFrame();
    emergencyCard->setStyleSheet(QStringLiteral("QFrame#panelFrame{background:#111827;border:1px solid #7F1D1D;border-radius:18px;}"));
    auto *emergencyLayout = new QVBoxLayout(emergencyCard);
    emergencyLayout->setContentsMargins(14, 14, 14, 14);
    emergencyLayout->setSpacing(8);
    m_emergencyButton = createControlButton(QStringLiteral("紧急停止"), QStringLiteral("#DC2626"), 66);
    auto *emergencyText = new QLabel(QStringLiteral("对接 send_emergency_stop"), emergencyCard);
    emergencyText->setWordWrap(true);
    emergencyText->setStyleSheet(QStringLiteral("color:#FECACA;font-size:12px;"));
    emergencyLayout->addWidget(m_emergencyButton);
    emergencyLayout->addWidget(emergencyText);
    layout->addWidget(emergencyCard);

    auto *stateCard = createPanelFrame();
    stateCard->setStyleSheet(QStringLiteral("QFrame#panelFrame{background:#0B1220;border:1px solid #1E293B;border-radius:18px;}"));
    auto *stateLayout = new QVBoxLayout(stateCard);
    stateLayout->setContentsMargins(16, 14, 16, 14);
    stateLayout->setSpacing(10);
    auto *stateTitle = new QLabel(QStringLiteral("操作顺序"), stateCard);
    stateTitle->setStyleSheet(QStringLiteral("font-size:16px;font-weight:800;"));
    auto *stateText = new QLabel(QStringLiteral("连接 → 自检 → 解锁 → 启动\n异常时点击“紧急停止”"), stateCard);
    stateText->setStyleSheet(QStringLiteral("color:#CBD5E1;font-size:13px;line-height:1.5;"));
    stateText->setWordWrap(true);
    stateLayout->addWidget(stateTitle);
    stateLayout->addWidget(stateText);
    layout->addWidget(stateCard);
    layout->addStretch(1);

    return frame;
}

QWidget *MainWindow::createCenterPanel()
{
    auto *frame = createPanelFrame();
    auto *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(12);

    auto *header = new QHBoxLayout();
    header->setSpacing(10);

    auto *titleWrap = new QVBoxLayout();
    titleWrap->setSpacing(4);
    auto *title = new QLabel(QStringLiteral("地图视图"), frame);
    title->setStyleSheet(titleStyle());
    auto *sub = new QLabel(QStringLiteral("支持导入本地图片作为地图。"), frame);
    sub->setStyleSheet(subtleStyle());
    sub->setWordWrap(true);
    titleWrap->addWidget(title);
    titleWrap->addWidget(sub);

    m_replaceMapButton = new QPushButton(QStringLiteral("导入地图"), frame);
    m_replaceMapButton->setMinimumWidth(116);
    m_replaceMapButton->setStyleSheet(QStringLiteral("QPushButton{background:#172554;border:1px solid #3B82F6;border-radius:14px;padding:10px 16px;color:#DBEAFE;font-weight:800;}QPushButton:hover{background:#1D4ED8;color:white;}"));

    header->addLayout(titleWrap, 1);
    header->addWidget(m_replaceMapButton, 0, Qt::AlignTop);

    m_mapWidget = new MapWidget(frame);

    layout->addLayout(header);
    layout->addWidget(m_mapWidget, 1);
    return frame;
}

QWidget *MainWindow::createRightPanel()
{
    auto *frame = createPanelFrame();
    frame->setFixedWidth(430);

    auto *layout = new QVBoxLayout(frame);
    layout->setContentsMargins(18, 18, 18, 18);
    layout->setSpacing(14);

    auto *title = new QLabel(QStringLiteral("状态面板"), frame);
    title->setStyleSheet(titleStyle());
    auto *sub = new QLabel(QStringLiteral("显示链路、电池、经纬度、海拔。"), frame);
    sub->setWordWrap(true);
    sub->setStyleSheet(subtleStyle());
    layout->addWidget(title);
    layout->addWidget(sub);

    auto *deviceCard = createPanelFrame();
    auto *deviceLayout = new QGridLayout(deviceCard);
    deviceLayout->setContentsMargins(16, 16, 16, 16);
    deviceLayout->setHorizontalSpacing(10);
    deviceLayout->setVerticalSpacing(10);
    auto *deviceTitle = new QLabel(QStringLiteral("设备状态"), deviceCard);
    deviceTitle->setStyleSheet(QStringLiteral("font-size:17px;font-weight:800;"));
    deviceLayout->addWidget(deviceTitle, 0, 0, 1, 2);
    deviceLayout->addWidget(createMetricTile(QStringLiteral("链路状态"), m_linkValue, QStringLiteral("断开")), 1, 0, 1, 2);
    deviceLayout->addWidget(createMetricTile(QStringLiteral("作业状态"), m_flightStateValue, QStringLiteral("待机")), 2, 0);
    deviceLayout->addWidget(createMetricTile(QStringLiteral("自检状态"), m_selfTestValue, QStringLiteral("未执行")), 2, 1);
    deviceLayout->addWidget(createMetricTile(QStringLiteral("安全锁状态"), m_lockStateValue, QStringLiteral("已打开")), 3, 0, 1, 2);
    deviceLayout->addWidget(createMetricTile(QStringLiteral("心跳延迟"), m_heartbeatDelayValue, QStringLiteral("-- ms")), 4, 0);
    deviceLayout->addWidget(createMetricTile(QStringLiteral("丢包率"), m_dropRateValue, QStringLiteral("-- %")), 4, 1);
    layout->addWidget(deviceCard);

    auto *batteryCard = createPanelFrame();
    auto *batteryLayout = new QGridLayout(batteryCard);
    batteryLayout->setContentsMargins(16, 16, 16, 16);
    batteryLayout->setHorizontalSpacing(10);
    batteryLayout->setVerticalSpacing(10);
    auto *batteryTitle = new QLabel(QStringLiteral("电池状态"), batteryCard);
    batteryTitle->setStyleSheet(QStringLiteral("font-size:17px;font-weight:800;"));
    batteryLayout->addWidget(batteryTitle, 0, 0, 1, 2);
    m_batteryWidget = new BatteryWidget(batteryCard);
    batteryLayout->addWidget(m_batteryWidget, 1, 0, 1, 2);
    batteryLayout->addWidget(createMetricTile(QStringLiteral("电池状态"), m_batteryStatusValue, QStringLiteral("未知")), 2, 0, 1, 2);
    batteryLayout->addWidget(createMetricTile(QStringLiteral("当前电量"), m_batteryCapacityValue, QStringLiteral("0%")), 3, 0);
    batteryLayout->addWidget(createMetricTile(QStringLiteral("电池数量"), m_batteryCountValue, QStringLiteral("0 块")), 3, 1);
    batteryLayout->addWidget(createMetricTile(QStringLiteral("平均电压"), m_voltageValue, QStringLiteral("0.00 V")), 4, 0);
    batteryLayout->addWidget(createMetricTile(QStringLiteral("当前电流"), m_currentValue, QStringLiteral("0.00 A")), 4, 1);
    layout->addWidget(batteryCard);

    auto *positionCard = createPanelFrame();
    auto *positionLayout = new QGridLayout(positionCard);
    positionLayout->setContentsMargins(16, 16, 16, 16);
    positionLayout->setHorizontalSpacing(10);
    positionLayout->setVerticalSpacing(10);
    auto *positionTitle = new QLabel(QStringLiteral("当前状态"), positionCard);
    positionTitle->setStyleSheet(QStringLiteral("font-size:17px;font-weight:800;"));
    positionLayout->addWidget(positionTitle, 0, 0, 1, 2);
    positionLayout->addWidget(createMetricTile(QStringLiteral("经纬度"), m_positionValue, QStringLiteral("--\n--")), 1, 0, 1, 2);
    positionLayout->addWidget(createMetricTile(QStringLiteral("海拔"), m_altitudeValue, QStringLiteral("绝对 0.0 m\n相对 0.0 m")), 2, 0, 1, 2);
    layout->addWidget(positionCard);

    auto *logCard = createPanelFrame();
    auto *logLayout = new QVBoxLayout(logCard);
    logLayout->setContentsMargins(16, 16, 16, 16);
    logLayout->setSpacing(10);

    auto *logTitle = new QLabel(QStringLiteral("状态文本 / 指令反馈"), logCard);
    logTitle->setStyleSheet(QStringLiteral("font-size:17px;font-weight:800;"));
    auto *logHint = new QLabel(QStringLiteral("显示状态文本与指令失败原因，可滚动查看历史记录。"), logCard);
    logHint->setWordWrap(true);
    logHint->setStyleSheet(subtleStyle());

    m_eventList = new QListWidget(logCard);
    m_eventList->setMinimumHeight(220);
    m_eventList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    logLayout->addWidget(logTitle);
    logLayout->addWidget(logHint);
    logLayout->addWidget(m_eventList, 1);
    layout->addWidget(logCard, 1);

    return frame;
}

void MainWindow::loadUiSettings()
{
    QSettings settings(QStringLiteral("OpenAI"), QStringLiteral("NexusPilotQt"));
    m_uiSettings.mode = settings.value(QStringLiteral("conn/mode"), 0).toInt() == 0 ? ConnectionSettings::Mode::Serial : ConnectionSettings::Mode::Udp;
    m_uiSettings.serialPort = settings.value(QStringLiteral("conn/serialPort"), QStringLiteral("COM3")).toString();
    m_uiSettings.baudRate = settings.value(QStringLiteral("conn/baudRate"), 57600).toInt();
    m_uiSettings.ip = settings.value(QStringLiteral("conn/ip"), QStringLiteral("192.168.4.1")).toString();
    m_uiSettings.port = static_cast<quint16>(settings.value(QStringLiteral("conn/port"), 14550).toUInt());
    m_uiSettings.gcsSystemId = static_cast<quint8>(settings.value(QStringLiteral("mav/gcsSystemId"), 255).toUInt());
    m_uiSettings.gcsComponentId = static_cast<quint8>(settings.value(QStringLiteral("mav/gcsComponentId"), 0).toUInt());
    m_uiSettings.targetSystemId = static_cast<quint8>(settings.value(QStringLiteral("mav/targetSystemId"), 0).toUInt());
    m_uiSettings.targetComponentId = static_cast<quint8>(settings.value(QStringLiteral("mav/targetComponentId"), 1).toUInt());
}

void MainWindow::saveUiSettings() const
{
    QSettings settings(QStringLiteral("OpenAI"), QStringLiteral("NexusPilotQt"));
    settings.setValue(QStringLiteral("conn/mode"), m_uiSettings.mode == ConnectionSettings::Mode::Serial ? 0 : 1);
    settings.setValue(QStringLiteral("conn/serialPort"), m_uiSettings.serialPort);
    settings.setValue(QStringLiteral("conn/baudRate"), m_uiSettings.baudRate);
    settings.setValue(QStringLiteral("conn/ip"), m_uiSettings.ip);
    settings.setValue(QStringLiteral("conn/port"), m_uiSettings.port);
    settings.setValue(QStringLiteral("mav/gcsSystemId"), m_uiSettings.gcsSystemId);
    settings.setValue(QStringLiteral("mav/gcsComponentId"), m_uiSettings.gcsComponentId);
    settings.setValue(QStringLiteral("mav/targetSystemId"), m_uiSettings.targetSystemId);
    settings.setValue(QStringLiteral("mav/targetComponentId"), m_uiSettings.targetComponentId);
}

void MainWindow::refreshMavSummary()
{
    if (!m_mavSummaryLabel)
    {
        return;
    }

    m_mavSummaryLabel->setText(
        QStringLiteral("GCS System ID: %1\nGCS Component ID: %2\nTarget System ID: %3\nTarget Component ID: %4")
            .arg(m_uiSettings.gcsSystemId)
            .arg(m_uiSettings.gcsComponentId)
            .arg(m_uiSettings.targetSystemId)
            .arg(m_uiSettings.targetComponentId));
}

void MainWindow::syncConnectionEditors()
{
    const bool serialMode = m_transportCombo->currentIndex() == 0;
    m_uiSettings.mode = serialMode ? ConnectionSettings::Mode::Serial : ConnectionSettings::Mode::Udp;

    m_primaryFieldLabel->setText(serialMode ? QStringLiteral("串口号") : QStringLiteral("IP 地址"));
    m_secondaryFieldLabel->setText(serialMode ? QStringLiteral("波特率") : QStringLiteral("端口"));
    m_primaryEdit->setPlaceholderText(serialMode ? QStringLiteral("COM3") : QStringLiteral("192.168.4.1"));
    m_secondaryEdit->setPlaceholderText(serialMode ? QStringLiteral("57600") : QStringLiteral("14550"));

    if (serialMode)
    {
        m_primaryEdit->setText(m_uiSettings.serialPort);
        m_secondaryEdit->setText(QString::number(m_uiSettings.baudRate));
    }
    else
    {
        m_primaryEdit->setText(m_uiSettings.ip);
        m_secondaryEdit->setText(QString::number(m_uiSettings.port));
    }
}

ConnectionSettings MainWindow::currentSettingsFromUi() const
{
    ConnectionSettings settings = m_uiSettings;
    const bool serialMode = m_transportCombo->currentIndex() == 0;
    settings.mode = serialMode ? ConnectionSettings::Mode::Serial : ConnectionSettings::Mode::Udp;
    if (serialMode)
    {
        settings.serialPort = m_primaryEdit->text().trimmed().isEmpty() ? QStringLiteral("COM3") : m_primaryEdit->text().trimmed();
        settings.baudRate = m_secondaryEdit->text().trimmed().toInt();
        if (settings.baudRate <= 0)
        {
            settings.baudRate = 57600;
        }
    }
    else
    {
        settings.ip = m_primaryEdit->text().trimmed().isEmpty() ? QStringLiteral("192.168.4.1") : m_primaryEdit->text().trimmed();
        const int port = m_secondaryEdit->text().trimmed().toInt();
        settings.port = port > 0 ? static_cast<quint16>(port) : 14550;
    }
    return settings;
}

void MainWindow::configureMavId()
{
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("配置 MAVID"));
    dialog.setModal(true);
    dialog.resize(360, 260);

    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(12);

    auto *tip = new QLabel(QStringLiteral("设置 MAVID，保存后立即生效。"), &dialog);
    tip->setWordWrap(true);
    tip->setStyleSheet(QStringLiteral("color:#475569;font-size:13px;"));
    layout->addWidget(tip);

    auto createSpinRow = [&dialog, layout](const QString &labelText, int value) -> QSpinBox * {
        auto *row = new QHBoxLayout();
        auto *label = new QLabel(labelText, &dialog);
        label->setMinimumWidth(140);
        auto *spin = new QSpinBox(&dialog);
        spin->setRange(0, 255);
        spin->setValue(value);
        row->addWidget(label);
        row->addWidget(spin, 1);
        layout->addLayout(row);
        return spin;
    };

    auto *gcsSystem = createSpinRow(QStringLiteral("GCS System ID"), m_uiSettings.gcsSystemId);
    auto *gcsComponent = createSpinRow(QStringLiteral("GCS Component ID"), m_uiSettings.gcsComponentId);
    auto *targetSystem = createSpinRow(QStringLiteral("Target System ID"), m_uiSettings.targetSystemId);
    auto *targetComponent = createSpinRow(QStringLiteral("Target Component ID"), m_uiSettings.targetComponentId);

    auto *buttons = new QHBoxLayout();
    buttons->addStretch(1);
    auto *cancel = new QPushButton(QStringLiteral("取消"), &dialog);
    auto *ok = new QPushButton(QStringLiteral("保存"), &dialog);
    ok->setStyleSheet(QStringLiteral("QPushButton{background:#2563EB;color:white;font-weight:800;padding:8px 16px;border-radius:10px;}"));
    buttons->addWidget(cancel);
    buttons->addWidget(ok);
    layout->addLayout(buttons);

    connect(cancel, &QPushButton::clicked, &dialog, &QDialog::reject);
    connect(ok, &QPushButton::clicked, &dialog, &QDialog::accept);

    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    m_uiSettings.gcsSystemId = static_cast<quint8>(gcsSystem->value());
    m_uiSettings.gcsComponentId = static_cast<quint8>(gcsComponent->value());
    m_uiSettings.targetSystemId = static_cast<quint8>(targetSystem->value());
    m_uiSettings.targetComponentId = static_cast<quint8>(targetComponent->value());
    saveUiSettings();
    refreshMavSummary();
    m_backend->applyMavId(m_uiSettings.gcsSystemId, m_uiSettings.gcsComponentId, m_uiSettings.targetSystemId, m_uiSettings.targetComponentId);
    appendEvent(QStringLiteral("MAVID 配置已更新。"), QStringLiteral("success"));
}

void MainWindow::connectOrDisconnect()
{
    if (m_backend->isConnected())
    {
        m_backend->disconnectDevice();
        appendEvent(QStringLiteral("已主动断开设备连接。"), QStringLiteral("info"));
        return;
    }

    m_uiSettings = currentSettingsFromUi();
    saveUiSettings();

    if (!m_backend->connectDevice(m_uiSettings))
    {
        QMessageBox::warning(this, QStringLiteral("连接失败"), QStringLiteral("设备连接失败，请检查通讯参数。"));
    }
}

void MainWindow::replaceMapImage()
{
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("选择地图图片"), QString(), QStringLiteral("Image Files (*.png *.jpg *.jpeg *.bmp *.webp)"));
    if (path.isEmpty())
    {
        return;
    }

    m_mapWidget->setMapImage(path);
    QSettings settings(QStringLiteral("OpenAI"), QStringLiteral("NexusPilotQt"));
    settings.setValue(QStringLiteral("map/path"), path);
    appendEvent(QStringLiteral("地图图片已替换：%1").arg(path), QStringLiteral("success"));
}

void MainWindow::appendEvent(const QString &message, const QString &level)
{
    const QString time = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"));
    auto *item = new QListWidgetItem(QStringLiteral("[%1] %2").arg(time, message));
    if (level == QStringLiteral("error"))
    {
        item->setForeground(QColor("#FCA5A5"));
    }
    else if (level == QStringLiteral("warning"))
    {
        item->setForeground(QColor("#FDE68A"));
    }
    else if (level == QStringLiteral("success"))
    {
        item->setForeground(QColor("#86EFAC"));
    }
    else
    {
        item->setForeground(QColor("#CBD5E1"));
    }
    m_eventList->insertItem(0, item);
}

void MainWindow::updateTelemetry(const TelemetrySnapshot &snapshot)
{
    m_snapshot = snapshot;
    m_connectionBadge->setText(snapshot.connectionText);
    m_connectionBadge->setStyleSheet(snapshot.connected ? badgeStyle(QStringLiteral("#166534")) : badgeStyle(QStringLiteral("#334155")));
    m_connectButton->setText(snapshot.connected ? QStringLiteral("断开连接") : QStringLiteral("连接设备"));

    m_linkValue->setText(snapshot.linkStatusText);
    m_batteryStatusValue->setText(snapshot.batteryStatusText);
    m_batteryCapacityValue->setText(QStringLiteral("%1 %").arg(snapshot.batteryPercent, 0, 'f', 1));
    m_batteryCountValue->setText(QStringLiteral("%1 块").arg(snapshot.batteryCount));
    m_voltageValue->setText(QStringLiteral("%1 V").arg(snapshot.averageVoltage, 0, 'f', 2));
    m_currentValue->setText(QStringLiteral("%1 A").arg(snapshot.currentCurrent, 0, 'f', 2));
    m_flightStateValue->setText(snapshot.flightStateText);
    m_selfTestValue->setText(snapshot.selfTestText);
    m_lockStateValue->setText(snapshot.safetyLocked ? QStringLiteral("已打开") : QStringLiteral("已解锁"));
    m_heartbeatDelayValue->setText(snapshot.heartbeatElapsedMs >= 0
                                       ? QStringLiteral("%1 ms").arg(snapshot.heartbeatElapsedMs)
                                       : QStringLiteral("-- ms"));
    m_dropRateValue->setText(snapshot.dropRate >= 0.0f
                                 ? QStringLiteral("%1 %").arg(snapshot.dropRate, 0, 'f', 1)
                                 : QStringLiteral("-- %"));
    m_positionValue->setText(QStringLiteral("经度：%1\n纬度：%2").arg(snapshot.longitude, 0, 'f', 6).arg(snapshot.latitude, 0, 'f', 6));
    m_altitudeValue->setText(QStringLiteral("绝对：%1 m\n相对：%2 m").arg(snapshot.absoluteAltitude, 0, 'f', 1).arg(snapshot.relativeAltitude, 0, 'f', 1));
    m_batteryWidget->setBatteryLevel(snapshot.batteryPercent, snapshot.batteryStatusText);

    m_mapWidget->setDronePose(snapshot.longitude == 0.0 ? snapshot.homeLongitude : snapshot.longitude,
                              snapshot.latitude == 0.0 ? snapshot.homeLatitude : snapshot.latitude,
                              snapshot.headingDegrees);

    updateButtonStates(snapshot);
}

void MainWindow::updateButtonStates(const TelemetrySnapshot &snapshot)
{
    const bool connected = snapshot.connected;
    const bool selfTestDone = snapshot.selfTestPassed;
    const bool locked = snapshot.safetyLocked;

    m_selfTestButton->setEnabled(connected && !snapshot.selfTestInProgress);
    m_lockButton->setEnabled(connected && !locked);
    m_unlockButton->setEnabled(connected && selfTestDone && locked);
    m_startButton->setEnabled(connected && selfTestDone && !locked);
    m_stopButton->setEnabled(connected);
    m_emergencyButton->setEnabled(connected);
}
