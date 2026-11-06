#include "MainWindow.h"

#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("WorkBuddy"));
    QApplication::setApplicationName(QStringLiteral("BirdyQt"));
    QApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    birdy::MainWindow window;
    window.showMaximized();
    return app.exec();
}
