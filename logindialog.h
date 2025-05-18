#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QShowEvent>

class LoginDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LoginDialog(QWidget *parent = nullptr);
    QString getEmail() const;
    QString getPassword() const;
    void showEvent(QShowEvent *event) override;

private slots:
    void switchToRegister();
    void switchToLogin();
    void handleLogin();
    void handleRegister();

private:
    QStackedWidget *stackedWidget{nullptr};
    
    // Login page widgets
    QWidget *loginPage{nullptr};
    QLineEdit *emailEdit{nullptr};
    QLineEdit *passwordEdit{nullptr};
    QPushButton *loginButton{nullptr};
    QPushButton *toRegisterButton{nullptr};
    
    // Register page widgets
    QWidget *registerPage{nullptr};
    QLineEdit *regEmailEdit{nullptr};
    QLineEdit *regPasswordEdit{nullptr};
    QLineEdit *confirmPasswordEdit{nullptr};
    QPushButton *registerButton{nullptr};
    QPushButton *toLoginButton{nullptr};

    void setupLoginPage();
    void setupRegisterPage();
    bool validateEmail(const QString &email) const;
};

#endif // LOGINDIALOG_H 