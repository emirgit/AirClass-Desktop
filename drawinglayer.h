#ifndef DRAWINGLAYER_H
#define DRAWINGLAYER_H

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QPointF>
#include <QVector>
#include <QColor>

class DrawingLayer : public QWidget
{
    Q_OBJECT

public:
    explicit DrawingLayer(QWidget *parent = nullptr);
    void clear();
    void setPenColor(const QColor &color);
    void setPenWidth(int width);
    void setDrawingEnabled(bool enabled);
    QColor getPenColor() const { return m_penColor; }
    bool isDrawingEnabled() const { return m_isDrawingEnabled; }

public slots:
    void drawRemotePoint(const QPointF &point, bool isStart);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    struct DrawingPath {
        QVector<QPointF> points;
        QColor color;
        int width;
    };

    QVector<DrawingPath> m_paths;
    QVector<QPointF> m_currentPath;
    QColor m_penColor;
    int m_penWidth;
    bool m_isDrawing;
    bool m_isDrawingEnabled;
};

#endif // DRAWINGLAYER_H 