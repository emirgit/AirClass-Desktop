#include "mainwindow.h"
#include <QApplication>
#include <QSplashScreen>
#include <QTimer>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Set application information
    QCoreApplication::setOrganizationName("AirClass");
    QCoreApplication::setApplicationName("AirClass Desktop");
    QCoreApplication::setApplicationVersion("1.0.0");

    // Dark mode temasını ayarla
    QApplication::setStyle("Fusion");

    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(35, 35, 35));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);

    QApplication::setPalette(darkPalette);

    // Display splash screen
    QPixmap pixmap(":/images/splash.png");
    QSplashScreen splash(pixmap);
    splash.show();
    app.processEvents();

    // Create main window
    MainWindow mainWindow;

    // Tam ekran modunu devre dışı bırak
    mainWindow.setWindowFlags(Qt::Window);
    mainWindow.resize(1024, 768);

    // Give some time to show splash screen (2 seconds)
    QTimer::singleShot(2000, [&]() {
        mainWindow.showNormal(); // showFullScreen yerine showNormal() kullan
        splash.finish(&mainWindow);
    });

    return app.exec();
}
