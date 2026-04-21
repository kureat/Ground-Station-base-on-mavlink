#include "CompassWidget.h"

#include <QPainter>
#include <QtMath>

CompassWidget::CompassWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(180, 180);
}

void CompassWidget::setHeading(float headingDegrees)
{
    m_headingDegrees = headingDegrees;
    update();
}

void CompassWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const int side = qMin(width(), height()) - 10;
    const QRectF circleRect((width() - side) / 2.0, (height() - side) / 2.0, side, side);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#111827"));
    painter.drawEllipse(circleRect);

    painter.setPen(QPen(QColor("#334155"), 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(circleRect.adjusted(5, 5, -5, -5));

    painter.save();
    painter.translate(circleRect.center());
    for (int angle = 0; angle < 360; angle += 30)
    {
        painter.save();
        painter.rotate(angle);
        const qreal inner = side / 2.0 - ((angle % 90 == 0) ? 24 : 16);
        painter.setPen(QPen(QColor("#CBD5E1"), (angle % 90 == 0) ? 2.4 : 1.2));
        painter.drawLine(QPointF(0, -side / 2.0 + 10), QPointF(0, -inner));
        painter.restore();
    }
    painter.restore();

    painter.setPen(QColor("#E2E8F0"));
    const QStringList dirs{QStringLiteral("N"), QStringLiteral("E"), QStringLiteral("S"), QStringLiteral("W")};
    for (int i = 0; i < 4; ++i)
    {
        const qreal angleRad = qDegreesToRadians(i * 90.0 - 90.0);
        const QPointF p(circleRect.center().x() + qCos(angleRad) * (side / 2.0 - 24),
                        circleRect.center().y() + qSin(angleRad) * (side / 2.0 - 24));
        painter.drawText(QRectF(p.x() - 10, p.y() - 10, 20, 20), Qt::AlignCenter, dirs.at(i));
    }

    painter.save();
    painter.translate(circleRect.center());
    painter.rotate(m_headingDegrees);
    QPolygonF needle;
    needle << QPointF(0, -side / 2.0 + 22) << QPointF(8, 0) << QPointF(0, 14) << QPointF(-8, 0);
    painter.setPen(QPen(QColor("#F87171"), 1.5));
    painter.setBrush(QColor("#EF4444"));
    painter.drawPolygon(needle);
    painter.restore();

    painter.setBrush(QColor("#0F172A"));
    painter.setPen(QPen(QColor("#60A5FA"), 2));
    painter.drawEllipse(circleRect.center(), 18, 18);

    painter.setPen(QColor("#E2E8F0"));
    painter.drawText(rect().adjusted(0, 0, 0, -6), Qt::AlignHCenter | Qt::AlignBottom,
                     QStringLiteral("HDG %1°").arg(m_headingDegrees, 0, 'f', 1));
}
