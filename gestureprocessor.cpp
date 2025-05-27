#include "gestureprocessor.h"
#include "presentationmanager.h"
#include "mainwindow.h"
#include <QDebug>

GestureProcessor::GestureProcessor(QObject *parent)
    : QObject(parent)
    , m_settings("AirClass", "Desktop")
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

    initializeRequiredGestures();
    initializeCustomGestures();
    loadCustomGestureSettings();

    // Register presentation control gesture commands
    registerGestureCommand("three_gun", [presentationManager]() {
        qDebug() << "Gesture: Next slide";
        presentationManager->nextSlide();
    });

    registerGestureCommand("three_gun", [presentationManager]() {
        qDebug() << "Gesture: Previous slide";
        presentationManager->previousSlide();
    });

    // Zoom in with three fingers
    registerGestureCommand("three_parmak", [presentationManager, mainWindow]() {
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

    // Zoom out with four fingers
    registerGestureCommand("four_parmak", [presentationManager, mainWindow]() {
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

    // Pointer mode with two fingers up
    registerGestureCommand("two_finger_up", [mainWindow]() {
        qDebug() << "Gesture: Pointer mode";
        QJsonObject data;
        data["gesture_type"] = "POINTER";
        data["client_id"] = "gesture";
        data["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        mainWindow->handleGestureCommand(data);
    });

    // Draw mode with one finger up (after two fingers)
    registerGestureCommand("one_finger_up", [mainWindow]() {
        qDebug() << "Gesture: Draw mode";
        QJsonObject data;
        data["gesture_type"] = "DRAW";
        data["client_id"] = "gesture";
        data["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        mainWindow->handleGestureCommand(data);
    });

    // Accept with like gesture
    registerGestureCommand("like", [mainWindow]() {
        qDebug() << "Gesture: Accept request";
        QJsonObject data;
        data["gesture_type"] = "ACCEPT";
        data["client_id"] = "gesture";
        data["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        mainWindow->handleGestureCommand(data);
    });

    // Decline with dislike gesture
    registerGestureCommand("dislike", [mainWindow]() {
        qDebug() << "Gesture: Decline request";
        QJsonObject data;
        data["gesture_type"] = "DECLINE";
        data["client_id"] = "gesture";
        data["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        mainWindow->handleGestureCommand(data);
    });

    // Attendance with take picture gesture
    registerGestureCommand("take_picture", [mainWindow]() {
        qDebug() << "Gesture: Take attendance";
        QJsonObject data;
        data["gesture_type"] = "ATTENDANCE";
        data["client_id"] = "gesture";
        data["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        mainWindow->handleGestureCommand(data);
    });

    // Attention with palm gesture
    registerGestureCommand("palm", [mainWindow]() {
        qDebug() << "Gesture: Attention";
        QJsonObject data;
        data["gesture_type"] = "ATTENTION";
        data["client_id"] = "gesture";
        data["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        mainWindow->handleGestureCommand(data);
    });

    // Timeout gesture
    registerGestureCommand("timeout", [mainWindow]() {
        qDebug() << "Gesture: Time out";
        QJsonObject data;
        data["gesture_type"] = "TIMEOUT";
        data["client_id"] = "gesture";
        data["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
        mainWindow->handleGestureCommand(data);
    });
}

void GestureProcessor::initializeRequiredGestures()
{
    // Required gestures and their default functions
    const QMap<QString, QString> requiredGestures = {
        {"three_gun", "next_slide"},
        {"three_gun", "previous_slide"},
        {"three_parmak", "zoom_in"},
        {"four_parmak", "zoom_out"},
        {"two_finger_up", "pointer"},
        {"one_finger_up", "draw"},
        {"like", "accept"},
        {"dislike", "decline"},
        {"take_picture", "attendance"},
        {"palm", "attention"},
        {"timeout", "timeout"}
    };

    for (auto it = requiredGestures.begin(); it != requiredGestures.end(); ++it) {
        GestureInfo info;
        info.type = GestureType::Required;
        info.action = it.value();
        info.repeatCount = 1;
        m_gestureCommands[it.key()] = info;
    }
}

void GestureProcessor::initializeCustomGestures()
{
    // Custom gestures
    const QStringList customGestures = {
        "call",
        "ok",
        "heart",
        "heart2",
        "mid_finger",
        "rock",
        "thumb_index",
        "holy",
        "three2"
    };

    for (const QString &gesture : customGestures) {
        GestureInfo info;
        info.type = GestureType::Custom;
        info.action = "";
        info.repeatCount = 1;
        m_gestureCommands[gesture] = info;
    }
}

void GestureProcessor::loadCustomGestureSettings()
{
    m_settings.beginGroup("CustomGestures");
    for (const QString &gesture : getCustomGestures()) {
        if (m_settings.contains(gesture + "/action")) {
            m_gestureCommands[gesture].action = m_settings.value(gesture + "/action").toString();
            m_gestureCommands[gesture].repeatCount = m_settings.value(gesture + "/repeatCount", 1).toInt();
        }
    }
    m_settings.endGroup();
}

void GestureProcessor::saveCustomGestureSettings()
{
    m_settings.beginGroup("CustomGestures");
    m_settings.remove(""); // Clear existing settings

    for (const QString &gesture : getCustomGestures()) {
        const GestureInfo &info = m_gestureCommands[gesture];
        if (!info.action.isEmpty()) {
            m_settings.setValue(gesture + "/action", info.action);
            m_settings.setValue(gesture + "/repeatCount", info.repeatCount);
        }
    }
    m_settings.endGroup();
    m_settings.sync();
}

void GestureProcessor::registerGestureCommand(const QString &gesture, std::function<void()> callback)
{
    GestureInfo info;
    info.type = GestureType::Required;
    info.action = gesture;
    info.repeatCount = 1;
    info.simpleCallback = callback;
    info.hasCoordCallback = false;
    m_gestureCommands[gesture] = info;
    qDebug() << "Registered gesture command:" << gesture;
}

void GestureProcessor::registerRequiredGestureCommand(const QString &gesture, std::function<void(double x, double y)> callback)
{
    if (m_gestureCommands.contains(gesture) && m_gestureCommands[gesture].type == GestureType::Required) {
        m_gestureCommands[gesture].coordCallback = callback;
        m_gestureCommands[gesture].hasCoordCallback = true;
    }
}

void GestureProcessor::registerCustomGestureCommand(const QString &gesture, std::function<void(double x, double y)> callback)
{
    if (m_gestureCommands.contains(gesture) && m_gestureCommands[gesture].type == GestureType::Custom) {
        m_gestureCommands[gesture].coordCallback = callback;
        m_gestureCommands[gesture].hasCoordCallback = true;
    }
}

void GestureProcessor::processGesture(const QString &gesture, double x, double y)
{
    if (!m_gestureCommands.contains(gesture)) {
        qDebug() << "Unknown gesture:" << gesture;
        return;
    }

    const GestureInfo &info = m_gestureCommands[gesture];
    
    for (int i = 0; i < info.repeatCount; ++i) {
        if (info.hasCoordCallback && info.coordCallback) {
            info.coordCallback(x, y);
        } else if (!info.hasCoordCallback && info.simpleCallback) {
            info.simpleCallback();
        }
    }
}

void GestureProcessor::setCustomGestureAction(const QString &gesture, const QString &action, int repeatCount)
{
    if (m_gestureCommands.contains(gesture) && m_gestureCommands[gesture].type == GestureType::Custom) {
        m_gestureCommands[gesture].action = action;
        m_gestureCommands[gesture].repeatCount = repeatCount;
        saveCustomGestureSettings();
    }
}

QString GestureProcessor::getCustomGestureAction(const QString &gesture) const
{
    if (m_gestureCommands.contains(gesture) && m_gestureCommands[gesture].type == GestureType::Custom) {
        return m_gestureCommands[gesture].action;
    }
    return QString();
}

int GestureProcessor::getCustomGestureRepeatCount(const QString &gesture) const
{
    if (m_gestureCommands.contains(gesture) && m_gestureCommands[gesture].type == GestureType::Custom) {
        return m_gestureCommands[gesture].repeatCount;
    }
    return 1;
}

void GestureProcessor::clearCustomGestureAction(const QString &gesture)
{
    if (m_gestureCommands.contains(gesture) && m_gestureCommands[gesture].type == GestureType::Custom) {
        m_gestureCommands[gesture].action = "";
        m_gestureCommands[gesture].repeatCount = 1;
        saveCustomGestureSettings();
    }
}

QStringList GestureProcessor::getRequiredGestures() const
{
    QStringList gestures;
    for (auto it = m_gestureCommands.begin(); it != m_gestureCommands.end(); ++it) {
        if (it.value().type == GestureType::Required) {
            gestures << it.key();
        }
    }
    return gestures;
}

QStringList GestureProcessor::getCustomGestures() const
{
    QStringList gestures;
    for (auto it = m_gestureCommands.begin(); it != m_gestureCommands.end(); ++it) {
        if (it.value().type == GestureType::Custom) {
            gestures << it.key();
        }
    }
    return gestures;
}

QStringList GestureProcessor::getAvailableActions() const
{
    return QStringList() << "next_slide" << "prev_slide" << "zoom_in" << "zoom_out";
}

void GestureProcessor::onCommandDetected(const QString &gesture)
{
    emit gestureDetected(gesture);

    if (m_gestureCommands.contains(gesture)) {
        qDebug() << "Executing gesture command:" << gesture;
        processGesture(gesture, 0.0, 0.0);
    } else {
        qWarning() << "Unsupported gesture detected:" << gesture;
        emit unsupportedGestureDetected(gesture);
    }
}

void GestureProcessor::unregisterGestureCommand(const QString &gesture)
{
    if (m_gestureCommands.contains(gesture)) {
        m_gestureCommands.remove(gesture);
        qDebug() << "Unregistered gesture command:" << gesture;
    } else {
        qWarning() << "Cannot unregister gesture command (not found):" << gesture;
    }
}

QStringList GestureProcessor::getRegisteredGestures() const
{
    QStringList gestures;
    for (auto it = m_gestureCommands.begin(); it != m_gestureCommands.end(); ++it) {
        gestures << it.key();
    }
    return gestures;
}
