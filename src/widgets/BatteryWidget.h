#pragma once

#include <QString>
#include <QWidget>

class BatteryWidget : public QWidget
{
public:
    explicit BatteryWidget(QWidget *parent = nullptr);

    void setBatteryLevel(float percent, const QString &statusText);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    float m_percent{0.0f};
    QString m_statusText{QStringLiteral("未知")};
};
