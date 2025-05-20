#include "gestureprocessor.h"
#include "presentationmanager.h"
#include "mainwindow.h"
#include <QDebug>

GestureProcessor::GestureProcessor(QObject *parent)
    : QObject(parent)
{
    // Get references to required managers
    MainWindow* mainWindow = qobject_cast<MainWindow*>(parent);
    if (!mainWindow) {
        qWarning() << "GestureProcessor: Parent is not MainWindow";
        return;
    }

    PresentationManager* presentationManager = mainWindow->getPresentationManager();
    if (!presentationManager) {
        qWarning() << "GestureProcessor: PresentationManager not found";
        return;
    }

    // Register presentation control gesture commands
    registerGestureCommand("Accept the request", [mainWindow]() {
        qDebug() << "Gesture: Accept request";
        QJsonObject data;
        data["gesture_type"] = "THUMB_UP";
        data["client_id"] = "gesture";
        data["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        mainWindow->handleGestureCommand(data);
    });

    registerGestureCommand("Deny the request", [mainWindow]() {
        qDebug() << "Gesture: Deny request";
        QJsonObject data;
        data["gesture_type"] = "THUMB_DOWN";
        data["client_id"] = "gesture";
        data["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        mainWindow->handleGestureCommand(data);
    });

    registerGestureCommand("Next slide", [presentationManager]() {
        qDebug() << "Gesture: Next slide";
        presentationManager->nextSlide();
    });

    registerGestureCommand("Previous slide", [presentationManager]() {
        qDebug() << "Gesture: Previous slide";
        presentationManager->previousSlide();
    });

    // Zoom in with optional center point
    registerGestureCommand("Zoom in", [presentationManager, mainWindow]() {
        qDebug() << "Gesture: Zoom in";
        QPdfView* pdfView = mainWindow->getPdfView();
        if (!pdfView) {
            qWarning() << "PDF view not found for zoom operation";
            return;
        }

        // Get the current mouse position from MainWindow
        QPoint mousePos = mainWindow->mapFromGlobal(QCursor::pos());
        if (mousePos != QPoint(-1, -1)) {
            // Convert mouse position to PDF view coordinates
            QPointF pdfViewPos = pdfView->mapFrom(mainWindow, mousePos);
            presentationManager->setZoomLevel(presentationManager->getZoomLevel() * 1.25, pdfViewPos);
        } else {
            // If no mouse position, zoom centered
            presentationManager->zoomIn();
        }
    });

    // Zoom out with optional center point
    registerGestureCommand("Zoom out", [presentationManager, mainWindow]() {
        qDebug() << "Gesture: Zoom out";
        QPdfView* pdfView = mainWindow->getPdfView();
        if (!pdfView) {
            qWarning() << "PDF view not found for zoom operation";
            return;
        }

        // Get the current mouse position from MainWindow
        QPoint mousePos = mainWindow->mapFromGlobal(QCursor::pos());
        if (mousePos != QPoint(-1, -1)) {
            // Convert mouse position to PDF view coordinates
            QPointF pdfViewPos = pdfView->mapFrom(mainWindow, mousePos);
            presentationManager->setZoomLevel(presentationManager->getZoomLevel() * 0.8, pdfViewPos);
        } else {
            // If no mouse position, zoom centered
            presentationManager->zoomOut();
        }
    });

    registerGestureCommand("Attention", [mainWindow]() {
        qDebug() << "Gesture: Attention";
        QJsonObject data;
        data["gesture_type"] = "WAVE";
        data["client_id"] = "gesture";
        data["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        mainWindow->handleGestureCommand(data);
    });

    registerGestureCommand("Time out", [mainWindow]() {
        qDebug() << "Gesture: Time out";
        QJsonObject data;
        data["gesture_type"] = "TIMEOUT";
        data["client_id"] = "gesture";
        data["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        mainWindow->handleGestureCommand(data);
    });
}

void GestureProcessor::onCommandDetected(const QString &gesture)
{
    emit gestureDetected(gesture);

    if (m_gestureCommandMap.contains(gesture)) {
        qDebug() << "Executing gesture command:" << gesture;

        // Execute the registered callback function
        m_gestureCommandMap[gesture]();
    } else {
        qWarning() << "Unsupported gesture detected:" << gesture;
        emit unsupportedGestureDetected(gesture);
    }
}

void GestureProcessor::registerGestureCommand(const QString &gesture, std::function<void()> callback)
{
    m_gestureCommandMap[gesture] = callback;
    qDebug() << "Registered gesture command:" << gesture;
}

void GestureProcessor::unregisterGestureCommand(const QString &gesture)
{
    if (m_gestureCommandMap.contains(gesture)) {
        m_gestureCommandMap.remove(gesture);
        qDebug() << "Unregistered gesture command:" << gesture;
    } else {
        qWarning() << "Cannot unregister gesture command (not found):" << gesture;
    }
}

QStringList GestureProcessor::getRegisteredGestures() const
{
    return m_gestureCommandMap.keys();
}
