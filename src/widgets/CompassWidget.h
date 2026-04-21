#pragma once

#include <QWidget>

class CompassWidget : public QWidget
{
    Q_OBJECT
public:
    explicit CompassWidget(QWidget *parent = nullptr);

    void setHeading(float headingDegrees);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    float m_headingDegrees{0.0f};
};
