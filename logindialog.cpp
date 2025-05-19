#include "logindialog.h"
#include <QMessageBox>
#include <QRegularExpression>
#include <QStyle>
#include <QIcon>
#include <QFont>
#include <QApplication>
#include <QScreen>
#include <QTimer>

LoginDialog::LoginDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("AirClass - Welcome");
    // Set window flags to enable maximize button and window controls
    setWindowFlags(Qt::Window | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);
    
    // Set minimum size for the window
    setMinimumSize(400, 600);
    
    // Center the dialog on the screen
    QRect screenGeometry = QApplication::primaryScreen()->geometry();
    int x = (screenGeometry.width() - width()) / 2;
    int y = (screenGeometry.height() - height()) / 2;
    move(x, y);

    // Store window state
    m_isFullScreen = false;

    setStyleSheet(R"(
        QDialog {
            background-color: #ffffff;
        }
        QLineEdit {
            padding: 12px;
            border: 2px solid #e0e0e0;
            border-radius: 8px;
            background-color: #f8f9fa;
            font-size: 14px;
            margin-bottom: 12px;
            color: #333333;
            min-height: 20px;
        }
        QLineEdit:focus {
            border-color: #4a90e2;
            background-color: #ffffff;
        }
        QLineEdit::placeholder {
            color: #999999;
        }
        QPushButton {
            padding: 10px 20px;
            background-color: #4a90e2;
            color: white;
            border: none;
            border-radius: 8px;
            font-size: 14px;
            font-weight: bold;
            margin: 5px;
            min-height: 40px;
        }
        QPushButton:hover {
            background-color: #357abd;
        }
        QPushButton#secondary {
            background-color: transparent;
            color: #4a90e2;
            border: 2px solid #4a90e2;
        }
        QPushButton#secondary:hover {
            background-color: #f0f7ff;
        }
        QLabel {
            color: #333333;
            font-size: 14px;
        }
        QLabel#title {
            font-size: 32px;
            font-weight: bold;
            color: #2c3e50;
            margin-bottom: 20px;
        }
        QLabel#subtitle {
            font-size: 18px;
            color: #666666;
            margin-bottom: 15px;
        }
        QLabel#info {
            font-size: 14px;
            color: #666666;
            margin-top: 5px;
        }
    )");

    stackedWidget = new QStackedWidget(this);
    setupLoginPage();
    setupRegisterPage();

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    mainLayout->setSpacing(15);
    mainLayout->addWidget(stackedWidget);
    setLayout(mainLayout);
}

void LoginDialog::setupLoginPage()
{
    loginPage = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(loginPage);
    layout->setSpacing(20);
    layout->setAlignment(Qt::AlignCenter);

    // Container widget for centering content
    QWidget *container = new QWidget;
    QVBoxLayout *containerLayout = new QVBoxLayout(container);
    containerLayout->setSpacing(20);
    containerLayout->setAlignment(Qt::AlignCenter);
    container->setMaximumWidth(500);

    QLabel *titleLabel = new QLabel("Welcome to AirClass");
    titleLabel->setObjectName("title");
    titleLabel->setAlignment(Qt::AlignCenter);
    containerLayout->addWidget(titleLabel);

    QLabel *subtitleLabel = new QLabel("Please sign in to continue");
    subtitleLabel->setObjectName("subtitle");
    subtitleLabel->setAlignment(Qt::AlignCenter);
    containerLayout->addWidget(subtitleLabel);

    QLabel *emailLabel = new QLabel("School Email Address");
    emailLabel->setObjectName("info");
    containerLayout->addWidget(emailLabel);

    emailEdit = new QLineEdit;
    emailEdit->setPlaceholderText("Enter your school email (e.g., student@school.edu)");
    emailEdit->setText("student@school.edu");
    containerLayout->addWidget(emailEdit);

    QLabel *passwordLabel = new QLabel("Password");
    passwordLabel->setObjectName("info");
    containerLayout->addWidget(passwordLabel);

    passwordEdit = new QLineEdit;
    passwordEdit->setPlaceholderText("Enter your password");
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setText("123456");
    containerLayout->addWidget(passwordEdit);

    loginButton = new QPushButton("Sign In");
    loginButton->setMinimumHeight(45);
    containerLayout->addWidget(loginButton);

    QLabel *divider = new QLabel("or");
    divider->setAlignment(Qt::AlignCenter);
    divider->setStyleSheet("color: #666666; margin: 10px 0;");
    containerLayout->addWidget(divider);

    toRegisterButton = new QPushButton("Create New Account");
    toRegisterButton->setObjectName("secondary");
    toRegisterButton->setMinimumHeight(45);
    containerLayout->addWidget(toRegisterButton);

    layout->addWidget(container, 0, Qt::AlignCenter);
    layout->addStretch();

    connect(loginButton, &QPushButton::clicked, this, &LoginDialog::handleLogin);
    connect(toRegisterButton, &QPushButton::clicked, this, &LoginDialog::switchToRegister);

    stackedWidget->addWidget(loginPage);
}

void LoginDialog::setupRegisterPage()
{
    registerPage = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(registerPage);
    layout->setSpacing(20);
    layout->setAlignment(Qt::AlignCenter);

    // Container widget for centering content
    QWidget *container = new QWidget;
    QVBoxLayout *containerLayout = new QVBoxLayout(container);
    containerLayout->setSpacing(20);
    containerLayout->setAlignment(Qt::AlignCenter);
    container->setMaximumWidth(500);

    QLabel *titleLabel = new QLabel("Create Your Account");
    titleLabel->setObjectName("title");
    titleLabel->setAlignment(Qt::AlignCenter);
    containerLayout->addWidget(titleLabel);

    QLabel *subtitleLabel = new QLabel("Join AirClass to get started");
    subtitleLabel->setObjectName("subtitle");
    subtitleLabel->setAlignment(Qt::AlignCenter);
    containerLayout->addWidget(subtitleLabel);

    QLabel *emailLabel = new QLabel("School Email Address");
    emailLabel->setObjectName("info");
    containerLayout->addWidget(emailLabel);

    regEmailEdit = new QLineEdit;
    regEmailEdit->setPlaceholderText("Enter your school email (e.g., student@school.edu)");
    containerLayout->addWidget(regEmailEdit);

    QLabel *passwordLabel = new QLabel("Password");
    passwordLabel->setObjectName("info");
    containerLayout->addWidget(passwordLabel);

    regPasswordEdit = new QLineEdit;
    regPasswordEdit->setPlaceholderText("Create a password (minimum 6 characters)");
    regPasswordEdit->setEchoMode(QLineEdit::Password);
    containerLayout->addWidget(regPasswordEdit);

    QLabel *confirmLabel = new QLabel("Confirm Password");
    confirmLabel->setObjectName("info");
    containerLayout->addWidget(confirmLabel);

    confirmPasswordEdit = new QLineEdit;
    confirmPasswordEdit->setPlaceholderText("Re-enter your password");
    confirmPasswordEdit->setEchoMode(QLineEdit::Password);
    containerLayout->addWidget(confirmPasswordEdit);

    registerButton = new QPushButton("Create Account");
    registerButton->setMinimumHeight(45);
    containerLayout->addWidget(registerButton);

    QLabel *divider = new QLabel("or");
    divider->setAlignment(Qt::AlignCenter);
    divider->setStyleSheet("color: #666666; margin: 10px 0;");
    containerLayout->addWidget(divider);

    toLoginButton = new QPushButton("Back to Sign In");
    toLoginButton->setObjectName("secondary");
    toLoginButton->setMinimumHeight(45);
    containerLayout->addWidget(toLoginButton);

    layout->addWidget(container, 0, Qt::AlignCenter);
    layout->addStretch();

    connect(registerButton, &QPushButton::clicked, this, &LoginDialog::handleRegister);
    connect(toLoginButton, &QPushButton::clicked, this, &LoginDialog::switchToLogin);

    stackedWidget->addWidget(registerPage);
}

void LoginDialog::switchToRegister()
{
    stackedWidget->setCurrentWidget(registerPage);
}

void LoginDialog::switchToLogin()
{
    stackedWidget->setCurrentWidget(loginPage);
}

void LoginDialog::handleLogin()
{
    QString email = emailEdit->text().trimmed();
    QString password = passwordEdit->text();

    if (email.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please fill in all fields");
        return;
    }

    if (!validateEmail(email)) {
        QMessageBox::warning(this, "Error", "Please enter a valid school email address");
        return;
    }

    accept();
}

void LoginDialog::handleRegister()
{
    QString email = regEmailEdit->text().trimmed();
    QString password = regPasswordEdit->text();
    QString confirmPassword = confirmPasswordEdit->text();

    if (email.isEmpty() || password.isEmpty() || confirmPassword.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please fill in all fields");
        return;
    }

    if (!validateEmail(email)) {
        QMessageBox::warning(this, "Error", "Please enter a valid school email address");
        return;
    }

    if (password != confirmPassword) {
        QMessageBox::warning(this, "Error", "Passwords do not match");
        return;
    }

    if (password.length() < 6) {
        QMessageBox::warning(this, "Error", "Password must be at least 6 characters long");
        return;
    }

    accept();
}

bool LoginDialog::validateEmail(const QString &email) const
{
    QRegularExpression regex("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$");
    return regex.match(email).hasMatch();
}

QString LoginDialog::getEmail() const
{
    return stackedWidget->currentWidget() == loginPage ? emailEdit->text() : regEmailEdit->text();
}

QString LoginDialog::getPassword() const
{
    return stackedWidget->currentWidget() == loginPage ? passwordEdit->text() : regPasswordEdit->text();
}

void LoginDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
}

bool LoginDialog::isFullScreen() const
{
    return m_isFullScreen;
}

void LoginDialog::setFullScreen(bool fullScreen)
{
    m_isFullScreen = fullScreen;
    if (fullScreen) {
        showFullScreen();
    } else {
        showNormal();
    }
} 