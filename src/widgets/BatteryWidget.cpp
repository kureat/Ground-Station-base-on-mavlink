#include "BatteryWidget.h"

#include <QPainter>
#include <QPainterPath>
#include <QtGlobal>

BatteryWidget::BatteryWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(210, 116);
}

void BatteryWidget::setBatteryLevel(float percent, const QString &statusText)
{
    m_percent = qBound(0.0f, percent, 100.0f);
    m_statusText = statusText;
    update();
}

void BatteryWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF outer = rect().adjusted(10, 18, -24, -24);
    const QRectF cap(outer.right() + 6, outer.top() + outer.height() * 0.28, 10, outer.height() * 0.44);
    const QRectF inner = outer.adjusted(8, 8, -8, -8);

    QColor fillColor("#16A34A");
    if (m_percent <= 20.0f)
    {
        fillColor = QColor("#DC2626");
    }
    else if (m_percent <= 45.0f)
    {
        fillColor = QColor("#F59E0B");
    }

    painter.setPen(QPen(QColor("#94A3B8"), 2.4));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(outer, 18, 18);
    painter.drawRoundedRect(cap, 3, 3);

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#0B1220"));
    painter.drawRoundedRect(inner, 12, 12);

    QRectF levelRect = inner;
    levelRect.setWidth(inner.width() * (m_percent / 100.0f));
    if (levelRect.width() > 0.0)
    {
        painter.setBrush(fillColor);
        painter.drawRoundedRect(levelRect, 10, 10);
    }

    painter.setPen(QColor("#F8FAFC"));
    QFont percentFont = painter.font();
    percentFont.setPointSize(18);
    percentFont.setBold(true);
    painter.setFont(percentFont);
    painter.drawText(inner, Qt::AlignCenter, QStringLiteral("%1%").arg(m_percent, 0, 'f', 0));

    QFont statusFont = painter.font();
    statusFont.setPointSize(10);
    statusFont.setBold(false);
    painter.setFont(statusFont);
    painter.setPen(QColor("#94A3B8"));
    painter.drawText(QRectF(10, rect().bottom() - 26, rect().width() - 20, 18),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     QStringLiteral("电池状态：%1").arg(m_statusText));
}
