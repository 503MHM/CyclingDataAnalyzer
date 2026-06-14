#include "ui/mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    //qputenv("QTWEBENGINE_CHROMIUM_FLAGS", "--disable-gpu --disable-software-rasterizer");
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    QObject::connect(&a, &QApplication::lastWindowClosed, [](){
        qDebug() << "lastWindowClosed emitted";
    });
    
    QObject::connect(&a, &QCoreApplication::aboutToQuit, [](){
        qDebug() << "aboutToQuit emitted";
    });
    return QCoreApplication::exec();
}
