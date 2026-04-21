#include "AttitudeWidget.h"

#include <QPainter>
#include <QtMath>

AttitudeWidget::AttitudeWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(180, 180);
}

void AttitudeWidget::setAttitude(float rollDegrees, float pitchDegrees)
{
    m_rollDegrees = rollDegrees;
    m_pitchDegrees = pitchDegrees;
    update();
}

void AttitudeWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const int side = qMin(width(), height()) - 10;
    const QRectF circleRect((width() - side) / 2.0, (height() - side) / 2.0, side, side);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#0B1220"));
    painter.drawEllipse(circleRect);

    painter.save();
    painter.translate(circleRect.center());
    painter.rotate(-m_rollDegrees);
    const qreal pitchOffset = qBound(-40.0, static_cast<double>(m_pitchDegrees), 40.0) * (side / 120.0);

    painter.setBrush(QColor("#4D7CFE"));
    painter.drawRect(QRectF(-side, pitchOffset, side * 2.0, side));
    painter.setBrush(QColor("#1E293B"));
    painter.drawRect(QRectF(-side, -side, side * 2.0, pitchOffset + side));
    painter.setPen(QPen(QColor("#F8FAFC"), 2));
    painter.drawLine(QPointF(-side, pitchOffset), QPointF(side, pitchOffset));

    painter.setPen(QPen(QColor("#E2E8F0"), 1.2));
    for (int i = -30; i <= 30; i += 10)
    {
        if (i == 0)
        {
            continue;
        }
        const qreal y = pitchOffset - i * (side / 120.0);
        const qreal widthLine = (i % 20 == 0) ? 50 : 30;
        painter.drawLine(QPointF(-widthLine / 2.0, y), QPointF(widthLine / 2.0, y));
    }
    painter.restore();

    painter.setPen(QPen(QColor("#CBD5E1"), 4));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(circleRect.adjusted(2, 2, -2, -2));

    painter.setPen(QPen(QColor("#F8FAFC"), 3));
    painter.drawLine(QPointF(circleRect.center().x() - 42, circleRect.center().y()), QPointF(circleRect.center().x() - 8, circleRect.center().y()));
    painter.drawLine(QPointF(circleRect.center().x() + 8, circleRect.center().y()), QPointF(circleRect.center().x() + 42, circleRect.center().y()));
    painter.drawLine(QPointF(circleRect.center().x(), circleRect.center().y() - 6), QPointF(circleRect.center().x(), circleRect.center().y() + 10));

    painter.setPen(QColor("#E2E8F0"));
    painter.drawText(rect().adjusted(0, 0, 0, -6), Qt::AlignHCenter | Qt::AlignBottom,
                     QStringLiteral("R %1°  P %2°").arg(m_rollDegrees, 0, 'f', 1).arg(m_pitchDegrees, 0, 'f', 1));
}
