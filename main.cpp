#include "Widget.h"
#include "Manager.h"

#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
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
