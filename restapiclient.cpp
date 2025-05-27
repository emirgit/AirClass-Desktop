#include "restapiclient.h"
#include <QNetworkRequest>
#include <QDebug>
#include <QUrlQuery>

RestApiClient::RestApiClient(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_pollTimer(new QTimer(this))
{
    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &RestApiClient::handleNetworkReply);

    connect(m_pollTimer, &QTimer::timeout, this, &RestApiClient::pollRequests);
}

RestApiClient::~RestApiClient()
{
    if (m_pollTimer->isActive()) {
        m_pollTimer->stop();
    }
}

void RestApiClient::setBaseUrl(const QString &baseUrl)
{
    m_baseUrl = baseUrl;
    if (!m_baseUrl.endsWith('/')) {
        m_baseUrl += '/';
    }
}

void RestApiClient::setAuthToken(const QString &token)
{
    m_authToken = token;
}

void RestApiClient::sendRequest(const QString &endpoint, const QString &method, const QJsonObject &data)
{
    QUrl url(m_baseUrl + endpoint);
    QNetworkRequest request(url);

    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    if (!m_authToken.isEmpty()) {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(m_authToken).toUtf8());
    }

    QNetworkReply *reply = nullptr;

    if (method == "GET") {
        reply = m_networkManager->get(request);
    } else if (method == "POST" || method == "PUT" || method == "DELETE") {
        QJsonDocument doc(data);
        QByteArray jsonData = doc.toJson();

        if (method == "POST") {
            reply = m_networkManager->post(request, jsonData);
        } else if (method == "PUT") {
            reply = m_networkManager->put(request, jsonData);
        } else if (method == "DELETE") {
            reply = m_networkManager->deleteResource(request);
        }
    }

    if (reply) {
        reply->setProperty("endpoint", endpoint);
    }
}

void RestApiClient::handleNetworkReply(QNetworkReply *reply)
{
    reply->deleteLater();

    QString endpoint = reply->property("endpoint").toString();

    if (reply->error() != QNetworkReply::NoError) {
        emit error(QString("Network error: %1").arg(reply->errorString()));
        return;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument response = QJsonDocument::fromJson(responseData);

    if (response.isNull()) {
        emit error("Invalid JSON response");
        return;
    }

    handleResponse(endpoint, response);
}


// Authentication endpoints
void RestApiClient::login(const QString &email, const QString &password)
{
    QJsonObject data;
    data["email"] = email;
    data["password"] = password;
    sendRequest("auth/login", "POST", data);
}

void RestApiClient::registerUser(const QString &name, const QString &email, const QString &password)
{
    QJsonObject data;
    data["name"] = name;
    data["email"] = email;
    data["password"] = password;
    sendRequest("auth/register", "POST", data);
}

// Classroom endpoints
void RestApiClient::listClassrooms()
{
    sendRequest("classroom", "GET");
}

void RestApiClient::createClassroom(const QString &name, const QString &description)
{
    QJsonObject data;
    data["name"] = name;
    data["description"] = description;
    sendRequest("classroom", "POST", data);
}

void RestApiClient::updateClassroom(const QString &roomId, const QString &name, const QString &description)
{
    QJsonObject data;
    data["room_id"] = roomId;
    data["name"] = name;
    data["description"] = description;
    sendRequest("classroom", "PUT", data);
}

void RestApiClient::deleteClassroom(const QString &roomId)
{
    QJsonObject data;
    data["room_id"] = roomId;
    sendRequest("classroom", "DELETE", data);
}

// Attendance endpoints
void RestApiClient::listAttendance(const QString &roomId)
{
    QUrl url(m_baseUrl + "attendance/list");
    QUrlQuery query;
    query.addQueryItem("room_id", roomId);
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    if (!m_authToken.isEmpty()) {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(m_authToken).toUtf8());
    }

    qDebug() << "Sending attendance list request to:" << url.toString();
    QNetworkReply *reply = m_networkManager->get(request);
    reply->setProperty("endpoint", "attendance/list");
}

void RestApiClient::markAttendance(const QString &roomId, const QString &code,
                                   const QString &studentId, const QString &studentName)
{
    QJsonObject data;
    data["room_id"] = roomId;
    data["code"] = code;
    data["student_id"] = studentId;
    data["student_name"] = studentName;
    sendRequest("attendance", "POST", data);
}

void RestApiClient::generateAttendanceCode(const QString &roomId, int durationMinutes)
{
    QJsonObject data;
    data["room_id"] = roomId;
    data["duration_minutes"] = durationMinutes;
    sendRequest("attendance/code", "POST", data);
}

// Request endpoints
void RestApiClient::listRequests(const QString &roomId)
{
    QUrl url(m_baseUrl + "request");
    QUrlQuery query;
    query.addQueryItem("room_id", roomId);
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    if (!m_authToken.isEmpty()) {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(m_authToken).toUtf8());
    }

    QNetworkReply *reply = m_networkManager->get(request);
    reply->setProperty("endpoint", "request");
}

void RestApiClient::createRequest(const QString &roomId, const QString &studentId, const QString &studentName)
{
    QJsonObject data;
    data["room_id"] = roomId;
    data["student_id"] = studentId;
    data["student_name"] = studentName;
    sendRequest("request", "POST", data);
}

void RestApiClient::updateRequest(const QString &roomId, const QString &requestId, const QString &action)
{
    QJsonObject data;
    data["room_id"] = roomId;
    data["request_id"] = requestId;
    data["action"] = action;
    sendRequest("request", "PUT", data);
}

// Polling methods
void RestApiClient::startPollingRequests(const QString &roomId, int intervalMs)
{
    m_currentRoomId = roomId;
    m_pollTimer->start(intervalMs);

    // Immediately poll once
    pollRequests();
}

void RestApiClient::stopPollingRequests()
{
    m_pollTimer->stop();
}

void RestApiClient::pollRequests()
{
    if (!m_currentRoomId.isEmpty()) {
        listRequests(m_currentRoomId);
    }
}



// Session endpoints
void RestApiClient::listSessions()
{
    sendRequest("session/list", "GET");
}

void RestApiClient::getActiveSession()
{
    sendRequest("session/active", "GET");
}

void RestApiClient::createSession(const QString &name)
{
    QJsonObject data;
    data["name"] = name;
    sendRequest("session/create", "POST", data);
}

void RestApiClient::activateSession(const QString &sessionId)
{
    sendRequest(QString("session/activate/%1").arg(sessionId), "PUT");
}

void RestApiClient::closeSession(const QString &sessionId)
{
    sendRequest(QString("session/close/%1").arg(sessionId), "PUT");
}

// Update handleResponse method to handle session endpoints
void RestApiClient::handleResponse(const QString &endpoint, const QJsonDocument &response)
{
    QJsonObject jsonObj = response.object();
    bool success = jsonObj["success"].toBool();

    qDebug() << "Handling response from endpoint:" << endpoint;
    qDebug() << "Response data:" << response.toJson();

    if (!success) {
        QString message = jsonObj["message"].toString();
        emit error(message.isEmpty() ? "Unknown error occurred" : message);
        return;
    }

    if (endpoint == "attendance/list") {
        qDebug() << "Received attendance list response:" << jsonObj;
        if (jsonObj.contains("data") && jsonObj["data"].isArray()) {
            emit attendanceListReceived(jsonObj["data"].toArray());
        } else {
            emit error("Invalid attendance list response format");
        }
    }
    else if (endpoint == "auth/login") {
        if (success) {
            QJsonObject data = jsonObj["data"].toObject();
            m_authToken = data["token"].toString();
            emit loginSuccess(data);
        } else {
            emit loginFailed(jsonObj["message"].toString());
        }
    }
    else if (endpoint == "auth/register") {
        if (success) {
            QJsonObject data = jsonObj["data"].toObject();
            m_authToken = data["token"].toString();
            emit registerSuccess(data);
        } else {
            emit registerFailed(jsonObj["message"].toString());
        }
    }
    else if (endpoint == "session/list") {
        if (success) {
            emit sessionsReceived(jsonObj["data"].toArray());
        }
    }
    else if (endpoint == "session/active") {
        if (success) {
            QJsonValue data = jsonObj["data"];
            if (data.isNull()) {
                emit activeSessionReceived(QJsonObject()); // Empty object for no active session
            } else {
                emit activeSessionReceived(data.toObject());
            }
        }
    }
    else if (endpoint == "session/create") {
        if (success) {
            emit sessionCreated(jsonObj["data"].toObject());
        }
    }
    else if (endpoint.startsWith("session/activate/")) {
        if (success) {
            emit sessionActivated(jsonObj["data"].toObject());
        }
    }
    else if (endpoint == "classroom") {
        if (success) {
            emit classroomsReceived(jsonObj["data"].toArray());
        }
    }
    else if (endpoint.startsWith("classroom") && success) {
        if (jsonObj.contains("data")) {
            emit classroomCreated(jsonObj["data"].toObject());
        } else {
            emit classroomDeleted();
        }
    }
    else if (endpoint.startsWith("attendance")) {
        if (endpoint == "attendance/code") {
            if (success) {
                QJsonObject data = jsonObj["data"].toObject();
                emit attendanceCodeGenerated(
                    data["code"].toString(),
                    data["qr_code"].toString(),
                    data["expiry"].toString()
                    );
            }
        }
    }
    else if (endpoint.startsWith("request")) {
        if (endpoint == "request") {
            if (success) {
                QJsonArray requests = jsonObj["data"].toArray();
                emit requestsReceived(requests);

                // Check for new requests
                for (const QJsonValue &val : requests) {
                    QJsonObject request = val.toObject();
                    QString requestId = request["id"].toString();

                    if (!m_processedRequestIds.contains(requestId)) {
                        m_processedRequestIds.append(requestId);
                        emit newSpeakRequest(
                            request["student_id"].toString(),
                            request["student_name"].toString(),
                            requestId
                            );
                    }
                }
            }
        } else {
            if (success) {
                emit requestCreated(jsonObj["data"].toObject());
            }
        }
    }
    else if (endpoint.startsWith("session/close/")) {
        if (success) {
            QString sessionId = endpoint.mid(QString("session/close/").length());
            emit sessionClosed(sessionId);
            stopPollingRequests();  // Stop polling when session is closed
        }
    }
}

void RestApiClient::getQrCode(const QString &roomId)
{
    // Use the correct endpoint structure
    QString endpoint = QString("qr/%1").arg(roomId);
    QUrl url(m_baseUrl + endpoint);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    if (!m_authToken.isEmpty()) {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(m_authToken).toUtf8());
    }
    
    QNetworkReply *reply = m_networkManager->get(request);
    reply->setProperty("endpoint", "qr");
    
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(responseData);
            
            qDebug() << "QR Code response:" << doc.toJson();
            
            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                if (obj.contains("data") && obj["data"].isObject()) {
                    QJsonObject data = obj["data"].toObject();
                    if (data.contains("qr_code")) {
                        QString qrBase64 = data["qr_code"].toString();
                        emit qrCodeReceived(qrBase64);
                        return;
                    }
                }
            }
            emit error("Invalid QR code response format");
        } else {
            emit error(QString("Failed to get QR code: %1").arg(reply->errorString()));
        }
    });
}
