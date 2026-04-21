#include <QApplication>

#include "MainWindow.h"
#include "backend/BackendTypes.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("NexusPilotQt"));
    app.setOrganizationName(QStringLiteral("OpenAI"));

    qRegisterMetaType<TelemetrySnapshot>("TelemetrySnapshot");

    MainWindow window;
    window.show();
    return app.exec();
}
