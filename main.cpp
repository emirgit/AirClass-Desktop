#include "mainwindow.h"
#include <QApplication>
#include <QSplashScreen>
#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Set application information
    QCoreApplication::setOrganizationName("AirClass");
    QCoreApplication::setApplicationName("AirClass Desktop");
    QCoreApplication::setApplicationVersion("1.0.0");

    // Display splash screen
    QPixmap pixmap(":/images/splash.png");
    QSplashScreen splash(pixmap);
    splash.show();
    app.processEvents();

    // Create main window
    MainWindow mainWindow;

    // Give some time to show splash screen (2 seconds)
    QTimer::singleShot(2000, [&]() {
        mainWindow.show();
        splash.finish(&mainWindow);
    });

    return app.exec();
}
