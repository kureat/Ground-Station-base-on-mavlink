#pragma once

#include <QPixmap>
#include <QWidget>

class MapWidget : public QWidget
{
public:
    explicit MapWidget(QWidget *parent = nullptr);

    void setMapImage(const QString &path);
    QString mapImagePath() const;

    void setGeoBounds(double minLon, double maxLon, double minLat, double maxLat);
    void setDronePose(double longitude, double latitude, float headingDegrees);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QPointF geoToPixel(double longitude, double latitude, const QRectF &contentRect) const;

private:
    QPixmap m_mapPixmap;
    QString m_mapPath;
    double m_minLon{113.3475};
    double m_maxLon{113.3597};
    double m_minLat{23.1188};
    double m_maxLat{23.1304};
    double m_longitude{113.3535};
    double m_latitude{23.1247};
    float m_headingDegrees{0.0f};
};
