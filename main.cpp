#include <QtGui>
#include <QSharedMemory>
#include "HomePrism.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // assert only one application
    /* QString UniqueID("HomePrism1");
    QSharedMemory sharedMemory;
    sharedMemory.setKey(UniqueID);

    if (!sharedMemory.create(1)) {
      QMessageBox::critical(0, QObject::tr("HomePrism"),
                            QObject::tr("Another HomePrism is already running. Will exit now."));
	    return -1; // Exit already a process running
    }

    /*
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        QMessageBox::critical(0, QObject::tr("HomePrism"),
                              QObject::tr("I couldn't detect any system tray "
                                          "on this system."));
        return -1;
    }
    */

    HomePrism HomePrism;
    HomePrism.show();
    //HomePrism.hide();
    return app.exec();
}
