#include <QtGui>
#include <QSharedMemory>
#include <QLocalSocket>
#include <QStyle>
#include <QDesktopWidget>
#include "HomePrism.h"
#include "HomePrismConfig.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // assure one application instance
    QString uniqueID( HomePrism_UUID );
    QLocalSocket socket;
    socket.connectToServer(uniqueID);
    if (socket.waitForConnected(500))
    {
        QMessageBox::critical(0, QObject::tr("HomePrism"),
                              QObject::tr("Another HomePrism is already running. Will exit now."));
        return EXIT_FAILURE; // Exit already a process running
    }

    // some app specific params
    QCoreApplication::setApplicationName( HomePrism_TITLE );
    QCoreApplication::setOrganizationName("DKFZ");

    // start
    HomePrism homePrismWindow;
    homePrismWindow.setGeometry(
        QStyle::alignedRect(
            Qt::LeftToRight,
            Qt::AlignCenter,
            homePrismWindow.size(),
            qApp->desktop()->availableGeometry()
        ));

    homePrismWindow.show();
    //HomePrism.hide();
    return app.exec();
}
