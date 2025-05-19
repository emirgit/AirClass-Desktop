#ifndef GESTUREPROCESSOR_H
#define GESTUREPROCESSOR_H

#include <QObject>
#include <QMap>
#include <QString>
#include <functional>

class GestureProcessor : public QObject
{
    Q_OBJECT
public:
    explicit GestureProcessor(QObject *parent = nullptr);

    void onCommandDetected(const QString &gesture);
    void registerGestureCommand(const QString &gesture, std::function<void()> callback);
    void unregisterGestureCommand(const QString &gesture);

    QStringList getRegisteredGestures() const;

signals:
    void gestureDetected(const QString &gesture);
    void unsupportedGestureDetected(const QString &gesture);

private:
    QMap<QString, std::function<void()>> m_gestureCommandMap;
};

#endif // GESTUREPROCESSOR_H
