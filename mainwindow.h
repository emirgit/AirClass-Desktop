#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMessageBox>
#include <QPushButton> // Add explicit include for QPushButton

#include "websocketclient.h"
#include "presentationmanager.h"
#include "attendancemanager.h"
#include "gestureprocessor.h"
#include "uicontroller.h"

// Forward declare UI namespace and MainWindow class properly
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void initialize();

private slots:
    void onConnected();
    void onDisconnected();
    void handleServerMessage(const QString &message);

    // UI Actions
    void on_actionOpen_triggered();
    void on_actionExit_triggered();
    void on_actionConnect_triggered();
    void on_actionDisconnect_triggered();
    void on_nextButton_clicked();
    void on_prevButton_clicked();
    void on_zoomInButton_clicked();
    void on_zoomOutButton_clicked();

private:
    Ui::MainWindow *ui;
    WebSocketClient *m_webSocketClient;
    PresentationManager *m_presentationManager;
    AttendanceManager *m_attendanceManager;
    GestureProcessor *m_gestureProcessor;
    UIController *m_uiController;

    // Message handlers
    void handleSlideCommand(const QJsonObject &data);
    void handleAttendanceMessage(const QJsonObject &data);
    void handleGestureCommand(const QJsonObject &data);
    void handleSpeakRequest(const QJsonObject &data);

    void setupConnections();
};

#endif // MAINWINDOW_H
