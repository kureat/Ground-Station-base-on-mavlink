#include "MapWidget.h"

#include <QPainter>
#include <QPainterPath>

MapWidget::MapWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(780, 620);
}

void MapWidget::setMapImage(const QString &path)
{
    if (path.trimmed().isEmpty())
    {
        m_mapPixmap = QPixmap();
        m_mapPath.clear();
        update();
        return;
    }

    QPixmap pix(path);
    if (!pix.isNull())
    {
        m_mapPixmap = pix;
        m_mapPath = path;
        update();
    }
}

QString MapWidget::mapImagePath() const
{
    return m_mapPath;
}

void MapWidget::setGeoBounds(double minLon, double maxLon, double minLat, double maxLat)
{
    m_minLon = minLon;
    m_maxLon = maxLon;
    m_minLat = minLat;
    m_maxLat = maxLat;
    update();
}

void MapWidget::setDronePose(double longitude, double latitude, float headingDegrees)
{
    m_longitude = longitude;
    m_latitude = latitude;
    m_headingDegrees = headingDegrees;
    update();
}

QPointF MapWidget::geoToPixel(double longitude, double latitude, const QRectF &contentRect) const
{
    const double xRatio = (longitude - m_minLon) / (m_maxLon - m_minLon);
    const double yRatio = 1.0 - (latitude - m_minLat) / (m_maxLat - m_minLat);
    return {
        contentRect.left() + xRatio * contentRect.width(),
        contentRect.top() + yRatio * contentRect.height()};
}

void MapWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF frameRect = rect().adjusted(8, 8, -8, -8);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor("#0B1220"));
    painter.drawRoundedRect(frameRect, 24, 24);

    const QRectF contentRect = frameRect.adjusted(16, 16, -16, -56);
    QPainterPath clipPath;
    clipPath.addRoundedRect(contentRect, 18, 18);

    painter.save();
    painter.setClipPath(clipPath);
    if (!m_mapPixmap.isNull())
    {
        painter.drawPixmap(contentRect.toRect(), m_mapPixmap);
    }
    else
    {
        painter.fillRect(contentRect, QColor("#0F172A"));

        painter.setPen(QPen(QColor("#334155"), 1.2, Qt::DashLine));
        painter.drawRoundedRect(contentRect.adjusted(26, 26, -26, -26), 16, 16);

        const QRectF infoBox(
            contentRect.left() + 120,
            contentRect.center().y() - 52,
            contentRect.width() - 240,
            104);

        const QRectF titleRect(
            infoBox.left(),
            infoBox.top(),
            infoBox.width(),
            34);

        const QRectF bodyRect(
            infoBox.left(),
            infoBox.top() + 36,
            infoBox.width(),
            52);

        painter.setPen(QColor("#E2E8F0"));
        QFont titleFont = painter.font();
        titleFont.setPointSize(16);
        titleFont.setBold(true);
        painter.setFont(titleFont);
        painter.drawText(titleRect, Qt::AlignHCenter | Qt::AlignVCenter,
                         QStringLiteral("地图区域"));

        painter.setPen(QColor("#94A3B8"));
        QFont bodyFont = painter.font();
        bodyFont.setPointSize(10);
        bodyFont.setBold(false);
        painter.setFont(bodyFont);
        painter.drawText(bodyRect,
                         Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap,
                         QStringLiteral("暂未导入地图图片\n点击右上角“导入地图”"));
    }
    painter.restore();

    if (!m_mapPixmap.isNull())
    {
        painter.setPen(QPen(QColor(255, 255, 255, 50), 1.2, Qt::DashLine));
        painter.drawLine(QPointF(contentRect.center().x(), contentRect.top()), QPointF(contentRect.center().x(), contentRect.bottom()));
        painter.drawLine(QPointF(contentRect.left(), contentRect.center().y()), QPointF(contentRect.right(), contentRect.center().y()));

        const QPointF droneCenter = geoToPixel(m_longitude, m_latitude, contentRect);
        painter.setBrush(QColor(37, 99, 235, 40));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(droneCenter, 28, 28);

        painter.save();
        painter.translate(droneCenter);
        painter.rotate(m_headingDegrees);
        QPainterPath arrow;
        arrow.moveTo(0, -24);
        arrow.lineTo(16, 14);
        arrow.lineTo(0, 7);
        arrow.lineTo(-16, 14);
        arrow.closeSubpath();
        painter.setPen(QPen(QColor("#F8FAFC"), 2));
        painter.setBrush(QColor("#2563EB"));
        painter.drawPath(arrow);
        painter.restore();
    }

    painter.setPen(QColor("#CBD5E1"));
    QFont footerFont = painter.font();
    footerFont.setPointSize(10);
    painter.setFont(footerFont);

    if (!m_mapPixmap.isNull())
    {
        painter.drawText(frameRect.adjusted(16, contentRect.height() + 26, -16, -12),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         QStringLiteral("当前位置：%1, %2")
                             .arg(m_longitude, 0, 'f', 6)
                             .arg(m_latitude, 0, 'f', 6));
    }

}
