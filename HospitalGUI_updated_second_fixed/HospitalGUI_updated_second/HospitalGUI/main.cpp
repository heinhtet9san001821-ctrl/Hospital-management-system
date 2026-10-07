#include "mainwindow.h"

#include <QApplication>
#include <QDir>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setApplicationName("Health++");
    QApplication::setOrganizationName("HealthPlusPlus");

#ifdef HMS_SOURCE_DIR
    QDir::setCurrent(QString::fromUtf8(HMS_SOURCE_DIR));
#endif

    MainWindow w;
    w.show();
    return QApplication::exec();
}
