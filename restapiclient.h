#ifndef RESTAPICLIENT_H
#define RESTAPICLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTimer>

class RestApiClient : public QObject
{
    Q_OBJECT

public:
    explicit RestApiClient(QObject *parent = nullptr);
    ~RestApiClient();

    void setBaseUrl(const QString &baseUrl);
    void setAuthToken(const QString &token);

    // Authentication endpoints
    void login(const QString &email, const QString &password);
    void registerUser(const QString &name, const QString &email, const QString &password);

    // Session endpoints
    void listSessions();
    void getActiveSession();
    void createSession(const QString &name);
    void activateSession(const QString &sessionId);
    void closeSession(const QString &sessionId);

    // Classroom endpoints
    void listClassrooms();
    void createClassroom(const QString &name, const QString &description);
    void updateClassroom(const QString &roomId, const QString &name, const QString &description);
    void deleteClassroom(const QString &roomId);

    // Attendance endpoints
    void listAttendance(const QString &roomId);
    void markAttendance(const QString &roomId, const QString &code, const QString &studentId, const QString &studentName);
    void generateAttendanceCode(const QString &roomId, int durationMinutes);

    // Request endpoints
    void listRequests(const QString &roomId);
    void createRequest(const QString &roomId, const QString &studentId, const QString &studentName);
    void updateRequest(const QString &roomId, const QString &requestId, const QString &action);

    // Polling for speak requests
    void startPollingRequests(const QString &roomId, int intervalMs = 2000);
    void stopPollingRequests();

    // New method
    void getQrCode(const QString &roomId);

signals:
    // Authentication signals
    void loginSuccess(const QJsonObject &data);
    void loginFailed(const QString &message);
    void registerSuccess(const QJsonObject &data);
    void registerFailed(const QString &message);

    // Session signals
    void sessionsReceived(const QJsonArray &sessions);
    void activeSessionReceived(const QJsonObject &session);
    void sessionCreated(const QJsonObject &session);
    void sessionActivated(const QJsonObject &session);
    void sessionClosed(const QString &sessionId);

    // Classroom signals
    void classroomsReceived(const QJsonArray &classrooms);
    void classroomCreated(const QJsonObject &classroom);
    void classroomUpdated(const QJsonObject &classroom);
    void classroomDeleted();

    // Attendance signals
    void attendanceListReceived(const QJsonArray &records);
    void attendanceMarked(const QJsonObject &record);
    void attendanceCodeGenerated(const QString &code, const QString &qrBase64, const QString &expiry);

    // Request signals
    void requestsReceived(const QJsonArray &requests);
    void requestCreated(const QJsonObject &request);
    void requestUpdated(const QString &requestId, const QString &action);
    void newSpeakRequest(const QString &studentId, const QString &studentName, const QString &requestId);

    // Error signal
    void error(const QString &message);

    // New signal
    void qrCodeReceived(const QString &qrBase64);

private slots:
    void handleNetworkReply(QNetworkReply *reply);
    void pollRequests();

private:
    void sendRequest(const QString &endpoint, const QString &method, const QJsonObject &data = QJsonObject());
    void handleResponse(const QString &endpoint, const QJsonDocument &response);

    QNetworkAccessManager *m_networkManager;
    QString m_baseUrl;
    QString m_authToken;

    // For polling
    QTimer *m_pollTimer;
    QString m_currentRoomId;
    QStringList m_processedRequestIds;
};

#endif // RESTAPICLIENT_H