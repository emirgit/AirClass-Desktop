#include "drawinglayer.h"
#include <QPainter>
#include <QMouseEvent>

DrawingLayer::DrawingLayer(QWidget *parent)
    : QWidget(parent)
    , m_penColor(Qt::red)
    , m_penWidth(2)
    , m_isDrawing(false)
    , m_isDrawingEnabled(true)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, false);
    setAttribute(Qt::WA_TranslucentBackground);
}

void DrawingLayer::clear()
{
    m_paths.clear();
    m_currentPath.clear();
    update();
}

void DrawingLayer::setPenColor(const QColor &color)
{
    m_penColor = color;
}

void DrawingLayer::setPenWidth(int width)
{
    m_penWidth = width;
}

void DrawingLayer::setDrawingEnabled(bool enabled)
{
    m_isDrawingEnabled = enabled;
    if (!enabled && m_isDrawing) {
        m_isDrawing = false;
        if (!m_currentPath.isEmpty()) {
            DrawingPath path;
            path.points = m_currentPath;
            path.color = m_penColor;
            path.width = m_penWidth;
            m_paths.append(path);
            m_currentPath.clear();
            update();
        }
    }
}

void DrawingLayer::drawRemotePoint(const QPointF &point, bool isStart)
{
    if (!m_isDrawingEnabled) return;
    
    if (isStart) {
        if (!m_currentPath.isEmpty()) {
            DrawingPath path;
            path.points = m_currentPath;
            path.color = m_penColor;
            path.width = m_penWidth;
            m_paths.append(path);
            m_currentPath.clear();
        }
    }
    m_currentPath.append(point);
    update();
}

void DrawingLayer::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw all completed paths
    for (const DrawingPath &path : m_paths) {
        if (path.points.size() < 2) continue;

        QPen pen(path.color, path.width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(pen);

        for (int i = 1; i < path.points.size(); ++i) {
            painter.drawLine(path.points[i-1], path.points[i]);
        }
    }

    // Draw current path
    if (m_currentPath.size() >= 2) {
        QPen pen(m_penColor, m_penWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(pen);

        for (int i = 1; i < m_currentPath.size(); ++i) {
            painter.drawLine(m_currentPath[i-1], m_currentPath[i]);
        }
    }
}

void DrawingLayer::mousePressEvent(QMouseEvent *event)
{
    if (!m_isDrawingEnabled) return;
    
    if (event->button() == Qt::LeftButton) {
        m_isDrawing = true;
        m_currentPath.clear();
        m_currentPath.append(event->pos());
    }
}

void DrawingLayer::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_isDrawingEnabled || !m_isDrawing) return;
    
    m_currentPath.append(event->pos());
    update();
}

void DrawingLayer::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_isDrawingEnabled) return;
    
    if (event->button() == Qt::LeftButton && m_isDrawing) {
        m_isDrawing = false;
        if (!m_currentPath.isEmpty()) {
            DrawingPath path;
            path.points = m_currentPath;
            path.color = m_penColor;
            path.width = m_penWidth;
            m_paths.append(path);
            m_currentPath.clear();
            update();
        }
    }
} 