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
#include <QTimer>
#include <QPushButton>
#include <QPrinter>
#include <QPrintDialog>
#include <QPainter>
#include <QVBoxLayout>
#include <QLabel>
#include <QIcon>
#include <QHBoxLayout>
#include <QTextStream>
#include <QFile>
#include <QScrollArea>
#include "testdata.h"

// Include UI header in implementation file, not in header
#include "ui_mainwindow.h"
#include "logindialog.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow) // Initialize UI
{
    ui->setupUi(this); // Set up the UI

    // Hide all menus initially
    ui->menuFile->menuAction()->setVisible(false);
    ui->menuConnection->menuAction()->setVisible(false);
    ui->menuView->menuAction()->setVisible(false);

    // Show dashboard by default after login
    if (ui->stackedWidget && ui->dashboardView)
        ui->stackedWidget->setCurrentWidget(ui->dashboardView);

    // Add Log out button to topBar if not already present
    QPushButton *logoutButton = new QPushButton("Log out", ui->topBar);
    logoutButton->setObjectName("logoutButton");
    logoutButton->setStyleSheet("QPushButton { color: #e74c3c; font-weight: bold; background: transparent; border: 1px solid #e74c3c; border-radius: 6px; padding: 6px 16px; } QPushButton:hover { background: #fbeee6; }");
    logoutButton->setVisible(false); // Initially hidden
    ui->horizontalLayout->addWidget(logoutButton);
    connect(logoutButton, &QPushButton::clicked, this, [this, logoutButton]() {
        // Hide all menus
        ui->menuFile->menuAction()->setVisible(false);
        ui->menuConnection->menuAction()->setVisible(false);
        ui->menuView->menuAction()->setVisible(false);
        logoutButton->setVisible(false);

        // Create and show login dialog first
        LoginDialog loginDialog;
        loginDialog.show();
        
        // Then hide main window
        this->hide();
        
        if (loginDialog.exec() == QDialog::Accepted) {
            // On successful login, show all menus
            ui->menuFile->menuAction()->setVisible(true);
            ui->menuConnection->menuAction()->setVisible(true);
            ui->menuView->menuAction()->setVisible(true);
            logoutButton->setVisible(true);

            // Show dashboard again
            if (ui->stackedWidget && ui->dashboardView)
                ui->stackedWidget->setCurrentWidget(ui->dashboardView);
            this->show();
        } else {
            // If login cancelled, close the app
            this->close();
        }
    });

    // Show login dialog
    LoginDialog loginDialog(this);
    if (loginDialog.exec() == QDialog::Accepted) {
        // Show all menus after successful login
        ui->menuFile->menuAction()->setVisible(true);
        ui->menuConnection->menuAction()->setVisible(true);
        ui->menuView->menuAction()->setVisible(true);
        logoutButton->setVisible(true);
        QMainWindow::showFullScreen(); // Ensure MainWindow is full screen after login
    } else {
        QTimer::singleShot(0, this, &QWidget::close);
        return;
    }
    
    // Get login credentials
    QString email = loginDialog.getEmail();
    QString password = loginDialog.getPassword();
    
    // TODO: Implement actual authentication logic here

    // Create subsystems
    m_webSocketClient = new WebSocketClient(this);
    m_presentationManager = new PresentationManager(this);
    m_attendanceManager = new AttendanceManager(this);
    m_gestureProcessor = new GestureProcessor(this);
    m_uiController = new UIController(this);

    setupConnections();
    initialize();

    // Connect recent presentations list click
    connect(ui->recentFilesList, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        QString filePath = item->data(Qt::UserRole).toString();
        if (!filePath.isEmpty()) {
            bool loaded = m_presentationManager->loadPresentation(filePath);
            if (loaded && ui->stackedWidget) {
                ui->stackedWidget->setCurrentIndex(1); // Presentation view
            }
        }
    });

    // Add Home button to Presentation view
    QHBoxLayout *homeLayout = new QHBoxLayout();
    homeLayout->setContentsMargins(10, 10, 0, 0);
    QPushButton *homeButton = new QPushButton(ui->presentationView);
    homeButton->setIcon(QIcon::fromTheme("go-home", QIcon(":/icons/home.png")));
    homeButton->setIconSize(QSize(24, 24));
    homeButton->setFixedSize(40, 40);
    homeButton->setStyleSheet(R"(
        QPushButton {
            background-color: transparent;
            border: none;
            border-radius: 20px;
            padding: 8px;
        }
        QPushButton:hover {
            background-color: rgba(74, 144, 226, 0.1);
        }
        QPushButton:pressed {
            background-color: rgba(74, 144, 226, 0.2);
        }
    )");
    connect(homeButton, &QPushButton::clicked, this, [this]() {
        ui->stackedWidget->setCurrentIndex(0);
    });
    homeLayout->addWidget(homeButton);
    homeLayout->addStretch();
    ui->presentationLayout->insertLayout(0, homeLayout);

    // Add Home button to Students view
    QHBoxLayout *homeLayoutStudents = new QHBoxLayout();
    homeLayoutStudents->setContentsMargins(10, 10, 0, 0);
    QPushButton *homeButtonStudents = new QPushButton(ui->studentsView);
    homeButtonStudents->setIcon(QIcon::fromTheme("go-home", QIcon(":/icons/home.png")));
    homeButtonStudents->setIconSize(QSize(24, 24));
    homeButtonStudents->setFixedSize(40, 40);
    homeButtonStudents->setStyleSheet(R"(
        QPushButton {
            background-color: transparent;
            border: none;
            border-radius: 20px;
            padding: 8px;
        }
        QPushButton:hover {
            background-color: rgba(74, 144, 226, 0.1);
        }
        QPushButton:pressed {
            background-color: rgba(74, 144, 226, 0.2);
        }
    )");
    connect(homeButtonStudents, &QPushButton::clicked, this, [this]() {
        ui->stackedWidget->setCurrentIndex(0);
    });
    homeLayoutStudents->addWidget(homeButtonStudents);
    homeLayoutStudents->addStretch();
    ui->studentsLayout->insertLayout(0, homeLayoutStudents);

    // Add Home button to QR Code view
    QHBoxLayout *homeLayoutQR = new QHBoxLayout();
    homeLayoutQR->setContentsMargins(10, 10, 0, 0);
    QPushButton *homeButtonQR = new QPushButton(ui->qrCodeView);
    homeButtonQR->setIcon(QIcon::fromTheme("go-home", QIcon(":/icons/home.png")));
    homeButtonQR->setIconSize(QSize(24, 24));
    homeButtonQR->setFixedSize(40, 40);
    homeButtonQR->setStyleSheet(R"(
        QPushButton {
            background-color: transparent;
            border: none;
            border-radius: 20px;
            padding: 8px;
        }
        QPushButton:hover {
            background-color: rgba(74, 144, 226, 0.1);
        }
        QPushButton:pressed {
            background-color: rgba(74, 144, 226, 0.2);
        }
    )");
    connect(homeButtonQR, &QPushButton::clicked, this, [this]() {
        ui->stackedWidget->setCurrentIndex(0);
    });
    homeLayoutQR->addWidget(homeButtonQR);
    homeLayoutQR->addStretch();
    ui->qrCodeLayout->insertLayout(0, homeLayoutQR);

    // Add download attendance button to Students view
    QPushButton *downloadAttendanceButton = new QPushButton(ui->attendanceGroupBox);
    downloadAttendanceButton->setIcon(QIcon::fromTheme("document-save", QIcon(":/icons/download.png")));
    downloadAttendanceButton->setText("Download Attendance");
    downloadAttendanceButton->setStyleSheet(R"(
        QPushButton {
            padding: 8px 16px;
            background-color: #6c757d;
            color: white;
            border: none;
            border-radius: 6px;
            font-size: 14px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #5a6268;
        }
        QPushButton:pressed {
            background-color: #545b62;
        }
    )");
    connect(downloadAttendanceButton, &QPushButton::clicked, this, [this]() {
        // Get test attendance data from TestData class
        QStringList testData = TestData::getTestAttendanceData();

        // Get save file path
        QString fileName = QFileDialog::getSaveFileName(this,
            "Save Attendance Report", "",
            "CSV Files (*.csv);;All Files (*)");

        if (fileName.isEmpty())
            return;

        // Add .csv extension if not present
        if (!fileName.endsWith(".csv", Qt::CaseInsensitive))
            fileName += ".csv";

        // Save to file
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream(&file);
            for (const QString &line : testData) {
                stream << line << "\n";
            }
            file.close();
            QMessageBox::information(this, "Success", "Attendance report has been saved successfully!");
        } else {
            QMessageBox::warning(this, "Error", "Could not save the attendance report!");
        }
    });

    // Add the button to the attendance group box layout
    QVBoxLayout *attendanceLayout = qobject_cast<QVBoxLayout*>(ui->attendanceGroupBox->layout());
    if (attendanceLayout) {
        attendanceLayout->addWidget(downloadAttendanceButton);
    }

    // Add test data to the student list from TestData class
    QStringList testStudents = TestData::getTestStudents();
    for (const QString &student : testStudents) {
        ui->studentListWidget->addItem(student);
    }

    // Add test speak requests
    QStringList testRequests = TestData::getTestSpeakRequests();
    for (const QString &request : testRequests) {
        QStringList parts = request.split(',');
        if (parts.size() >= 3) {
            QString studentId = parts[0];
            QString studentName = parts[1];
            QString message = parts[2];
            
            QListWidgetItem *item = new QListWidgetItem(
                QString("%1: %2").arg(studentName, message)
            );
            item->setData(Qt::UserRole, studentId);
            ui->requestListWidget->addItem(item);
        }
    }

    // Update approve/reject button connections
    if (ui->approveButton && ui->requestListWidget) {
        connect(ui->approveButton, &QPushButton::clicked, [this]() {
            QListWidgetItem *currentItem = ui->requestListWidget->currentItem();
            if (currentItem) {
                QString studentId = currentItem->data(Qt::UserRole).toString();
                QString studentName = currentItem->text().split(':')[0];
                
                // Remove from request list
                delete ui->requestListWidget->takeItem(ui->requestListWidget->row(currentItem));
                
                // Show approval message
                QMessageBox::information(this, "Approved", 
                    QString("Speak request approved for %1").arg(studentName));
            }
        });
    }

    if (ui->rejectButton && ui->requestListWidget) {
        connect(ui->rejectButton, &QPushButton::clicked, [this]() {
            QListWidgetItem *currentItem = ui->requestListWidget->currentItem();
            if (currentItem) {
                QString studentId = currentItem->data(Qt::UserRole).toString();
                QString studentName = currentItem->text().split(':')[0];
                
                // Remove from request list
                delete ui->requestListWidget->takeItem(ui->requestListWidget->row(currentItem));
                
                // Show rejection message
                QMessageBox::information(this, "Rejected", 
                    QString("Speak request rejected for %1").arg(studentName));
            }
        });
    }
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

    // Add Gesture Guide action
    QAction *actionGestureGuide = new QAction("Gesture Guide", this);
    ui->menuView->addAction(actionGestureGuide);
    connect(actionGestureGuide, &QAction::triggered, [this]() {
        QDialog *gestureDialog = new QDialog(this);
        gestureDialog->setWindowTitle("Gesture Guide");
        gestureDialog->setMinimumSize(400, 500);

        QVBoxLayout *layout = new QVBoxLayout(gestureDialog);
        
        // Create a scroll area for gestures
        QScrollArea *scrollArea = new QScrollArea(gestureDialog);
        QWidget *scrollContent = new QWidget(scrollArea);
        QVBoxLayout *scrollLayout = new QVBoxLayout(scrollContent);

        // Add gesture items
        struct GestureItem {
            QString emoji;
            QString name;
            QString description;
        };

        QList<GestureItem> gestures = {
            {"👎", "Dislike", "Reject speak request from mobile"},
            {"👍", "Like", "Approve speak request from mobile"},
            {"🤫", "Mute", "Play alarm sound to get attention and maintain silence"},
            {"☝️", "One", "Draw mode"},
            {"✌️", "Two Up", "Pointer mode"},
            {"⏰", "Timeout", "Start 15-minute countdown"},
            {"➡️", "Right", "Previous slide"},
            {"⬅️", "Left", "Next slide"}
        };

        for (const auto &gesture : gestures) {
            QWidget *gestureWidget = new QWidget(scrollContent);
            QHBoxLayout *gestureLayout = new QHBoxLayout(gestureWidget);
            
            QLabel *emojiLabel = new QLabel(gesture.emoji, gestureWidget);
            emojiLabel->setStyleSheet("font-size: 24px;");

            QLabel *nameLabel = new QLabel(gesture.name, gestureWidget);
            nameLabel->setStyleSheet("font-weight: bold; font-size: 14px;");
            
            QLabel *descLabel = new QLabel(gesture.description, gestureWidget);
            descLabel->setWordWrap(true);
            
            gestureLayout->addWidget(emojiLabel);
            gestureLayout->addWidget(nameLabel);
            gestureLayout->addWidget(descLabel);
            gestureLayout->setStretch(2, 1); // Make description take more space
            
            scrollLayout->addWidget(gestureWidget);
        }

        scrollContent->setLayout(scrollLayout);
        scrollArea->setWidget(scrollContent);
        scrollArea->setWidgetResizable(true);
        layout->addWidget(scrollArea);

        // Add close button
        QPushButton *closeButton = new QPushButton("Close", gestureDialog);
        closeButton->setStyleSheet(R"(
            QPushButton {
                padding: 8px 16px;
                background-color: #6c757d;
                color: white;
                border: none;
                border-radius: 6px;
                font-size: 14px;
                font-weight: bold;
            }
            QPushButton:hover {
                background-color: #5a6268;
            }
            QPushButton:pressed {
                background-color: #545b62;
            }
        )");
        connect(closeButton, &QPushButton::clicked, gestureDialog, &QDialog::close);
        layout->addWidget(closeButton, 0, Qt::AlignCenter);

        gestureDialog->setLayout(layout);
        gestureDialog->exec();
    });

    // Add timeout functionality
    QAction *actionTimeout = new QAction("Timeout", this);
    actionTimeout->setShortcut(QKeySequence("Ctrl+T")); // Add keyboard shortcut
    ui->menuView->addAction(actionTimeout);
    
    connect(actionTimeout, &QAction::triggered, [this]() {
        QDialog *timeoutDialog = new QDialog(this);
        timeoutDialog->setWindowTitle("Timeout");
        timeoutDialog->setFixedSize(300, 200); // Increase size to accommodate buttons
        
        QVBoxLayout *layout = new QVBoxLayout(timeoutDialog);
        
        QLabel *timeLabel = new QLabel("15:00", timeoutDialog);
        timeLabel->setStyleSheet(R"(
            QLabel {
                font-size: 48px;
                font-weight: bold;
                color: #dc3545;
                qproperty-alignment: AlignCenter;
            }
        )");
        
        QHBoxLayout *controlLayout = new QHBoxLayout();
        QPushButton *decreaseButton = new QPushButton("-", timeoutDialog);
        QPushButton *increaseButton = new QPushButton("+", timeoutDialog);

        decreaseButton->setFixedSize(40, 40);
        increaseButton->setFixedSize(40, 40);

        QString controlButtonStyle = R"(
            QPushButton {
                font-size: 24px;
                font-weight: bold;
                padding: 5px;
                border: 1px solid #cccccc;
                border-radius: 4px;
                background-color: #f8f9fa;
            }
            QPushButton:hover {
                background-color: #e2e6ea;
            }
            QPushButton:pressed {
                background-color: #dae0e5;
            }
        )";

        decreaseButton->setStyleSheet(controlButtonStyle);
        increaseButton->setStyleSheet(controlButtonStyle);

        controlLayout->addStretch();
        controlLayout->addWidget(decreaseButton);
        controlLayout->addWidget(increaseButton);
        controlLayout->addStretch();

        QPushButton *startButton = new QPushButton("Start", timeoutDialog);
        startButton->setStyleSheet(R"(
            QPushButton {
                padding: 8px 16px;
                background-color: #dc3545;
                color: white;
                border: none;
                border-radius: 6px;
                font-size: 14px;
                font-weight: bold;
            }
            QPushButton:hover {
                background-color: #c82333;
            }
            QPushButton:pressed {
                background-color: #bd2130;
            }
        )");
        
        layout->addWidget(timeLabel);
        layout->addLayout(controlLayout);
        layout->addWidget(startButton);
        
        QTimer *timer = new QTimer(timeoutDialog);
        int initialRemainingSeconds = 15 * 60; // Default 15 minutes
        int remainingSeconds = initialRemainingSeconds;
        
        // Update the displayed time
        auto updateTimerDisplay = [=](QLabel* label, int seconds) {
            int minutes = seconds / 60;
            int secs = seconds % 60;
            label->setText(QString("%1:%2")
                .arg(minutes, 2, 10, QChar('0'))
                .arg(secs, 2, 10, QChar('0')));
        };

        // Connect decrease/increase buttons
        connect(decreaseButton, &QPushButton::clicked, [=]() mutable {
            if (!timer->isActive() && remainingSeconds > 60) { // Minimum 1 minute
                remainingSeconds -= 60;
                updateTimerDisplay(timeLabel, remainingSeconds);
            }
        });

        connect(increaseButton, &QPushButton::clicked, [=]() mutable {
             if (!timer->isActive() && remainingSeconds < 3600) { // Maximum 60 minutes
                remainingSeconds += 60;
                updateTimerDisplay(timeLabel, remainingSeconds);
            }
        });

        connect(startButton, &QPushButton::clicked, [=]() mutable {
            if (timer->isActive()) {
                timer->stop();
                startButton->setText("Start");
                remainingSeconds = initialRemainingSeconds; // Reset on Stop
                updateTimerDisplay(timeLabel, remainingSeconds);
            } else {
                initialRemainingSeconds = remainingSeconds; // Save current duration as initial
                timer->start(1000); // Update every second
                startButton->setText("Stop");
            }
        });
        
        connect(timer, &QTimer::timeout, [=]() mutable {
            remainingSeconds--;
            if (remainingSeconds < 0) {
                timer->stop();
                startButton->setText("Start");
                remainingSeconds = initialRemainingSeconds; // Reset after timeout
                updateTimerDisplay(timeLabel, remainingSeconds);
                QMessageBox::information(timeoutDialog, "Timeout", "Time's up!");
                return;
            }
            updateTimerDisplay(timeLabel, remainingSeconds);
        });
        
        updateTimerDisplay(timeLabel, remainingSeconds); // Initial display

        timeoutDialog->setLayout(layout);
        timeoutDialog->exec();
    });

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
        addRecentPresentation(pathToLoad);
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

void MainWindow::on_saveImageButton_clicked()
{
    QPixmap pixmap = ui->qrCodePreviewLabel->pixmap();
    if (pixmap.isNull()) {
        QMessageBox::warning(this, "Error", "No QR code to save");
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this,
        "Save QR Code", "",
        "PNG Images (*.png);;JPEG Images (*.jpg);;All Files (*)");

    if (fileName.isEmpty())
        return;

    if (!pixmap.save(fileName)) {
        QMessageBox::warning(this, "Error", "Failed to save QR code image");
    }
}

void MainWindow::on_printButton_clicked()
{
    QPixmap pixmap = ui->qrCodePreviewLabel->pixmap();
    if (pixmap.isNull()) {
        QMessageBox::warning(this, "Error", "No QR code to print");
        return;
    }

    QPrinter printer(QPrinter::HighResolution);
    printer.setPageSize(QPageSize(QPageSize::A4));
    printer.setPageOrientation(QPageLayout::Portrait);

    QPrintDialog printDialog(&printer, this);
    if (printDialog.exec() == QDialog::Accepted) {
        QPainter painter(&printer);
        QRect rect = painter.viewport();
        QSize size = pixmap.size();
        size.scale(rect.size(), Qt::KeepAspectRatio);
        painter.setViewport(rect.x(), rect.y(), size.width(), size.height());
        painter.setWindow(pixmap.rect());
        painter.drawPixmap(0, 0, pixmap);
    }
}

void MainWindow::on_displayFullScreenButton_clicked()
{
    QPixmap pixmap = ui->qrCodePreviewLabel->pixmap();
    if (pixmap.isNull()) {
        QMessageBox::warning(this, "Error", "No QR code to display");
        return;
    }

    QDialog *fullScreenDialog = new QDialog(this, Qt::Window | Qt::WindowStaysOnTopHint);
    fullScreenDialog->setWindowState(Qt::WindowFullScreen);
    fullScreenDialog->setStyleSheet("background-color: white;");

    QVBoxLayout *layout = new QVBoxLayout(fullScreenDialog);
    QLabel *label = new QLabel(fullScreenDialog);
    label->setPixmap(pixmap);
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);

    QPushButton *closeButton = new QPushButton("Close", fullScreenDialog);
    closeButton->setStyleSheet("QPushButton { padding: 10px; font-size: 14px; }");
    connect(closeButton, &QPushButton::clicked, fullScreenDialog, &QDialog::close);
    layout->addWidget(closeButton, 0, Qt::AlignCenter);

    fullScreenDialog->show();
}

void MainWindow::addRecentPresentation(const QString &filePath)
{
    if (filePath.isEmpty()) return;
    m_recentPresentations.removeAll(filePath); // Remove if already exists
    m_recentPresentations.prepend(filePath); // Add to top
    if (m_recentPresentations.size() > 10) // Limit to 10
        m_recentPresentations = m_recentPresentations.mid(0, 10);
    // Update UI
    ui->recentFilesList->clear();
    for (const QString &path : m_recentPresentations) {
        QListWidgetItem *item = new QListWidgetItem(QFileInfo(path).fileName());
        item->setToolTip(path);
        item->setData(Qt::UserRole, path);
        ui->recentFilesList->addItem(item);
    }
}

void MainWindow::showFullScreen()
{
    QMainWindow::showFullScreen();
}
