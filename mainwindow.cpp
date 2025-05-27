#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>
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
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QUrl>
#include <QDateTime>
#include <QSettings>
#include <QColorDialog>
#include <QComboBox>
#include <QSlider>
#include "testdata.h"
#include "logindialog.h"
#include "sessiondialog.h"


// Include UI header in implementation file, not in header
#include "ui_mainwindow.h"
#include "logindialog.h"

MainWindow::MainWindow(RestApiClient* restApi, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_restApi(restApi)
    , m_timer(new QTimer(this))
    , m_isTimerRunning(false)
    , m_remainingSeconds(0)
{
    ui->setupUi(this);

    // Clear existing menu items first
    ui->menuView->clear();
    
    // Add menu items
    ui->menuView->addAction(ui->actionDashboard);
    ui->menuView->addAction(ui->actionPresentation);
    ui->menuView->addAction(ui->actionStudents);
    ui->menuView->addAction(ui->actionGenerate_QR_Code);

    // Hide all menus initially
    ui->menuFile->menuAction()->setVisible(false);
    ui->menuConnection->menuAction()->setVisible(false);
    ui->menuView->menuAction()->setVisible(false);

    // Hide server connection and class information sections in dashboard
    if (ui->serverGroupBox) {
        ui->serverGroupBox->setVisible(false);
    }
    if (ui->classInfoGroupBox) {
        ui->classInfoGroupBox->setVisible(false);
    }

    // Show dashboard by default after login
    if (ui->stackedWidget && ui->dashboardView)
        ui->stackedWidget->setCurrentWidget(ui->dashboardView);

    // Create subsystems
    m_webSocketClient = new WebSocketClient(this);
    m_presentationManager = new PresentationManager(this);
    m_attendanceManager = new AttendanceManager(this);
    m_gestureProcessor = new GestureProcessor(this);
    m_uiController = new UIController(this);

    // Connect WebSocket gesture signals
    connect(m_webSocketClient, &WebSocketClient::gestureReceived, 
            this, [this](const QString &gestureType, const QString &clientId, const QString &timestamp) {
        qDebug() << "Gesture received in MainWindow:" << gestureType;
        QJsonObject data;
        data["gesture_type"] = gestureType;
        data["client_id"] = clientId;
        data["timestamp"] = timestamp;
        handleGestureCommand(data);
    });

    qDebug() << "MainWindow initialized with REST API client";
    // Connect WebSocket signals for connection status
    connect(m_webSocketClient, &WebSocketClient::connected, this, [this]() {
        qDebug() << "WebSocket connected";
        if (ui->connectionStatusLabel) {
            // Update connection status label
            qDebug() << "Updating connection status label to 'Connected'";
            //ui->connectionStatusLabel->setText("Connected");
            //ui->connectionStatusLabel->setStyleSheet("color: green;");
        }
        qDebug() << "WebSocket connection established, setting auth token";
    });

    qDebug() << "Connecting WebSocket disconnected signal";

    connect(m_webSocketClient, &WebSocketClient::disconnected, this, [this]() {
        qDebug() << "WebSocket disconnected";
        if (ui && ui->connectionStatusLabel) {
            qDebug() << "Updating connection status label to 'Disconnected'";
            //ui->connectionStatusLabel->setText("Disconnected");
            //ui->connectionStatusLabel->setStyleSheet("color: red;");
            qDebug() << "disconnected hale getirme";
        }
    });
    qDebug() << "WebSocket disconnected signal connected";

    // Qt 6 için QMediaPlayer başlatma
    m_notificationPlayer = new QMediaPlayer(this);
    QAudioOutput *audioOutput = new QAudioOutput(this);
    m_notificationPlayer->setAudioOutput(audioOutput);
    audioOutput->setVolume(0.7f);

    // Hide notification widget initially
    if (ui->notificationWidget) {
        ui->notificationWidget->setVisible(false);
    }

    // Set current room ID
    m_currentRoomId = "580369";

    // Add Log out button to topBar
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

        // Clear auth token
        m_authToken.clear();
        m_restApi->setAuthToken("");

        // Show login dialog
        showLoginDialog();
    });

    // Setup REST API connections
    setupRestApiConnections();

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

    // Show login dialog
    showLoginDialog();

    // Initialize timer
    m_timer = new QTimer(this);
    m_isTimerRunning = false;
    m_remainingSeconds = 0;
    
    connect(m_timer, &QTimer::timeout, this, &MainWindow::updateTimer);
    connect(ui->timerStartButton, &QPushButton::clicked, this, &MainWindow::on_timerStartButton_clicked);
    connect(ui->timerStopButton, &QPushButton::clicked, this, &MainWindow::on_timerStopButton_clicked);
    connect(ui->timerResetButton, &QPushButton::clicked, this, &MainWindow::on_timerResetButton_clicked);
    connect(ui->timerMinutesSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::on_timerMinutesSpinBox_valueChanged);
    connect(ui->timerSecondsSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::on_timerSecondsSpinBox_valueChanged);

    // Initialize timer display and controls
    ui->timerStopButton->setEnabled(false);
    ui->timerMinutesSpinBox->setRange(0, 59);
    ui->timerSecondsSpinBox->setRange(0, 59);
    updateTimerDisplay();
}

// Update showLoginDialog method in mainwindow.cpp
void MainWindow::showLoginDialog()
{
    // Clear menu items before showing login dialog
    ui->menuView->clear();
    
    // Add menu items back
    ui->menuView->addAction(ui->actionDashboard);
    ui->menuView->addAction(ui->actionPresentation);
    ui->menuView->addAction(ui->actionStudents);
    ui->menuView->addAction(ui->actionGenerate_QR_Code);

    // Hide all menus
    ui->menuFile->menuAction()->setVisible(false);
    ui->menuConnection->menuAction()->setVisible(false);
    ui->menuView->menuAction()->setVisible(false);
    
    // Create and show login dialog
    LoginDialog loginDialog(m_restApi, this);
    
    // Connect login signals
    connect(&loginDialog, &LoginDialog::loginSuccessful, 
            this, [this](const QString &username, const QString &token) {
        m_userName = username;
        m_authToken = token;
        m_restApi->setAuthToken(m_authToken);
        qDebug() << "Login successful for user:" << m_userName << "with token:" << m_authToken;

        // Show session dialog after successful login
        showSessionDialog();
    });

    // Hide main window while showing login dialog
    this->hide();

    // Show login dialog
    if (loginDialog.exec() != QDialog::Accepted) {
        // If login was cancelled or failed, close the application
        QTimer::singleShot(0, this, &QWidget::close);
    }

    // Set initial timer display
    updateTimerDisplay();

    // Connect WebSocket drawing signals to DrawingLayer
    connect(m_webSocketClient, &WebSocketClient::drawingReceived,
            this, [this](double x, double y, bool isStart, const QString &color, int width) {
                QPointF point(x, y);
                m_drawingLayer->setPenColor(QColor(color));
                m_drawingLayer->setPenWidth(width);
                m_drawingLayer->drawRemotePoint(point, isStart);
            });
}

// Add new method to show session dialog
void MainWindow::showSessionDialog()
{
    qDebug() << "Showing session dialog for user:" << m_userName;
    SessionDialog sessionDialog(m_restApi, this);
    
    connect(&sessionDialog, &SessionDialog::sessionSelected,
            this, [this](const QString &sessionId, const QString &sessionName, const QString &sessionCode) {
        qDebug() << "Session selected:" << sessionName << "Code:" << sessionCode;

        m_webSocketClient->connectToServer("ws://localhost:8082");

        // qDebug() << "Connecting to WebSocket server for session:" << session
        
        // Update current room/session info
        m_currentRoomId = sessionCode; // Use session code as room ID
        m_sessionId = sessionId;
        m_sessionName = sessionName;
        m_sessionCode = sessionCode;
        
        // Update window title and UI
        updateSessionInfo(sessionId, sessionName, sessionCode);
        
        // Show all menus after successful session selection
        ui->menuFile->menuAction()->setVisible(true);
        ui->menuConnection->menuAction()->setVisible(true);
        ui->menuView->menuAction()->setVisible(true);

        // Show logout button
        if (QWidget *logoutButton = findChild<QWidget*>("logoutButton")) {
            logoutButton->setVisible(true);
        }

        // Show the main window
        this->show();

        // Initialize the UI and setup connections
        initialize();
        setupConnections();

        // Just get requests once instead of continuous polling
        m_restApi->listRequests(m_currentRoomId);
    });
    
    // Show session dialog
    if (sessionDialog.exec() != QDialog::Accepted) {
        // If session selection was cancelled, go back to login
        showLoginDialog();
    }
}

void MainWindow::setupRestApiConnections()
{
    // Connect REST API signals
    connect(m_restApi, &RestApiClient::loginSuccess, this, &MainWindow::onLoginSuccess);
    connect(m_restApi, &RestApiClient::loginFailed, this, &MainWindow::onLoginFailed);
    connect(m_restApi, &RestApiClient::attendanceListReceived, this, &MainWindow::onAttendanceListReceived);
    connect(m_restApi, &RestApiClient::attendanceCodeGenerated, this, &MainWindow::onAttendanceCodeGenerated);
    connect(m_restApi, &RestApiClient::requestsReceived, this, &MainWindow::onRequestsReceived);
    connect(m_restApi, &RestApiClient::newSpeakRequest, this, &MainWindow::onNewSpeakRequest);
    connect(m_restApi, &RestApiClient::requestUpdated, this, &MainWindow::onRequestUpdated);
    connect(m_restApi, &RestApiClient::sessionClosed, this, &MainWindow::onSessionClosed);
    connect(m_restApi, &RestApiClient::qrCodeReceived, this, &MainWindow::onQrCodeReceived);
    // Connect close session button
    connect(ui->closeSessionButton, &QPushButton::clicked, this, &MainWindow::closeSession);
}

void MainWindow::onLoginSuccess(const QJsonObject &data)
{
    m_userName = data["name"].toString();
    m_authToken = data["token"].toString();
    m_restApi->setAuthToken(m_authToken);
}

void MainWindow::onLoginFailed(const QString &message)
{
    QMessageBox::warning(this, "Login Failed", message);
}

void MainWindow::initialize()
{
    // Set window properties
    setWindowTitle("AirClass Desktop");

    // Pass UI elements to managers and controllers
    if (ui->studentListWidget && ui->requestListWidget) {
        m_attendanceManager->setStudentListWidget(ui->studentListWidget);
        m_attendanceManager->setRequestListWidget(ui->requestListWidget);
    }

    // Add Download Attendance and Refresh buttons to attendance group box
    if (ui->attendanceGroupBox) {
        QHBoxLayout *attendanceButtonsLayout = new QHBoxLayout();
        
        // Create Download Attendance button
        QPushButton *downloadAttendanceButton = new QPushButton(ui->attendanceGroupBox);
        downloadAttendanceButton->setText("Download Attendance");
        downloadAttendanceButton->setStyleSheet("QPushButton { background-color: #2ecc71; color: white; border: none; padding: 8px 16px; border-radius: 4px; } QPushButton:hover { background-color: #27ae60; }");
        connect(downloadAttendanceButton, &QPushButton::clicked, this, &MainWindow::downloadAttendanceReport);
        attendanceButtonsLayout->addWidget(downloadAttendanceButton);

        // Create Refresh button
        QPushButton *refreshAttendanceButton = new QPushButton(ui->attendanceGroupBox);
        refreshAttendanceButton->setText("Refresh");
        refreshAttendanceButton->setStyleSheet("QPushButton { background-color: #3498db; color: white; border: none; padding: 8px 16px; border-radius: 4px; } QPushButton:hover { background-color: #2980b9; }");
        connect(refreshAttendanceButton, &QPushButton::clicked, this, [this]() {
            if (!m_currentRoomId.isEmpty()) {
                m_restApi->listAttendance(m_currentRoomId);
            }
        });
        attendanceButtonsLayout->addWidget(refreshAttendanceButton);

        // Add the buttons layout to the attendance group box
        if (QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(ui->attendanceGroupBox->layout())) {
            layout->addLayout(attendanceButtonsLayout);
        }
    }

    // Initialize PDF view
    if (ui->pdfView) {
        // Configure PDF view settings
        ui->pdfView->setPageMode(QPdfView::PageMode::SinglePage);
        ui->pdfView->setZoomMode(QPdfView::ZoomMode::Custom);
        
        // Create and setup drawing layer
        m_drawingLayer = new DrawingLayer(ui->pdfView);
        m_drawingLayer->setGeometry(ui->pdfView->geometry());
        m_drawingLayer->show();
        
        // Install event filter to handle PDF view resize events
        ui->pdfView->installEventFilter(this);
        
        // Create drawing controls layout
        QHBoxLayout *drawingControlsLayout = new QHBoxLayout();
        drawingControlsLayout->setObjectName("drawingControlsLayout");
        
        // Common button style
        QString buttonStyle = R"(
            QPushButton {
                background-color: white;
                color: #333;
                border: 1px solid #e0e0e0;
                padding: 8px;
                border-radius: 8px;
                font-size: 18px;
                min-width: 44px;
                min-height: 44px;
                max-width: 44px;
                max-height: 44px;
                box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
            }
            QPushButton:hover {
                background-color: #f8f9fa;
                border-color: #bdbdbd;
                box-shadow: 0 4px 8px rgba(0, 0, 0, 0.15);
            }
            QPushButton:pressed {
                background-color: #f1f3f4;
                border-color: #9e9e9e;
                box-shadow: 0 1px 2px rgba(0, 0, 0, 0.1);
            }
        )";
        
        // Create clear button with emoji
        QPushButton *clearButton = new QPushButton("🗑️", this);
        clearButton->setObjectName("clearDrawingButton");
        clearButton->setToolTip("Clear Drawing");
        clearButton->setStyleSheet(buttonStyle);
        connect(clearButton, &QPushButton::clicked, this, [this]() {
            if (m_drawingLayer) {
                m_drawingLayer->clear();
            }
        });
        drawingControlsLayout->addWidget(clearButton);
        
        // Create drawing toggle button with emoji
        QPushButton *drawButton = new QPushButton("✏️", this);
        drawButton->setObjectName("drawButton");
        drawButton->setToolTip("Toggle Drawing");
        drawButton->setCheckable(true);
        drawButton->setChecked(true);
        drawButton->setStyleSheet(buttonStyle + R"(
            QPushButton:checked {
                background-color: #e8f5e9;
                border-color: #66bb6a;
                color: #2e7d32;
            }
            QPushButton:checked:hover {
                background-color: #c8e6c9;
                border-color: #43a047;
            }
            QPushButton:checked:pressed {
                background-color: #a5d6a7;
                border-color: #2e7d32;
            }
        )");
        connect(drawButton, &QPushButton::clicked, this, [this, drawButton]() {
            if (m_drawingLayer) {
                m_drawingLayer->setDrawingEnabled(drawButton->isChecked());
            }
        });
        drawingControlsLayout->addWidget(drawButton);
        
        // Create color button with emoji
        QPushButton *colorButton = new QPushButton("🎨", this);
        colorButton->setObjectName("colorButton");
        colorButton->setToolTip("Select Color");
        colorButton->setStyleSheet(buttonStyle);
        connect(colorButton, &QPushButton::clicked, this, [this]() {
            if (m_drawingLayer) {
                QColor color = QColorDialog::getColor(m_drawingLayer->getPenColor(), this, "Select Pen Color");
                if (color.isValid()) {
                    m_drawingLayer->setPenColor(color);
                }
            }
        });
        drawingControlsLayout->addWidget(colorButton);
        
        // Create pen width slider
        QSlider *widthSlider = new QSlider(Qt::Horizontal, this);
        widthSlider->setObjectName("penWidthSlider");
        widthSlider->setRange(1, 10);
        widthSlider->setValue(2);
        widthSlider->setFixedWidth(120);
        widthSlider->setStyleSheet(R"(
            QSlider::groove:horizontal {
                border: 1px solid #e0e0e0;
                height: 6px;
                background: white;
                margin: 2px 0;
                border-radius: 3px;
            }
            QSlider::handle:horizontal {
                background: white;
                border: 1px solid #bdbdbd;
                width: 16px;
                height: 16px;
                margin: -5px 0;
                border-radius: 8px;
                box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
            }
            QSlider::handle:horizontal:hover {
                background: #f8f9fa;
                border-color: #9e9e9e;
                box-shadow: 0 4px 8px rgba(0, 0, 0, 0.15);
            }
            QSlider::handle:horizontal:pressed {
                background: #f1f3f4;
                border-color: #757575;
                box-shadow: 0 1px 2px rgba(0, 0, 0, 0.1);
            }
        )");
        connect(widthSlider, &QSlider::valueChanged, this, [this](int value) {
            if (m_drawingLayer) {
                m_drawingLayer->setPenWidth(value);
            }
        });
        drawingControlsLayout->addWidget(widthSlider);
        
        // Add drawing controls to presentation layout
        if (ui->presentationLayout) {
            ui->presentationLayout->insertLayout(1, drawingControlsLayout);
        }
        
        // Pass it to presentation manager
        m_presentationManager->setPdfView(ui->pdfView);
        
        // Connect PDF view signals
        connect(m_presentationManager, &PresentationManager::error, this, [this](const QString &message) {
            QMessageBox::warning(this, tr("PDF Error"), message);
        });
        
        connect(m_presentationManager, &PresentationManager::presentationLoaded, this, [this](const QString &title, int pages) {
            statusBar()->showMessage(tr("Loaded presentation: %1 (%2 pages)").arg(title).arg(pages), 3000);
            
            // Switch to presentation view after successful load
            if (ui->stackedWidget) {
                ui->stackedWidget->setCurrentIndex(1);
            }
        });
    }

    // Register gesture commands
    m_gestureProcessor->registerGestureCommand("next_slide", [this]() {
        qDebug() << "Executing next slide command";
        m_presentationManager->nextSlide();
    });

    m_gestureProcessor->registerGestureCommand("prev_slide", [this]() {
        qDebug() << "Executing previous slide command";
        m_presentationManager->previousSlide();
    });

    m_gestureProcessor->registerGestureCommand("zoom_in", [this]() {
        qDebug() << "Executing zoom in command";
        m_presentationManager->zoomIn();
    });

    m_gestureProcessor->registerGestureCommand("zoom_out", [this]() {
        qDebug() << "Executing zoom out command";
        m_presentationManager->zoomOut();
    });

    qDebug() << "Application initialized successfully";
}

void MainWindow::setupConnections()
{
    qDebug() << "Setting up connections in MainWindow...";
    // WebSocket connections
    connect(m_webSocketClient, &WebSocketClient::connected, this, &MainWindow::onConnected);
    qDebug() << "WebSocket connected signal connected in MainWindow";
    connect(m_webSocketClient, &WebSocketClient::disconnected, this, &MainWindow::onDisconnected);
    qDebug() << "WebSocket disconnected signal connected in MainWindow";
    connect(m_webSocketClient, &WebSocketClient::messageReceived, this, &MainWindow::handleServerMessage);
    qDebug() << "WebSocket message received signal connected in MainWindow";

    // Connect gesture and page navigation signals
    connect(m_webSocketClient, &WebSocketClient::gestureReceived, this, [this](const QString &gestureType, const QString &clientId, const QString &timestamp) {
        qDebug() << "Gesture received in MainWindow:" << gestureType << "from client:" << clientId;
        
        QJsonObject gestureData;
        gestureData["gesture_type"] = gestureType;
        gestureData["client_id"] = clientId;
        gestureData["timestamp"] = timestamp;
        handleGestureCommand(gestureData);
    });

    qDebug() << "WebSocket gesture received signal connected in MainWindow";

    connect(m_webSocketClient, &WebSocketClient::pageNavigationReceived, this, [this](const QString &action, const QString &clientId, const QString &timestamp) {
        Q_UNUSED(timestamp)  // <-- Bunu ekleyin
        qDebug() << "Page navigation received in MainWindow:" << action << "from client:" << clientId;
        
        if (action == "next") {
            qDebug() << "Executing next slide command";
            m_presentationManager->nextSlide();
        } else if (action == "previous") {
            qDebug() << "Executing previous slide command";
            m_presentationManager->previousSlide();
        }
    });

    qDebug() << "WebSocket page navigation received signal connected in MainWindow";

    // Connect UI buttons to actions
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

    qDebug() << "UI button connections set up in MainWindow";

    // Connect new UI buttons
    if (ui->generateAttendanceCodeButton) {
        // wait 2 seconds before connecting to ensure UI is fully initialized
        QTimer::singleShot(2000, this, []() {
            qDebug() << "Connecting generateAttendanceCodeButton in MainWindow";
            // connect(ui->generateAttendanceCodeButton, &QPushButton::clicked, 
            //         this, &MainWindow::on_generateAttendanceCodeButton_clicked);
            qDebug() << "Generate Attendance Code button connected in MainWindow";
        });

        // qDebug() << "Connecting generateAttendanceCodeButton in MainWindow";
        // connect(ui->generateAttendanceCodeButton, &QPushButton::clicked, 
        //         this, &MainWindow::on_generateAttendanceCodeButton_clicked);
        // qDebug() << "Generate Attendance Code button connected in MainWindow";
    }

    qDebug() << "Generate Attendance Code button connected in MainWindow";
    
    if (ui->refreshAttendanceButton) {
        qDebug() << "Connecting refreshAttendanceButton in MainWindow";
        connect(ui->refreshAttendanceButton, &QPushButton::clicked,
                this, &MainWindow::on_refreshAttendanceButton_clicked);
        qDebug() << "Refresh Attendance button connected in MainWindow";
    }

    qDebug() << "Refresh Attendance button connected in MainWindow";
    
    if (ui->approveRequestButton) {
        connect(ui->approveRequestButton, &QPushButton::clicked,
                this, &MainWindow::on_approveRequestButton_clicked);
    }
    
    qDebug() << "Approve Request button connected in MainWindow";

    if (ui->rejectRequestButton) {
        connect(ui->rejectRequestButton, &QPushButton::clicked,
                this, &MainWindow::on_rejectRequestButton_clicked);
    }



    // Connect attendance manager signals
    qDebug() << "Connecting attendance manager signals in MainWindow";

    // Connect menu actions
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

    qDebug() << "Connections set up successfully in MainWindow";

    // Add Gesture Guide action
    QAction *actionGestureGuide = new QAction("Gesture Guide", this);
    ui->menuView->addAction(actionGestureGuide);
    connect(actionGestureGuide, &QAction::triggered, [this]() {
        QDialog *gestureDialog = new QDialog(this);
        gestureDialog->setWindowTitle("Gesture Guide");
        gestureDialog->setMinimumSize(500, 600);

        QVBoxLayout *layout = new QVBoxLayout(gestureDialog);

        // Create a tab widget
        QTabWidget *tabWidget = new QTabWidget(gestureDialog);

        // Required Gestures Tab
        QWidget *requiredTab = new QWidget(tabWidget);
        QVBoxLayout *requiredLayout = new QVBoxLayout(requiredTab);

        QLabel *requiredTitle = new QLabel("Required Gestures", requiredTab);
        requiredTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #2c3e50; margin-bottom: 10px;");
        requiredLayout->addWidget(requiredTitle);

        // Create a scroll area for required gestures
        QScrollArea *requiredScrollArea = new QScrollArea(requiredTab);
        QWidget *requiredContent = new QWidget(requiredScrollArea);
        QVBoxLayout *requiredContentLayout = new QVBoxLayout(requiredContent);

        // Add required gesture items
        struct GestureItem {
            QString emoji;
            QString name;
            QString description;
            QString command;
        };

        QList<GestureItem> requiredGestures = {
            {"⬅️", "Left", "Next slide", "next_slide"},
            {"➡️", "Right", "Previous slide", "prev_slide"},
            {"✌️", "Two Up", "Pointer mode (x,y coordinates)", "point"},
            {"☝️", "One", "Draw mode (x,y coordinates)", "draw"},
            {"👍", "Like", "Accept/Approve request", "accept"},
            {"👎", "Dislike", "Decline/Reject request", "decline"},
            {"📸", "Take Picture", "Take attendance", "attendance"},
            {"✋", "Palm", "Get attention", "attention"},
            {"⏰", "Timeout", "Start timer", "timeout"}
        };

        for (const auto &gesture : requiredGestures) {
            QWidget *gestureWidget = new QWidget(requiredContent);
            QHBoxLayout *gestureLayout = new QHBoxLayout(gestureWidget);

            QLabel *emojiLabel = new QLabel(gesture.emoji, gestureWidget);
            emojiLabel->setStyleSheet("font-size: 24px;");

            QLabel *nameLabel = new QLabel(gesture.name, gestureWidget);
            nameLabel->setStyleSheet("font-weight: bold; font-size: 14px;");

            QLabel *descLabel = new QLabel(gesture.description, gestureWidget);
            descLabel->setWordWrap(true);

            QLabel *commandLabel = new QLabel(gesture.command, gestureWidget);
            commandLabel->setStyleSheet("color: #666;");

            gestureLayout->addWidget(emojiLabel);
            gestureLayout->addWidget(nameLabel);
            gestureLayout->addWidget(descLabel);
            gestureLayout->addWidget(commandLabel);
            gestureLayout->setStretch(2, 1);

            requiredContentLayout->addWidget(gestureWidget);
        }

        requiredContent->setLayout(requiredContentLayout);
        requiredScrollArea->setWidget(requiredContent);
        requiredScrollArea->setWidgetResizable(true);
        requiredLayout->addWidget(requiredScrollArea);
        requiredTab->setLayout(requiredLayout);

        // Custom Gestures Tab
        QWidget *customTab = new QWidget(tabWidget);
        QVBoxLayout *customLayout = new QVBoxLayout(customTab);

        QLabel *customTitle = new QLabel("Custom Gestures", customTab);
        customTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #2c3e50; margin-bottom: 10px;");
        customLayout->addWidget(customTitle);

        // Create a scroll area for custom gestures
        QScrollArea *customScrollArea = new QScrollArea(customTab);
        QWidget *customContent = new QWidget(customScrollArea);
        QVBoxLayout *customContentLayout = new QVBoxLayout(customContent);

        // Add custom gesture items
        QList<GestureItem> customGestures = {
            {"📞", "Call", "Customizable action", ""},
            {"👌", "OK", "Customizable action", ""},
            {"❤️", "Heart", "Customizable action", ""},
            {"💕", "Heart-2", "Customizable action", ""},
            {"🖕", "Mid Finger", "Customizable action", ""},
            {"4️⃣", "Four", "Customizable action", ""},
            {"🤘", "Rock", "Customizable action", ""},
            {"🤌", "Thumb Index", "Customizable action", ""},
            {"🙏", "Holy", "Customizable action", ""},
            {"3️⃣", "Three", "Customizable action", ""},
            {"✌️", "Three-2", "Customizable action", ""},
            {"🤟", "Three-3", "Customizable action", ""}
        };

        for (const auto &gesture : customGestures) {
            QWidget *gestureWidget = new QWidget(customContent);
            QHBoxLayout *gestureLayout = new QHBoxLayout(gestureWidget);

            QLabel *emojiLabel = new QLabel(gesture.emoji, gestureWidget);
            emojiLabel->setStyleSheet("font-size: 24px;");

            QLabel *nameLabel = new QLabel(gesture.name, gestureWidget);
            nameLabel->setStyleSheet("font-weight: bold; font-size: 14px;");

            // Action ComboBox
            QComboBox *actionCombo = new QComboBox(gestureWidget);
            actionCombo->addItems(m_gestureProcessor->getAvailableActions());
            actionCombo->setCurrentText(m_gestureProcessor->getCustomGestureAction(gesture.name.toLower()));
            
            // Repeat Count SpinBox
            QSpinBox *repeatSpin = new QSpinBox(gestureWidget);
            repeatSpin->setRange(1, 10);
            repeatSpin->setValue(m_gestureProcessor->getCustomGestureRepeatCount(gesture.name.toLower()));
            repeatSpin->setPrefix("Repeat: ");

            // Save changes when combo box or spin box changes
            connect(actionCombo, &QComboBox::currentTextChanged, [=](const QString &action) {
                m_gestureProcessor->setCustomGestureAction(gesture.name.toLower(), action, repeatSpin->value());
            });

            connect(repeatSpin, QOverload<int>::of(&QSpinBox::valueChanged), [=](int value) {
                m_gestureProcessor->setCustomGestureAction(gesture.name.toLower(), actionCombo->currentText(), value);
            });

            gestureLayout->addWidget(emojiLabel);
            gestureLayout->addWidget(nameLabel);
            gestureLayout->addWidget(actionCombo);
            gestureLayout->addWidget(repeatSpin);
            gestureLayout->setStretch(2, 1);

            customContentLayout->addWidget(gestureWidget);
        }

        customContent->setLayout(customContentLayout);
        customScrollArea->setWidget(customContent);
        customScrollArea->setWidgetResizable(true);
        customLayout->addWidget(customScrollArea);
        customTab->setLayout(customLayout);

        // Add tabs to tab widget
        tabWidget->addTab(requiredTab, "Required Gestures");
        tabWidget->addTab(customTab, "Custom Gestures");

        layout->addWidget(tabWidget);

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

    qDebug() << "Gesture Guide action added to menu";

    // Add timeout functionality
    QAction *actionTimeout = new QAction("Timeout", this);
    actionTimeout->setShortcut(QKeySequence("Ctrl+T")); // Add keyboard shortcut
    ui->menuView->addAction(actionTimeout);

    connect(actionTimeout, &QAction::triggered, [this]() {
        showTimerDialog();
    });

    // Connect presentation manager signals
    connect(m_presentationManager, &PresentationManager::error, [this](const QString &errorMessage) {
        QMessageBox::warning(this, tr("PDF Error"), errorMessage);
    });

    connect(m_presentationManager, &PresentationManager::pageChanged, [this](int currentPage, int totalPages) {
        ui->pageIndicatorLabel->setText(tr("Page %1 of %2").arg(currentPage + 1).arg(totalPages));
    });

}


// Update onConnected method to use session code
void MainWindow::onConnected()
{
    qDebug() << "Connected to server";
    statusBar()->showMessage(tr("Connected to server"), 3000);

    ui->connectionStatusLabel->setText("Connected");
    ui->connectionStatusLabel->setStyleSheet("color: green;");
    ui->disconnectButton->setEnabled(true);
    ui->connectButton->setEnabled(false);

    // Send identification message with session code
    QJsonObject message;
    message["type"] = "identify";
    message["role"] = "desktop";
    message["roomId"] = m_currentRoomId; // Use session code as room ID
    message["sessionId"] = m_sessionId;
    message["sessionName"] = m_sessionName;

    QJsonDocument doc(message);
    m_webSocketClient->sendMessage(doc.toJson(QJsonDocument::Compact));

    m_restApi->startPollingRequests(m_currentRoomId);
}

void MainWindow::onDisconnected()
{
    qDebug() << "Disconnected from server1";
    //ui->connectionStatusLabel->setText("Disconnected");
    qDebug() << "Dc 1";
    //ui->connectionStatusLabel->setStyleSheet("color: red;");
    qDebug() << "Dc 2";
    //ui->disconnectButton->setEnabled(false);
    qDebug() << "Dc 3";
    //ui->connectButton->setEnabled(true);
    qDebug() << "Dc 4";
    m_restApi->stopPollingRequests();
    qDebug() << "Dc 5";
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

    qDebug() << "Processing message type:" << messageType;

    if (messageType == "page_navigation") {
        QString action = jsonObj["action"].toString();
        qDebug() << "Page navigation action:" << action;
        
        if (action == "next") {
            qDebug() << "Executing next slide command from WebSocket";
            m_presentationManager->nextSlide();
        } else if (action == "previous") {
            qDebug() << "Executing previous slide command from WebSocket";
            m_presentationManager->previousSlide();
        }
    } else if (messageType == "gesture") {
        qDebug() << "Handling gesture message";
        handleGestureCommand(jsonObj);
    } else if (messageType == "room_joined") {
        QString qrCodeData = jsonObj["qr_code"].toString();
        if (!qrCodeData.isEmpty()) {
            QByteArray decodedData = QByteArray::fromBase64(qrCodeData.toUtf8());
            QPixmap pixmap;
            pixmap.loadFromData(decodedData);

            if (ui->qrCodePreviewLabel) {
                ui->qrCodePreviewLabel->setPixmap(pixmap);
                ui->qrCodePreviewLabel->show();
            }
        }
    }
    else if (messageType == "attendance_update") {
        handleAttendanceUpdate(jsonObj["data"].toObject());
    } else if (messageType == "attendance_qr_code") {
        handleAttendanceQRCode(jsonObj["data"].toObject());
    } else if (messageType == "speak_request") {
        handleSpeakRequest(jsonObj["data"].toObject());
    } else if (messageType == "request_update") {
        handleRequestUpdate(jsonObj["data"].toObject());
    }

}

void MainWindow::onAttendanceListReceived(const QJsonArray &records)
{
    qDebug() << "onAttendanceListReceived called with" << records.size() << "records";
    
    // Clear existing attendance list
    if (ui->studentListWidget) {
        ui->studentListWidget->clear();
        qDebug() << "Cleared student list widget";
        
        // Add attendance records to the list
        for (const QJsonValue &value : records) {
            QJsonObject record = value.toObject();
            QString studentId = record["student_id"].toString();
            QString studentName = record["student_name"].toString();
            QString timestamp = record["timestamp"].toString();
            QString code = record["code"].toString();
            
            qDebug() << "Processing record:" << studentId << studentName << timestamp << code;
            
            // Format the display text with all information
            QString displayText = QString("%1 - %2\nKod: %3 | Zaman: %4")
                .arg(studentId)
                .arg(studentName)
                .arg(code)
                .arg(timestamp);
            
            QListWidgetItem *item = new QListWidgetItem(displayText);
            item->setData(Qt::UserRole, studentId);
            
            // Style the item
            item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            
            ui->studentListWidget->addItem(item);
            qDebug() << "Added item to list:" << displayText;
        }
        
        // Update attendance count
        if (ui->attendanceCountLabel) {
            //ui->attendanceCountLabel->setText(QString("Öğrenci sayısı: %1").arg(records.size()));
            qDebug() << "Updated attendance count label";
        }
    } else {
        qDebug() << "ERROR: studentListWidget is null!";
    }
}

void MainWindow::on_refreshAttendanceButton_clicked()
{
    qDebug() << "Refresh button clicked, current room ID:" << m_currentRoomId;
    
    if (!m_currentRoomId.isEmpty()) {
        // Show loading indicator
        if (ui->studentListWidget) {
            ui->studentListWidget->clear();
            QListWidgetItem *loadingItem = new QListWidgetItem("Yükleniyor...");
            loadingItem->setTextAlignment(Qt::AlignCenter);
            ui->studentListWidget->addItem(loadingItem);
            qDebug() << "Added loading indicator to student list widget";
        } else {
            qDebug() << "ERROR: studentListWidget is null in refresh handler!";
        }
        
        // Refresh attendance list
        m_restApi->listAttendance(m_currentRoomId);
        qDebug() << "Called listAttendance with room ID:" << m_currentRoomId;
    } else {
        QMessageBox::warning(this, "Hata", "Aktif bir sınıf bulunamadı.");
        qDebug() << "Warning shown: No active room found";
    }
}

void MainWindow::onAttendanceCodeGenerated(const QString &code, const QString &qrBase64, const QString &expiry)
{
    // Display the QR code
    QByteArray imageData = QByteArray::fromBase64(qrBase64.toUtf8());
    QPixmap pixmap;
    pixmap.loadFromData(imageData);
    
    if (ui->qrCodePreviewLabel) {
        ui->qrCodePreviewLabel->setPixmap(pixmap);
    }
    
    QMessageBox::information(this, "Attendance Code Generated",
        QString("Code: %1\nValid until: %2").arg(code, expiry));
}

void MainWindow::onRequestsReceived(const QJsonArray &requests)
{
    // Update the requests list widget
    if (ui->requestListWidget) {
        ui->requestListWidget->clear();
        
        for (const QJsonValue &value : requests) {
            QJsonObject request = value.toObject();
            QString studentName = request["student_name"].toString();
            QString requestId = request["id"].toString();
            
            QListWidgetItem *item = new QListWidgetItem(
                QString("%1 wants to speak").arg(studentName)
            );
            item->setData(Qt::UserRole, requestId);
            item->setData(Qt::UserRole + 1, studentName);
            
            ui->requestListWidget->addItem(item);
        }
    }
}

void MainWindow::onNewSpeakRequest(const QString &studentId, const QString &studentName, const QString &requestId)
{
    // Show notification
    showSpeakRequestNotification(studentName, requestId);
    
    // Play notification sound
    playNotificationSound();
    
    qDebug() << "New speak request from:" << studentName << "ID:" << studentId;
}

void MainWindow::onRequestUpdated(const QString &requestId, const QString &action)
{
    Q_UNUSED(action)  // <-- Bunu ekleyin

    // Hide notification if it's for the current request
    if (m_currentRequestId == requestId) {
        hideSpeakRequestNotification();
    }
    
    // Remove from request list widget
    if (ui->requestListWidget) {
        for (int i = 0; i < ui->requestListWidget->count(); ++i) {
            QListWidgetItem *item = ui->requestListWidget->item(i);
            if (item && item->data(Qt::UserRole).toString() == requestId) {
                delete ui->requestListWidget->takeItem(i);
                break;
            }
        }
    }
}

void MainWindow::on_generateAttendanceCodeButton_clicked()
{
    // if (!m_currentRoomId.isEmpty()) {
    //     // Generate attendance code via REST API
    //     m_restApi->generateAttendanceCode(m_currentRoomId, 5); // Use fixed duration of 5 minutes
        
    //     // Also get the QR code
    //     m_restApi->getQrCode(m_currentRoomId);
    // }
}

void MainWindow::on_downloadAttendanceButton_clicked()
{
    downloadAttendanceReport();
}

void MainWindow::on_approveRequestButton_clicked()
{
    if (!m_currentRequestId.isEmpty()) {
        m_restApi->updateRequest(m_currentRoomId, m_currentRequestId, "approve");
        hideSpeakRequestNotification();
    }
}

void MainWindow::on_rejectRequestButton_clicked()
{
    if (!m_currentRequestId.isEmpty()) {
        m_restApi->updateRequest(m_currentRoomId, m_currentRequestId, "reject");
        hideSpeakRequestNotification();
    }
}

void MainWindow::handleAttendanceUpdate(const QJsonObject &data)
{
    QString studentName = data["student_name"].toString();
    QString studentId = data["student_id"].toString();
    
    // Add to attendance list
    QListWidgetItem *item = new QListWidgetItem(studentName);
    item->setData(Qt::UserRole, studentId);
    
    if (ui->studentListWidget) {
        ui->studentListWidget->addItem(item);
    }
    
    // Update count
    int count = ui->studentListWidget ? ui->studentListWidget->count() : 0;
    if (ui->attendanceCountLabel) {
        ui->attendanceCountLabel->setText(QString("Students present: %1").arg(count));
    }
}

void MainWindow::handleAttendanceQRCode(const QJsonObject &data)
{
    QString qrBase64 = data["qr_code"].toString();
    QString code = data["code"].toString();
    QString expiry = data["expiry"].toString();
    
    // Display the QR code
    QByteArray imageData = QByteArray::fromBase64(qrBase64.toUtf8());
    QPixmap pixmap;
    pixmap.loadFromData(imageData);
    
    if (ui->qrCodePreviewLabel) {
        ui->qrCodePreviewLabel->setPixmap(pixmap);
    }
    
    // Switch to QR Code view
    if (ui->stackedWidget) {
        ui->stackedWidget->setCurrentIndex(3);
    }
}

void MainWindow::handleSpeakRequest(const QJsonObject &data)
{
    QString studentName = data["student_name"].toString();
    QString requestId = data["id"].toString();
    
    showSpeakRequestNotification(studentName, requestId);
    playNotificationSound();
}

void MainWindow::handleRequestUpdate(const QJsonObject &data)
{
    QString requestId = data["request_id"].toString();
    QString action = data["action"].toString();
    
    onRequestUpdated(requestId, action);
}

void MainWindow::playNotificationSound()
{
    // Qt 6 için güncellendi
    if (!m_notificationPlayer) {
        m_notificationPlayer = new QMediaPlayer(this);
        QAudioOutput *audioOutput = new QAudioOutput(this);
        m_notificationPlayer->setAudioOutput(audioOutput);
    }

    m_notificationPlayer->setSource(QUrl("qrc:/sounds/notification.wav"));

    if (m_notificationPlayer->audioOutput()) {
        m_notificationPlayer->audioOutput()->setVolume(0.7f);
    }

    m_notificationPlayer->play();
}

void MainWindow::showSpeakRequestNotification(const QString &studentName, const QString &requestId)
{
    m_currentRequestId = requestId;
    m_currentRequestStudentName = studentName;
    
    if (ui->notificationWidget) {
        ui->speakRequestLabel->setText(QString("🙋 %1 wants to speak").arg(studentName));
        ui->notificationWidget->setVisible(true);
        
        // Animate the notification
        ui->notificationWidget->setStyleSheet(
            "QWidget#notificationWidget {"
            "    background-color: #ffc107;"
            "    border-radius: 8px;"
            "    margin: 5px;"
            "    border: 2px solid #ff9800;"
            "}"
        );
    }
}

void MainWindow::hideSpeakRequestNotification()
{
    if (ui->notificationWidget) {
        ui->notificationWidget->setVisible(false);
    }
    
    m_currentRequestId.clear();
    m_currentRequestStudentName.clear();
}

void MainWindow::downloadAttendanceReport()
{
    if (m_currentRoomId.isEmpty()) {
        QMessageBox::warning(this, "Error", "No active room selected.");
        return;
    }

    // First, get the latest attendance data
    m_restApi->listAttendance(m_currentRoomId);

    // Connect a one-time slot to handle the response
    QMetaObject::Connection *const connection = new QMetaObject::Connection;
    *connection = connect(m_restApi, &RestApiClient::attendanceListReceived,
                          this, [this, connection](const QJsonArray &records) {
                              // Disconnect this one-time connection
                              QObject::disconnect(*connection);
                              delete connection;

                              // Get save file path
                              QString fileName = QFileDialog::getSaveFileName(this,
                                                                              "Save Attendance Report",
                                                                              QString("attendance_report_%1_%2.pdf")
                                                                                  .arg(m_userName)
                                                                                  .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss")),
                                                                              "PDF Files (*.pdf);;All Files (*)");

                              if (fileName.isEmpty()) {
                                  return;
                              }

                              // Create PDF document with better settings
                              QPdfWriter pdfWriter(fileName);
                              pdfWriter.setPageSize(QPageSize(QPageSize::A4));
                              pdfWriter.setPageMargins(QMarginsF(20, 20, 20, 20)); // Marjinleri azalttım
                              pdfWriter.setResolution(300); // Yüksek çözünürlük

                              QPainter painter(&pdfWriter);

                              // Antialiasing ve render hints ekle
                              painter.setRenderHint(QPainter::Antialiasing, true);
                              painter.setRenderHint(QPainter::TextAntialiasing, true);
                              painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

                              // Set up fonts with better sizing
                              QFont titleFont("Arial", 16, QFont::Bold);
                              QFont headerFont("Arial", 11, QFont::Bold);
                              QFont contentFont("Arial", 9, QFont::Normal);

                              // PDF boyutlarını al
                              int pageWidth = pdfWriter.width();
                              int pageHeight = pdfWriter.height();
                              int margin = 40; // 20mm margin in device units

                              // Calculate column widths based on actual page width - daha dengeli dağılım
                              int totalWidth = pageWidth - (2 * margin);
                              int idWidth = totalWidth * 0.12;        // Student ID için daha az yer
                              int nameWidth = totalWidth * 0.40;      // İsim için daha fazla yer
                              int timestampWidth = totalWidth * 0.33; // Timestamp için yeterli yer
                              int codeWidth = totalWidth * 0.15;      // Code için standart yer

                              // Draw title
                              painter.setFont(titleFont);
                              painter.setPen(Qt::black);
                              QRect titleRect(margin, margin, totalWidth, 60);
                              painter.drawText(titleRect, Qt::AlignHCenter | Qt::AlignVCenter, "Attendance Report");

                              // Draw session info
                              painter.setFont(contentFont);
                              QString sessionInfo = QString("Session: %1 (Room ID: %2)")
                                                        .arg(m_sessionName)
                                                        .arg(m_currentRoomId);
                              QRect sessionRect(margin, margin + 70, totalWidth, 40);
                              painter.drawText(sessionRect, Qt::AlignHCenter | Qt::AlignVCenter, sessionInfo);

                              // Draw date and time
                              QString dateTime = QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm:ss");
                              QRect dateRect(margin, margin + 110, totalWidth, 40);
                              painter.drawText(dateRect, Qt::AlignHCenter | Qt::AlignVCenter, dateTime);

                              // Table starting position
                              int tableStartY = margin + 170;
                              int headerHeight = 50;  // Header için daha fazla yükseklik
                              int rowHeight = 45;     // Satırlar için çok daha fazla yükseklik

                              // Draw table header
                              painter.setFont(headerFont);

                              // Header background
                              painter.fillRect(margin, tableStartY, totalWidth, headerHeight, QColor(240, 240, 240));

                              // Header borders
                              painter.setPen(QPen(Qt::black, 1));
                              painter.drawRect(margin, tableStartY, idWidth, headerHeight);
                              painter.drawRect(margin + idWidth, tableStartY, nameWidth, headerHeight);
                              painter.drawRect(margin + idWidth + nameWidth, tableStartY, timestampWidth, headerHeight);
                              painter.drawRect(margin + idWidth + nameWidth + timestampWidth, tableStartY, codeWidth, headerHeight);

                              // Header text with proper alignment - daha fazla padding
                              painter.setPen(Qt::black);
                              painter.drawText(QRect(margin + 10, tableStartY + 10, idWidth - 20, headerHeight - 20),
                                               Qt::AlignCenter | Qt::AlignVCenter, "Student ID");
                              painter.drawText(QRect(margin + idWidth + 10, tableStartY + 10, nameWidth - 20, headerHeight - 20),
                                               Qt::AlignCenter | Qt::AlignVCenter, "Student Name");
                              painter.drawText(QRect(margin + idWidth + nameWidth + 10, tableStartY + 10, timestampWidth - 20, headerHeight - 20),
                                               Qt::AlignCenter | Qt::AlignVCenter, "Timestamp");
                              painter.drawText(QRect(margin + idWidth + nameWidth + timestampWidth + 10, tableStartY + 10, codeWidth - 20, headerHeight - 20),
                                               Qt::AlignCenter | Qt::AlignVCenter, "Code");

                              // Draw content rows
                              painter.setFont(contentFont);
                              int currentY = tableStartY + headerHeight;

                              for (int i = 0; i < records.size(); ++i) {
                                  QJsonObject record = records[i].toObject();

                                  // Check if we need a new page
                                  if (currentY + rowHeight > pageHeight - margin) {
                                      pdfWriter.newPage();
                                      currentY = margin;

                                      // Redraw header on new page
                                      painter.setFont(headerFont);
                                      painter.fillRect(margin, currentY, totalWidth, headerHeight, QColor(240, 240, 240));
                                      painter.setPen(QPen(Qt::black, 1));
                                      painter.drawRect(margin, currentY, idWidth, headerHeight);
                                      painter.drawRect(margin + idWidth, currentY, nameWidth, headerHeight);
                                      painter.drawRect(margin + idWidth + nameWidth, currentY, timestampWidth, headerHeight);
                                      painter.drawRect(margin + idWidth + nameWidth + timestampWidth, currentY, codeWidth, headerHeight);

                                      painter.setPen(Qt::black);
                                      painter.drawText(QRect(margin + 10, currentY + 10, idWidth - 20, headerHeight - 20),
                                                       Qt::AlignCenter | Qt::AlignVCenter, "Student ID");
                                      painter.drawText(QRect(margin + idWidth + 10, currentY + 10, nameWidth - 20, headerHeight - 20),
                                                       Qt::AlignCenter | Qt::AlignVCenter, "Student Name");
                                      painter.drawText(QRect(margin + idWidth + nameWidth + 10, currentY + 10, timestampWidth - 20, headerHeight - 20),
                                                       Qt::AlignCenter | Qt::AlignVCenter, "Timestamp");
                                      painter.drawText(QRect(margin + idWidth + nameWidth + timestampWidth + 10, currentY + 10, codeWidth - 20, headerHeight - 20),
                                                       Qt::AlignCenter | Qt::AlignVCenter, "Code");

                                      currentY += headerHeight;
                                      painter.setFont(contentFont);
                                  }

                                  // Alternate row background
                                  if (i % 2 == 1) {
                                      painter.fillRect(margin, currentY, totalWidth, rowHeight, QColor(248, 249, 250));
                                  }

                                  // Draw cell borders
                                  painter.setPen(QPen(Qt::black, 1));
                                  painter.drawRect(margin, currentY, idWidth, rowHeight);
                                  painter.drawRect(margin + idWidth, currentY, nameWidth, rowHeight);
                                  painter.drawRect(margin + idWidth + nameWidth, currentY, timestampWidth, rowHeight);
                                  painter.drawRect(margin + idWidth + nameWidth + timestampWidth, currentY, codeWidth, rowHeight);

                                  // Draw cell content with padding and proper alignment - çok daha fazla padding
                                  painter.setPen(Qt::black);

                                  // Student ID - center aligned
                                  painter.drawText(QRect(margin + 10, currentY + 10, idWidth - 20, rowHeight - 20),
                                                   Qt::AlignCenter | Qt::AlignVCenter,
                                                   record["student_id"].toString());

                                  // Student Name - left aligned with more padding for readability
                                  QString studentName = record["student_name"].toString();
                                  painter.drawText(QRect(margin + idWidth + 15, currentY + 10, nameWidth - 30, rowHeight - 20),
                                                   Qt::AlignLeft | Qt::AlignVCenter,
                                                   studentName);

                                  // Timestamp - center aligned
                                  QString timestamp = record["timestamp"].toString();
                                  // Format timestamp if needed
                                  if (!timestamp.isEmpty()) {
                                      QDateTime dt = QDateTime::fromString(timestamp, Qt::ISODate);
                                      if (dt.isValid()) {
                                          timestamp = dt.toString("dd.MM.yyyy HH:mm");
                                      }
                                  }
                                  painter.drawText(QRect(margin + idWidth + nameWidth + 10, currentY + 10, timestampWidth - 20, rowHeight - 20),
                                                   Qt::AlignCenter | Qt::AlignVCenter, timestamp);

                                  // Code - center aligned
                                  painter.drawText(QRect(margin + idWidth + nameWidth + timestampWidth + 10, currentY + 10, codeWidth - 20, rowHeight - 20),
                                                   Qt::AlignCenter | Qt::AlignVCenter,
                                                   record["code"].toString());

                                  currentY += rowHeight;
                              }

                              // Draw summary at the bottom
                              currentY += 30;
                              painter.setFont(headerFont);
                              QString totalText = QString("Total Attendance: %1 students").arg(records.size());
                              painter.drawText(QRect(margin, currentY, totalWidth, 30),
                                               Qt::AlignRight | Qt::AlignVCenter, totalText);

                              // Draw footer with generation info
                              currentY += 50;
                              painter.setFont(contentFont);
                              QString footerText = QString("Report generated on %1 by %2")
                                                       .arg(QDateTime::currentDateTime().toString("dd.MM.yyyy HH:mm:ss"))
                                                       .arg(m_userName);
                              painter.drawText(QRect(margin, currentY, totalWidth, 30),
                                               Qt::AlignCenter | Qt::AlignVCenter, footerText);

                              painter.end();

                              QMessageBox::information(this, "Success",
                                                       QString("Attendance report saved successfully to:\n%1").arg(fileName));
                          });
}

// Update handleGestureCommand to handle approve/reject gestures
void MainWindow::handleGestureCommand(const QJsonObject &data)
{
    QString gestureType = data["gesture_type"].toString();
    QString clientId = data["client_id"].toString();
    
    qDebug() << "Handling gesture:" << gestureType << "from client:" << clientId;
    
    if (gestureType == "HAND_RAISE") {
        QMessageBox::information(this, "Hand Raise", 
            QString("Student %1 raised their hand").arg(clientId));
    }
    else if (gestureType == "THUMB_UP") {
        // Approve current speak request
        if (!m_currentRequestId.isEmpty()) {
            on_approveRequestButton_clicked();
            QMessageBox::information(this, "Request Approved", 
                QString("Approved speak request from %1").arg(m_currentRequestStudentName));
        }
    }
    else if (gestureType == "THUMB_DOWN") {
        // Reject current speak request
        if (!m_currentRequestId.isEmpty()) {
            on_rejectRequestButton_clicked();
            QMessageBox::information(this, "Request Rejected", 
                QString("Rejected speak request from %1").arg(m_currentRequestStudentName));
        }
    }
    else if (gestureType == "WAVE") {
        // Play attention sound
        playNotificationSound();
        QMessageBox::information(this, "Attention", 
            QString("Getting students' attention"));
    }
    else if (gestureType == "NEXT_SLIDE") {
        m_presentationManager->nextSlide();
    }
    else if (gestureType == "PREV_SLIDE") {
        m_presentationManager->previousSlide();
    }
    else if (gestureType == "ZOOM_IN") {
        m_presentationManager->zoomIn();
    }
    else if (gestureType == "ZOOM_OUT") {
        m_presentationManager->zoomOut();
    }
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
        if (!QFile::exists(sofficePath)) {
            QMessageBox::warning(this, tr("LibreOffice Not Found"), 
                tr("LibreOffice is required to convert PPTX files. Please install LibreOffice first."));
            return;
        }

        QProcess process;
        process.setWorkingDirectory(outputDir);
        
        QStringList arguments;
        arguments << "--headless"
                 << "--convert-to" << "pdf"
                 << "--outdir" << outputDir
                 << filePath;

        try {
            process.start("\"" + sofficePath + "\"", arguments);
            bool finished = process.waitForFinished(30000); // 30 second timeout

            if (!finished) {
                QMessageBox::warning(this, tr("Conversion Timeout"), 
                    tr("PPTX to PDF conversion timed out. Please try again with a smaller file."));
                process.kill();
                return;
            }

            if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
                QString error = QString::fromUtf8(process.readAllStandardError());
                QMessageBox::warning(this, tr("Conversion Failed"), 
                    tr("LibreOffice failed to convert PPTX to PDF.\nError: %1").arg(error));
                return;
            }

            // PDF çıktısını dosya adından tahmin et
            QDir dir(outputDir);
            QStringList pdfFiles = dir.entryList(QStringList() << (fileNameOnly + ".pdf"), QDir::Files, QDir::Time);
            if (!pdfFiles.isEmpty()) {
                QString newestPdf = dir.absoluteFilePath(pdfFiles.first());
                pathToLoad = newestPdf;
                qDebug() << "PDF bulundu:" << pathToLoad;
            } else {
                QMessageBox::warning(this, tr("Conversion Failed"), tr("Converted PDF file not found."));
                return;
            }
        } catch (const std::exception& e) {
            QMessageBox::warning(this, tr("Conversion Error"), 
                tr("Error during PPTX conversion: %1").arg(e.what()));
            return;
        }
    }

    // PDF'i yüklemeye çalış
    QApplication::setOverrideCursor(Qt::WaitCursor);
    bool loaded = false;
    
    try {
        // Ensure PDF view is properly initialized
        if (!ui->pdfView) {
            throw std::runtime_error("PDF view not initialized");
        }

        // Load the presentation
        loaded = m_presentationManager->loadPresentation(pathToLoad);
        
        if (loaded) {
            // Add to recent presentations
            addRecentPresentation(pathToLoad);
        }
    } catch (const std::exception& e) {
        QMessageBox::warning(this, tr("Loading Error"), 
            tr("Error loading presentation: %1").arg(e.what()));
    } catch (...) {
        QMessageBox::warning(this, tr("Loading Error"), 
            tr("Unknown error occurred while loading presentation."));
    }
    
    QApplication::restoreOverrideCursor();
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

    // QPrintDialog printDialog(&printer, this);
    // if (printDialog.exec() == QDialog::Accepted) {
    //     QPainter painter(&printer);
    //     QRect rect = painter.viewport();
    //     QSize size = pixmap.size();
    //     size.scale(rect.size(), Qt::KeepAspectRatio);
    //     painter.setViewport(rect.x(), rect.y(), size.width(), size.height());
    //     painter.setWindow(pixmap.rect());
    //     painter.drawPixmap(0, 0, pixmap);
    // }
}

void MainWindow::on_displayFullScreenButton_clicked()
{
    // Get QR code from REST API
    if (!m_currentRoomId.isEmpty()) {
        m_restApi->getQrCode(m_currentRoomId);
    } else {
        QMessageBox::warning(this, "Error", "No active room/session found.");
    }
}

void MainWindow::onQrCodeReceived(const QString &qrBase64)
{
    qDebug() << "QR Code received, length:" << qrBase64.length();
    
    // Convert base64 to QPixmap
    QByteArray imageData = QByteArray::fromBase64(qrBase64.toUtf8());
    QPixmap pixmap;
    if (!pixmap.loadFromData(imageData)) {
        QMessageBox::warning(this, "Error", "Failed to load QR code image");
        return;
    }

    // Show QR code in full screen dialog
    QDialog *fullScreenDialog = new QDialog(this, Qt::Window | Qt::WindowStaysOnTopHint);
    fullScreenDialog->setWindowState(Qt::WindowFullScreen);
    fullScreenDialog->setStyleSheet("background-color: white;");

    QVBoxLayout *layout = new QVBoxLayout(fullScreenDialog);
    
    // Add session info
    QLabel *sessionLabel = new QLabel(QString("Session: %1").arg(m_sessionName), fullScreenDialog);
    sessionLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #333;");
    sessionLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(sessionLabel);
    
    // Add room ID label
    QLabel *roomLabel = new QLabel(QString("Room ID: %1").arg(m_currentRoomId), fullScreenDialog);
    roomLabel->setStyleSheet("font-size: 20px; color: #666;");
    roomLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(roomLabel);

    // Add QR code label with larger size
    QLabel *label = new QLabel(fullScreenDialog);
    label->setPixmap(pixmap.scaled(500, 500, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);

    // Add instructions
    QLabel *instructionsLabel = new QLabel("Scan this QR code with the AirClass mobile app to join the session", fullScreenDialog);
    instructionsLabel->setStyleSheet("font-size: 16px; color: #666; margin: 20px;");
    instructionsLabel->setAlignment(Qt::AlignCenter);
    instructionsLabel->setWordWrap(true);
    layout->addWidget(instructionsLabel);

    // Add close button with better styling
    QPushButton *closeButton = new QPushButton("Close", fullScreenDialog);
    closeButton->setStyleSheet(R"(
        QPushButton {
            padding: 12px 24px;
            font-size: 16px;
            background-color: #4a90e2;
            color: white;
            border: none;
            border-radius: 6px;
            min-width: 120px;
            margin-top: 20px;
        }
        QPushButton:hover {
            background-color: #357abd;
        }
    )");
    connect(closeButton, &QPushButton::clicked, fullScreenDialog, &QDialog::close);
    layout->addWidget(closeButton, 0, Qt::AlignCenter);

    // Also update the preview in the main window
    if (ui->qrCodePreviewLabel) {
        ui->qrCodePreviewLabel->setPixmap(pixmap.scaled(200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

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

QPdfView* MainWindow::getPdfView() const
{
    // Find the PDF view widget in the UI
    QPdfView* pdfView = findChild<QPdfView*>();
    if (!pdfView) {
        qWarning() << "PDF view not found in MainWindow";
    }
    return pdfView;
}

// Add a method to update session info
void MainWindow::updateSessionInfo(const QString &sessionId, const QString &sessionName, const QString &sessionCode)
{
    m_sessionId = sessionId;
    m_sessionName = sessionName;
    m_sessionCode = sessionCode;
    
    // Update window title
    if (!m_sessionName.isEmpty()) {
        setWindowTitle(QString("AirClass Desktop - %1 (%2)").arg(m_sessionName, m_sessionCode));
    } else {
        setWindowTitle("AirClass Desktop");
    }
    
    // Update top bar with session info
    if (ui->topBar) {
        // Clear existing widgets in topBar
        QLayoutItem *child;
        while ((child = ui->horizontalLayout->takeAt(0)) != nullptr) {
            delete child->widget();
            delete child;
        }
        
        // Add session info with code
        QLabel *sessionLabel = new QLabel(QString("Session: %1 | Code: %2").arg(m_sessionName, m_sessionCode), ui->topBar);
        sessionLabel->setStyleSheet("color: #4a90e2; font-weight: bold; margin-right: 15px;");
        ui->horizontalLayout->addWidget(sessionLabel);
        
        // Add close session button right after session info
        QPushButton *closeSessionBtn = new QPushButton("Close Session", ui->topBar);
        closeSessionBtn->setObjectName("closeSessionButton");
        closeSessionBtn->setStyleSheet("QPushButton { color: #e74c3c; font-weight: bold; background: transparent; border: 1px solid #e74c3c; border-radius: 6px; padding: 6px 16px; } QPushButton:hover { background: #fbeee6; }");
        connect(closeSessionBtn, &QPushButton::clicked, this, &MainWindow::closeSession);
        ui->horizontalLayout->addWidget(closeSessionBtn);
        
        // Add user info
        QLabel *userLabel = new QLabel(QString("User: %1").arg(m_userName), ui->topBar);
        userLabel->setStyleSheet("color: #333; margin-right: 15px;");
        ui->horizontalLayout->addWidget(userLabel);
        
        // Add spacer
        ui->horizontalLayout->addStretch();
        
        // Re-add logout button
        QPushButton *logoutButton = new QPushButton("Log out", ui->topBar);
        logoutButton->setObjectName("logoutButton");
        logoutButton->setStyleSheet("QPushButton { color: #e74c3c; font-weight: bold; background: transparent; border: 1px solid #e74c3c; border-radius: 6px; padding: 6px 16px; } QPushButton:hover { background: #fbeee6; }");
        
        connect(logoutButton, &QPushButton::clicked, this, [this]() {
            // Hide all menus
            ui->menuFile->menuAction()->setVisible(false);
            ui->menuConnection->menuAction()->setVisible(false);
            ui->menuView->menuAction()->setVisible(false);

            // Clear auth token and session info
            m_authToken.clear();
            m_sessionId.clear();
            m_sessionName.clear();
            m_sessionCode.clear();
            m_currentRoomId.clear();
            m_restApi->setAuthToken("");

            // Show login dialog
            showLoginDialog();
        });
        
        ui->horizontalLayout->addWidget(logoutButton);
    }
    
    // Store session info in settings
    QSettings settings("AirClass", "Desktop");
    settings.setValue("session_id", sessionId);
    settings.setValue("session_name", sessionName);
    settings.setValue("session_code", sessionCode);
}

void MainWindow::closeSession()
{
    if (!m_sessionId.isEmpty()) {
        m_restApi->closeSession(m_sessionId);
    }
}

void MainWindow::onSessionClosed(const QString &sessionId)
{
    if (sessionId == m_sessionId) {
        m_sessionId.clear();
        m_sessionName.clear();
        m_sessionCode.clear();
        m_currentRoomId.clear();
        
        // Clear UI elements
        ui->requestListWidget->clear();
        
        // Hide all menus
        ui->menuFile->menuAction()->setVisible(false);
        ui->menuConnection->menuAction()->setVisible(false);
        ui->menuView->menuAction()->setVisible(false);
        
        // Show message to the user
        QMessageBox::information(this, "Session Closed", "The session has been closed successfully.");
        
        // Show login dialog
        showLoginDialog();
    }
}

MainWindow::~MainWindow()
{
    delete ui;
    
    // Clean up dynamically allocated objects
    if (m_webSocketClient) {
        delete m_webSocketClient;
        m_webSocketClient = nullptr;
    }
    
    if (m_presentationManager) {
        delete m_presentationManager;
        m_presentationManager = nullptr;
    }
    
    if (m_attendanceManager) {
        delete m_attendanceManager;
        m_attendanceManager = nullptr;
    }
    
    if (m_gestureProcessor) {
        delete m_gestureProcessor;
        m_gestureProcessor = nullptr;
    }
    
    if (m_uiController) {
        delete m_uiController;
        m_uiController = nullptr;
    }
    
    if (m_notificationPlayer) {
        delete m_notificationPlayer;
        m_notificationPlayer = nullptr;
    }
    
    // Note: Don't delete m_restApi as it's owned by QApplication
}

void MainWindow::setupDrawingControls()
{
    // Create drawing controls layout
    QHBoxLayout *drawingControlsLayout = new QHBoxLayout();
    drawingControlsLayout->setObjectName("drawingControlsLayout");
    
    // Common button style
    QString buttonStyle = R"(
        QPushButton {
            background-color: white;
            color: #333;
            border: 1px solid #e0e0e0;
            padding: 8px;
            border-radius: 8px;
            font-size: 18px;
            min-width: 44px;
            min-height: 44px;
            max-width: 44px;
            max-height: 44px;
            box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
        }
        QPushButton:hover {
            background-color: #f8f9fa;
            border-color: #bdbdbd;
            box-shadow: 0 4px 8px rgba(0, 0, 0, 0.15);
        }
        QPushButton:pressed {
            background-color: #f1f3f4;
            border-color: #9e9e9e;
            box-shadow: 0 1px 2px rgba(0, 0, 0, 0.1);
        }
    )";
    
    // Create clear button with emoji
    QPushButton *clearButton = new QPushButton("🗑️", this);
    clearButton->setObjectName("clearDrawingButton");
    clearButton->setToolTip("Clear Drawing");
    clearButton->setStyleSheet(buttonStyle);
    connect(clearButton, &QPushButton::clicked, this, [this]() {
        if (m_drawingLayer) {
            m_drawingLayer->clear();
        }
    });
    drawingControlsLayout->addWidget(clearButton);
    
    // Create drawing toggle button with emoji
    QPushButton *drawButton = new QPushButton("✏️", this);
    drawButton->setObjectName("drawButton");
    drawButton->setToolTip("Toggle Drawing");
    drawButton->setCheckable(true);
    drawButton->setChecked(true);
    drawButton->setStyleSheet(buttonStyle + R"(
        QPushButton:checked {
            background-color: #e8f5e9;
            border-color: #66bb6a;
            color: #2e7d32;
        }
        QPushButton:checked:hover {
            background-color: #c8e6c9;
            border-color: #43a047;
        }
        QPushButton:checked:pressed {
            background-color: #a5d6a7;
            border-color: #2e7d32;
        }
    )");
    connect(drawButton, &QPushButton::clicked, this, [this, drawButton]() {
        if (m_drawingLayer) {
            m_drawingLayer->setDrawingEnabled(drawButton->isChecked());
        }
    });
    drawingControlsLayout->addWidget(drawButton);
    
    // Create color button with emoji
    QPushButton *colorButton = new QPushButton("🎨", this);
    colorButton->setObjectName("colorButton");
    colorButton->setToolTip("Select Color");
    colorButton->setStyleSheet(buttonStyle);
    connect(colorButton, &QPushButton::clicked, this, [this]() {
        if (m_drawingLayer) {
            QColor color = QColorDialog::getColor(m_drawingLayer->getPenColor(), this, "Select Pen Color");
            if (color.isValid()) {
                m_drawingLayer->setPenColor(color);
            }
        }
    });
    drawingControlsLayout->addWidget(colorButton);
    
    // Create pen width slider
    QSlider *widthSlider = new QSlider(Qt::Horizontal, this);
    widthSlider->setObjectName("penWidthSlider");
    widthSlider->setRange(1, 10);
    widthSlider->setValue(2);
    widthSlider->setFixedWidth(120);
    widthSlider->setStyleSheet(R"(
        QSlider::groove:horizontal {
            border: 1px solid #e0e0e0;
            height: 6px;
            background: white;
            margin: 2px 0;
            border-radius: 3px;
        }
        QSlider::handle:horizontal {
            background: white;
            border: 1px solid #bdbdbd;
            width: 16px;
            height: 16px;
            margin: -5px 0;
            border-radius: 8px;
            box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
        }
        QSlider::handle:horizontal:hover {
            background: #f8f9fa;
            border-color: #9e9e9e;
            box-shadow: 0 4px 8px rgba(0, 0, 0, 0.15);
        }
        QSlider::handle:horizontal:pressed {
            background: #f1f3f4;
            border-color: #757575;
            box-shadow: 0 1px 2px rgba(0, 0, 0, 0.1);
        }
    )");
    connect(widthSlider, &QSlider::valueChanged, this, [this](int value) {
        if (m_drawingLayer) {
            m_drawingLayer->setPenWidth(value);
        }
    });
    drawingControlsLayout->addWidget(widthSlider);
    
    // Add drawing controls to presentation layout
    if (ui->presentationLayout) {
        ui->presentationLayout->insertLayout(1, drawingControlsLayout);
    }
}

void MainWindow::on_clearDrawingButton_clicked()
{
    if (m_drawingLayer) {
        m_drawingLayer->clear();
    }
}

void MainWindow::on_colorButton_clicked()
{
    if (m_drawingLayer) {
        QColor color = QColorDialog::getColor(m_drawingLayer->getPenColor(), this, "Select Pen Color");
        if (color.isValid()) {
            m_drawingLayer->setPenColor(color);
        }
    }
}

// Add this method to handle PDF view resize events
bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->pdfView && event->type() == QEvent::Resize) {
        if (m_drawingLayer) {
            m_drawingLayer->setGeometry(ui->pdfView->geometry());
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::on_timerStartButton_clicked()
{
    if (!m_isTimerRunning) {
        // Set initial time if timer is not already set
        if (m_remainingSeconds == 0) {
            m_remainingSeconds = (ui->timerMinutesSpinBox->value() * 60) + ui->timerSecondsSpinBox->value();
        }
        
        if (m_remainingSeconds > 0) {
            m_timer->start(1000); // Start timer with 1 second interval
        m_isTimerRunning = true;
        ui->timerStartButton->setEnabled(false);
        ui->timerStopButton->setEnabled(true);
            ui->timerMinutesSpinBox->setEnabled(false);
            ui->timerSecondsSpinBox->setEnabled(false);
        }
    }
}

void MainWindow::on_timerStopButton_clicked()
{
    if (m_isTimerRunning) {
        m_timer->stop();
        m_isTimerRunning = false;
        ui->timerStartButton->setEnabled(true);
        ui->timerStopButton->setEnabled(false);
        ui->timerMinutesSpinBox->setEnabled(true);
        ui->timerSecondsSpinBox->setEnabled(true);
    }
}

void MainWindow::on_timerResetButton_clicked()
{
    m_timer->stop();
    m_isTimerRunning = false;
    m_remainingSeconds = ui->timerMinutesSpinBox->value() * 60;
    updateTimerDisplay();
    ui->timerStartButton->setEnabled(true);
    ui->timerStopButton->setEnabled(false);
}

void MainWindow::on_timerMinutesSpinBox_valueChanged(int value)
{
    if (!m_isTimerRunning) {
        m_remainingSeconds = (value * 60) + ui->timerSecondsSpinBox->value();
        updateTimerDisplay();
    }
}

void MainWindow::on_timerSecondsSpinBox_valueChanged(int value)
{
    if (!m_isTimerRunning) {
        m_remainingSeconds = (ui->timerMinutesSpinBox->value() * 60) + value;
        updateTimerDisplay();
    }
}

void MainWindow::updateTimer()
{
    if (m_remainingSeconds > 0) {
        m_remainingSeconds--;
        updateTimerDisplay();
        
        if (m_remainingSeconds == 0) {
        m_timer->stop();
        m_isTimerRunning = false;
        ui->timerStartButton->setEnabled(true);
        ui->timerStopButton->setEnabled(false);
            ui->timerMinutesSpinBox->setEnabled(true);
            ui->timerSecondsSpinBox->setEnabled(true);
            QMessageBox::information(this, "Timer", "Süre doldu!");
        }
    }
}

void MainWindow::updateTimerDisplay()
{
    int minutes = m_remainingSeconds / 60;
    int seconds = m_remainingSeconds % 60;
    ui->timerDisplayLabel->setText(QString("%1:%2")
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0')));
}

// Add new method to handle timer dialog
void MainWindow::showTimerDialog()
{
    QDialog *timerDialog = new QDialog(this);
    timerDialog->setWindowTitle("Timer");
    timerDialog->setFixedSize(300, 250); // Increased height for reset button
    timerDialog->setStyleSheet("background-color: white;");

    QVBoxLayout *layout = new QVBoxLayout(timerDialog);

    // Time display
    QLabel *timeLabel = new QLabel("10:00", timerDialog);
    timeLabel->setStyleSheet("QLabel { font-size: 48px; font-weight: bold; color: #2c3e50; }");
    timeLabel->setAlignment(Qt::AlignCenter);

    // Control buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    QPushButton *decreaseButton = new QPushButton("-", timerDialog);
    QPushButton *increaseButton = new QPushButton("+", timerDialog);
    QPushButton *startButton = new QPushButton("Start", timerDialog);
    QPushButton *resetButton = new QPushButton("Reset", timerDialog);

    // Set button sizes
    decreaseButton->setFixedSize(50, 50);
    increaseButton->setFixedSize(50, 50);
    startButton->setFixedSize(100, 40);
    resetButton->setFixedSize(100, 40);

    // Style for control buttons
    QString buttonStyle = R"(
        QPushButton {
            font-size: 24px;
            font-weight: bold;
            border: 2px solid #3498db;
            border-radius: 25px;
            background-color: white;
            color: #3498db;
        }
        QPushButton:hover {
            background-color: #3498db;
            color: white;
        }
    )";

    // Style for action buttons (start and reset)
    QString actionButtonStyle = R"(
        QPushButton {
            font-size: 14px;
            font-weight: bold;
            background-color: #2ecc71;
            color: white;
            border: none;
            border-radius: 5px;
        }
        QPushButton:hover {
            background-color: #27ae60;
        }
    )";

    // Style for reset button
    QString resetButtonStyle = R"(
        QPushButton {
            font-size: 14px;
            font-weight: bold;
            background-color: #95a5a6;
            color: white;
            border: none;
            border-radius: 5px;
        }
        QPushButton:hover {
            background-color: #7f8c8d;
        }
    )";

    decreaseButton->setStyleSheet(buttonStyle);
    increaseButton->setStyleSheet(buttonStyle);
    startButton->setStyleSheet(actionButtonStyle);
    resetButton->setStyleSheet(resetButtonStyle);

    buttonLayout->addStretch();
    buttonLayout->addWidget(decreaseButton);
    buttonLayout->addWidget(increaseButton);
    buttonLayout->addStretch();

    layout->addWidget(timeLabel);
    layout->addLayout(buttonLayout);
    layout->addWidget(startButton, 0, Qt::AlignCenter);
    layout->addWidget(resetButton, 0, Qt::AlignCenter);

    // Timer setup - using pointers to maintain state
    QTimer *timer = new QTimer(timerDialog);
    int *remainingSeconds = new int(600); // Başlangıç değeri 10 dakika
    bool *isRunning = new bool(false);
    int *initialSeconds = new int(600); // Başlangıç değerini saklamak için

    // Update display function
    auto updateDisplay = [=]() {
        int minutes = *remainingSeconds / 60;
        int secs = *remainingSeconds % 60;
        timeLabel->setText(QString("%1:%2")
            .arg(minutes, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0')));
    };

    // Connect buttons
    connect(decreaseButton, &QPushButton::clicked, [=]() {
        if (!*isRunning && *remainingSeconds >= 60) {  // Minimum 1 dakika
            *remainingSeconds -= 60;
            *initialSeconds = *remainingSeconds; // Sıfırlama için başlangıç değerini güncelle
            updateDisplay();
        }
    });

    connect(increaseButton, &QPushButton::clicked, [=]() {
        if (!*isRunning && *remainingSeconds < 3600) {  // Maximum 60 dakika
            *remainingSeconds += 60;
            *initialSeconds = *remainingSeconds; // Sıfırlama için başlangıç değerini güncelle
            updateDisplay();
        }
    });

    connect(resetButton, &QPushButton::clicked, [=]() {
        timer->stop();
        *isRunning = false;
        *remainingSeconds = *initialSeconds; // Son ayarlanan değere sıfırla
        startButton->setText("Start");
        startButton->setStyleSheet(actionButtonStyle);
        decreaseButton->setEnabled(true);
        increaseButton->setEnabled(true);
        updateDisplay();
    });

    connect(startButton, &QPushButton::clicked, [=]() {
        if (*isRunning) {
            timer->stop();
            *isRunning = false;
            startButton->setText("Start");
            startButton->setStyleSheet(actionButtonStyle);
            decreaseButton->setEnabled(true);
            increaseButton->setEnabled(true);
            resetButton->setEnabled(true);
        } else {
            if (*remainingSeconds > 0) {
                timer->start(1000);
                *isRunning = true;
                startButton->setText("Stop");
                decreaseButton->setEnabled(false);
                increaseButton->setEnabled(false);
                resetButton->setEnabled(false);
                startButton->setStyleSheet(R"(
                    QPushButton {
                        font-size: 14px;
                        font-weight: bold;
                        background-color: #e74c3c;
                        color: white;
                        border: none;
                        border-radius: 5px;
                    }
                    QPushButton:hover {
                        background-color: #c0392b;
                    }
                )");
            }
        }
    });

    connect(timer, &QTimer::timeout, [=]() {
        if (*remainingSeconds > 0) {
            *remainingSeconds -= 1;
            updateDisplay();
            
            if (*remainingSeconds == 0) {
                timer->stop();
                *isRunning = false;
                startButton->setText("Start");
                startButton->setStyleSheet(actionButtonStyle);
                decreaseButton->setEnabled(true);
                increaseButton->setEnabled(true);
                resetButton->setEnabled(true);
                QMessageBox::information(timerDialog, "Timer", "Süre doldu!");
            }
        }
    });

    // Initial display
    updateDisplay();

    // Cleanup when dialog is closed
    connect(timerDialog, &QDialog::finished, [=]() {
        timer->stop();
        delete remainingSeconds;
        delete isRunning;
        delete initialSeconds;
    });

    timerDialog->setLayout(layout);
    timerDialog->exec();
}

void MainWindow::on_actionDashboard_triggered()
{
    if (ui->stackedWidget) {
        ui->stackedWidget->setCurrentIndex(0);
    }
}

void MainWindow::on_actionPresentation_triggered()
{
    if (ui->stackedWidget) {
        ui->stackedWidget->setCurrentIndex(1);
    }
}

void MainWindow::on_actionStudents_triggered()
{
    if (ui->stackedWidget) {
        ui->stackedWidget->setCurrentIndex(2);
    }
}

void MainWindow::on_actionGenerate_QR_Code_triggered()
{
    if (ui->stackedWidget) {
        ui->stackedWidget->setCurrentIndex(3);
    }
}

void MainWindow::on_connectButton_clicked()
{
    on_actionConnect_triggered();
}

void MainWindow::on_disconnectButton_clicked()
{
    on_actionDisconnect_triggered();
}

void MainWindow::on_openPresentationButton_clicked()
{
    on_actionOpen_triggered();
}

void MainWindow::on_approveButton_clicked()
{
    if (!m_currentRequestId.isEmpty()) {
        m_restApi->updateRequest(m_currentRoomId, m_currentRequestId, "approve");
        hideSpeakRequestNotification();
    }
}

void MainWindow::on_rejectButton_clicked()
{
    if (!m_currentRequestId.isEmpty()) {
        m_restApi->updateRequest(m_currentRoomId, m_currentRequestId, "reject");
        hideSpeakRequestNotification();
    }
}

void MainWindow::on_closeSessionButton_clicked()
{
    closeSession();
}
