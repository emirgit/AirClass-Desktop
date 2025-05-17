#include "gestureprocessor.h"
#include <QDebug>

GestureProcessor::GestureProcessor(QObject *parent)
    : QObject(parent)
{
    // Register default gesture commands
    // Note: These are just placeholders; actual implementation will map
    // to MainWindow's presentation manager functions
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
