#include "Widget.h"
#include "Manager.h"

#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    qunsetenv("QT_FORCE_STDERR_LOGGING");
    qunsetenv("QT_ASSUME_STDERR_HAS_CONSOLE");
    qunsetenv("QT_LOGGING_TO_CONSOLE");

    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(false);

    Manager manager;
    if (!manager.init()) {
        qWarning() << "Failed to init manager";
        return 1;
    }

//    Widget w;
//    w.show();
    return a.exec();
}
