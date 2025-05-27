#ifndef GESTUREPROCESSOR_H
#define GESTUREPROCESSOR_H

#include <QObject>
#include <QMap>
#include <QString>
#include <functional>
#include <QSettings>

class GestureProcessor : public QObject
{
    Q_OBJECT
public:
    explicit GestureProcessor(QObject *parent = nullptr);

    // Gesture types
    enum class GestureType {
        Required,   // Zorunlu gesture'lar
        Custom     // Özelleştirilebilir gesture'lar
    };

    // Gesture command registration
    void registerRequiredGestureCommand(const QString &gesture, std::function<void(double x, double y)> callback);
    void registerCustomGestureCommand(const QString &gesture, std::function<void(double x, double y)> callback);
    void processGesture(const QString &gesture, double x = 0.0, double y = 0.0);
    
    // Custom gesture management
    void setCustomGestureAction(const QString &gesture, const QString &action, int repeatCount = 1);
    QString getCustomGestureAction(const QString &gesture) const;
    int getCustomGestureRepeatCount(const QString &gesture) const;
    void clearCustomGestureAction(const QString &gesture);
    
    // Get available gestures
    QStringList getRequiredGestures() const;
    QStringList getCustomGestures() const;
    QStringList getAvailableActions() const;

    // Simple gesture commands
    void registerGestureCommand(const QString &gesture, std::function<void()> callback);
    void unregisterGestureCommand(const QString &gesture);
    QStringList getRegisteredGestures() const;

public slots:
    void onCommandDetected(const QString &gesture);

signals:
    void gestureDetected(const QString &gesture);
    void unsupportedGestureDetected(const QString &gesture);

private:
    struct GestureInfo {
        GestureType type;
        std::function<void(double x, double y)> coordCallback;
        std::function<void()> simpleCallback;
        QString action;
        int repeatCount;
        bool hasCoordCallback;
    };

    QMap<QString, GestureInfo> m_gestureCommands;
    QSettings m_settings;

    void loadCustomGestureSettings();
    void saveCustomGestureSettings();
    void initializeRequiredGestures();
    void initializeCustomGestures();
};

#endif // GESTUREPROCESSOR_H
