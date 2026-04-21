#pragma once

#include <QWidget>

class AttitudeWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AttitudeWidget(QWidget *parent = nullptr);

    void setAttitude(float rollDegrees, float pitchDegrees);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    float m_rollDegrees{0.0f};
    float m_pitchDegrees{0.0f};
};
