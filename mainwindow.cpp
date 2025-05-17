#include "mainwindow.h"
#include <QFileDialog>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>
#include <QMessageBox>
#include <QStandardPaths>
#include <QFileInfo>
#include <QProcess>

// Include UI header in implementation file, not in header
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow) // Initialize UI
{
    ui->setupUi(this); // Set up the UI

    // Create subsystems
    m_webSocketClient = new WebSocketClient(this);
    m_presentationManager = new PresentationManager(this);
    m_attendanceManager = new AttendanceManager(this);
    m_gestureProcessor = new GestureProcessor(this);
    m_uiController = new UIController(this);

    setupConnections();
    initialize();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initialize()
{
    // Set window properties
    setWindowTitle("AirClass Desktop");

    // Pass UI elements to managers and controllers - these should be defined in ui_mainwindow.h
    if (ui->studentListWidget && ui->requestListWidget) {
        m_attendanceManager->setStudentListWidget(ui->studentListWidget);
        m_attendanceManager->setRequestListWidget(ui->requestListWidget);
    }

    // Check if pdfView exists in the UI
    if (ui->pdfView) {
        m_presentationManager->setPdfView(ui->pdfView);
    }

    // Set up UI controller with views
    // if (ui->presentationPage && ui->dashboardPage && ui->studentsPage) {
    //     m_uiController->setupViews(ui->presentationPage, ui->dashboardPage, ui->studentsPage);
    // }

    // Setup WebSocket connection
    m_webSocketClient->connectToServer("ws://localhost:12345");  // Static server URL for simplicity

    // Register gesture commands
    m_gestureProcessor->registerGestureCommand("next_slide", [this]() {
        qDebug() << "gesture next";
        m_presentationManager->nextSlide();
    });

    m_gestureProcessor->registerGestureCommand("prev_slide", [this]() {
        m_presentationManager->previousSlide();
    });

    m_gestureProcessor->registerGestureCommand("zoom_in", [this]() {
        m_presentationManager->zoomIn();
    });

    m_gestureProcessor->registerGestureCommand("zoom_out", [this]() {
        m_presentationManager->zoomOut();
    });

    qDebug() << "Application initialized successfully";
}

void MainWindow::setupConnections()
{
    // WebSocket connections
    connect(m_webSocketClient, &WebSocketClient::connected, this, &MainWindow::onConnected);
    connect(m_webSocketClient, &WebSocketClient::disconnected, this, &MainWindow::onDisconnected);
    connect(m_webSocketClient, &WebSocketClient::messageReceived, this, &MainWindow::handleServerMessage);

    // Connect UI buttons to actions - use dynamic property approach to avoid direct access issues
    if (ui->nextButton) {
        connect(ui->nextButton, &QPushButton::clicked, this, &MainWindow::on_nextButton_clicked);
    }

    if (ui->prevButton) {
        connect(ui->prevButton, &QPushButton::clicked, this, &MainWindow::on_prevButton_clicked);
    }

    if (ui->zoomInButton) {
        connect(ui->zoomInButton, &QPushButton::clicked, this, &MainWindow::on_zoomInButton_clicked);
    }

    if (ui->zoomOutButton) {
        connect(ui->zoomOutButton, &QPushButton::clicked, this, &MainWindow::on_zoomOutButton_clicked);
    }

    // Connect approval and rejection buttons with null checks
    if (ui->approveButton && ui->requestListWidget) {
        connect(ui->approveButton, &QPushButton::clicked, [this]() {
            QListWidgetItem *currentItem = ui->requestListWidget->currentItem();
            if (currentItem) {
                QString studentId = currentItem->data(Qt::UserRole).toString();
                m_attendanceManager->grantSpeakPermission(studentId);
            }
        });
    }

    if (ui->rejectButton && ui->requestListWidget) {
        connect(ui->rejectButton, &QPushButton::clicked, [this]() {
            QListWidgetItem *currentItem = ui->requestListWidget->currentItem();
            if (currentItem) {
                QString studentId = currentItem->data(Qt::UserRole).toString();
                // Just remove the raised hand flag, no need to revoke if not granted yet
                Student student = m_attendanceManager->getStudent(studentId);
                student.setRaisedHand(false);
                m_attendanceManager->markAttendance(student);
            }
        });
    }

    //Fix action connections - uncomment if these actions are added to the UI
    // if (ui->menuBar()->findChild<QAction*>("actionDashboard")) {
    //     connect(ui->menuBar()->findChild<QAction*>("actionDashboard"), &QAction::triggered, [this]() {
    //         if (ui->stackedWidget) {
    //             ui->stackedWidget->setCurrentIndex(0);
    //         }
    //     });
    // }

    // if (ui->menuBar()->findChild<QAction*>("actionPresentation")) {
    //     connect(ui->menuBar()->findChild<QAction*>("actionPresentation"), &QAction::triggered, [this]() {
    //         if (ui->stackedWidget) {
    //             ui->stackedWidget->setCurrentIndex(1);
    //         }
    //     });
    // }

    // if (ui->menuBar()->findChild<QAction*>("actionStudents")) {
    //     connect(ui->menuBar()->findChild<QAction*>("actionStudents"), &QAction::triggered, [this]() {
    //         if (ui->stackedWidget) {
    //             ui->stackedWidget->setCurrentIndex(2);
    //         }
    //     });
    // }

    connect(ui->actionDashboard, &QAction::triggered, [this]() {
        ui->stackedWidget->setCurrentIndex(0);
    });
    
    connect(ui->actionPresentation, &QAction::triggered, [this]() {
        ui->stackedWidget->setCurrentIndex(1);
    });
    
    connect(ui->actionStudents, &QAction::triggered, [this]() {
        ui->stackedWidget->setCurrentIndex(2);
    });

    connect(ui->actionGenerate_QR_Code, &QAction::triggered, [this]() {
        ui->stackedWidget->setCurrentIndex(3);
    });
    // Connect qr code
    // connect(ui->actionGenerate_QR_Code, &QPushButton::clicked, [this]() {
        // if (ui->actionGenerate_QR_Code) {
    //         QString qrCodeData = "https://example.com"; // Replace with actual data
    //         m_uiController->showQRCode(qrCodeData);
        // }
    //     ui->stackedWidget->setCurrentIndex(3); // QR Code view index
    //     // add qdebug message
    //     qDebug() << "QR Code generated and displayed";
    // });


    // Connect presentation manager signals
    connect(m_presentationManager, &PresentationManager::error, [this](const QString &errorMessage) {
        QMessageBox::warning(this, tr("PDF Error"), errorMessage);
    });




    connect(m_presentationManager, &PresentationManager::pageChanged, [this](int currentPage, int totalPages) {
        ui->pageIndicatorLabel->setText(tr("Page %1 of %2").arg(currentPage + 1).arg(totalPages));
    });

}

void MainWindow::onConnected()
{
    qDebug() << "Connected to server";
    if (ui->statusbar) {
        ui->statusbar->showMessage(tr("Connected to server"), 3000);
    } else {
        // Use the QMainWindow's statusBar() method
        statusBar()->showMessage(tr("Connected to server"), 3000);
    }

    ui->connectionStatusLabel->setText("Connected");
    ui->connectionStatusLabel->setStyleSheet("color: green;");
    ui->disconnectButton->setEnabled(true);
    ui->connectButton->setEnabled(false);

    // Send identification message
    QJsonObject message;
    message["type"] = "identify";
    message["role"] = "desktop";

    // Check if roomIdEdit exists
    QString roomId = "defaultRoom";
    if (ui->roomIdEdit) {
        roomId = ui->roomIdEdit->text().isEmpty() ? "defaultRoom" : ui->roomIdEdit->text();
    }
    message["roomId"] = roomId;

    QJsonDocument doc(message);
    m_webSocketClient->sendMessage(doc.toJson(QJsonDocument::Compact));
}

void MainWindow::onDisconnected()
{
    qDebug() << "Disconnected from server";
    ui->connectionStatusLabel->setText("Disconnected");
    ui->connectionStatusLabel->setStyleSheet("color: red;");
    ui->disconnectButton->setEnabled(false);
    ui->connectButton->setEnabled(true);
    // Use the QMainWindow's statusBar() method instead of accessing ui->statusBar
    statusBar()->showMessage(tr("Disconnected from server"), 3000);
}

void MainWindow::handleServerMessage(const QString &message)
{
    qDebug() << "Received message from server:" << message;

    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "Invalid JSON message received";
        return;
    }

    QJsonObject jsonObj = doc.object();
    QString messageType = jsonObj["type"].toString();

    if (messageType == "slide_command") {
        handleSlideCommand(jsonObj);
    } else if (messageType == "attendance") {
        handleAttendanceMessage(jsonObj);
    } else if (messageType == "gesture") {
        handleGestureCommand(jsonObj);
    } else if (messageType == "speak_request") {
        handleSpeakRequest(jsonObj);
    } else if (messageType == "room_joined")
    {
        /*
            example message:
            "{\"type\": \"room_joined\", \"room_id\": \"airclass-123\", \"client_id\": \"\", \"qr_code\": \"iVBORw0KGgoAAAANSUhEUgAAATYAAAE2AQAAAADDx4MEAAABeklEQVR4nO2aTW7DIBCFHxCpS3KDHiW5Wm8WDlTJ3mNNNYCbVB1F3iT4570VoE8CMnqe8ThOsESTX4QB5GyRs0XOFjlb5GyRs3VQLrkmYHQO6TzNC+dVnO9wXBRVriN3BVAW5LaS8x2Om5oX1Ckouo/WcL7DcvHBFe/c19bRuNO/lVFNEV+9ryf3nAsiMuggZlR/XERkPec7mD/Ge6poagtxG/fw+4rH5aGNFcQhBtnWPfxOOLSyNuZ5OoQ/06q89nv4vXFBDREE6bOEogRFBInvgz38IZLVGphT+VCc0pI6/dGl3nWIAtcyiQDp+sJ9Pbll/miuyGoSrYHpjz7xmKVZQ/ShVZJ6nTIeb6+v8DutSaRZg/mjV70r5WevrkCNQnl8MR59+7sq+dIvH1ftvPP7R+9+IlCLLJfO3yfZyj38fvu7J0hyIQvGD/YTu+cPmbNGeylk/uhbX91Kq6TWu6yv3s45/j/RFDlb5GyRs0XOFjlb5Gwtra9+AH0iJCEseopPAAAAAElFTkSuQmCC\"}"

            parse it and show qr code image as decoded
        */
        QString qrCodeData = jsonObj["qr_code"].toString();
        if (!qrCodeData.isEmpty()) {
            // Decode the base64 string
            QByteArray decodedData = QByteArray::fromBase64(qrCodeData.toUtf8());
            QPixmap pixmap;
            pixmap.loadFromData(decodedData);

            // Show the QR code in a label or a dialog
            if (ui->qrCodePreviewLabel) {
                ui->qrCodePreviewLabel->setPixmap(pixmap);
                ui->qrCodePreviewLabel->show();
            } else {
                qWarning() << "QR Code label not found in UI";
            }
        } else {
            qWarning() << "QR Code data is empty";
        }
    }
    
    else {
        qWarning() << "Unknown message type:" << messageType;
    }

}

void MainWindow::handleSlideCommand(const QJsonObject &data)
{
    QString command = data["command"].toString();

    if (command == "next") {
        m_presentationManager->nextSlide();
    } else if (command == "previous") {
        m_presentationManager->previousSlide();
    } else if (command == "zoom_in") {
        m_presentationManager->zoomIn();
    } else if (command == "zoom_out") {
        m_presentationManager->zoomOut();
    }
}

void MainWindow::handleAttendanceMessage(const QJsonObject &data)
{
    QString studentId = data["studentId"].toString();
    QString studentName = data["studentName"].toString();

    Student student(studentId, studentName);
    m_attendanceManager->markAttendance(student);
}

void MainWindow::handleGestureCommand(const QJsonObject &data)
{
    QString gesture = data["gesture"].toString();
    m_gestureProcessor->onCommandDetected(gesture);
}

void MainWindow::handleSpeakRequest(const QJsonObject &data)
{
    QString studentId = data["studentId"].toString();
    m_attendanceManager->raiseHand(studentId);
}

void MainWindow::on_actionOpen_triggered()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open Presentation"), "", tr("PDF or PowerPoint Files (*.pdf *.pptx)"));

    if (filePath.isEmpty())
        return;

    QString pathToLoad = filePath;

    // Eğer PPTX ise PDF'e dönüştür
    if (filePath.endsWith(".pptx", Qt::CaseInsensitive)) {
        QString outputDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
        QString fileNameOnly = QFileInfo(filePath).completeBaseName();

        QString sofficePath = "C:/Program Files/LibreOffice/program/soffice.exe";
        QStringList arguments;
        arguments << "--headless"
                  << "--convert-to" << "pdf"
                  << "--outdir" << outputDir
                  << filePath;

        QProcess process;
        process.start("\"" + sofficePath + "\"", arguments);
        bool finished = process.waitForFinished(100000); // 10 saniye bekle

        qDebug() << "LibreOffice output:" << process.readAllStandardOutput();
        qDebug() << "LibreOffice error:" << process.readAllStandardError();

        if (!finished || process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
            QMessageBox::warning(this, tr("Conversion Failed"), tr("LibreOffice PPTX'i PDF'e dönüştüremedi."));
            return;
        }

        // PDF çıktısını dosya adından tahmin et
        QDir dir(outputDir);
        QStringList pdfFiles = dir.entryList(QStringList() << "*.pdf", QDir::Files, QDir::Time);
        if (!pdfFiles.isEmpty()) {
            QString newestPdf = dir.absoluteFilePath(pdfFiles.first());
            pathToLoad = newestPdf;
            qDebug() << "PDF bulundu:" << pathToLoad;
        } else {
            QMessageBox::warning(this, tr("Conversion Failed"), tr("Dönüştürülmüş PDF bulunamadı."));
            return;
        }
    }

    // PDF'i yüklemeye çalış
    bool loaded = m_presentationManager->loadPresentation(pathToLoad);
    if (loaded) {
        if (ui->stackedWidget) {
            ui->stackedWidget->setCurrentIndex(1); // Presentation görünümüne geç
        }
    }
}

void MainWindow::on_actionExit_triggered()
{
    QApplication::quit();
}

void MainWindow::on_actionConnect_triggered()
{
    QString serverUrl;
    if (ui->serverUrlEdit) {
        serverUrl = ui->serverUrlEdit->text();
    }

    if (serverUrl.isEmpty()) {
        QMessageBox::warning(this, tr("Connection Error"), tr("Please enter a server URL"));
        return;
    }
    ui->connectionStatusLabel->setText("Connecting...");
    ui->connectionStatusLabel->setStyleSheet("color: orange;");
    m_webSocketClient->connectToServer(serverUrl);
}

void MainWindow::on_actionDisconnect_triggered()
{
    m_webSocketClient->disconnect();
}

void MainWindow::on_nextButton_clicked()
{
    qDebug() << "NextButton";
    m_presentationManager->nextSlide();
}

void MainWindow::on_prevButton_clicked()
{
    m_presentationManager->previousSlide();
}

void MainWindow::on_zoomInButton_clicked()
{
    m_presentationManager->zoomIn();
}

void MainWindow::on_zoomOutButton_clicked()
{
    m_presentationManager->zoomOut();
}
