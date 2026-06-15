#include "ui/mainwindow.h"

#include <QApplication>
#include <QFile>

int main(int argc, char *argv[])
{
    // qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
    //     "--disable-gpu-compositing");

    QApplication a(argc, argv);

    QFile styleFile(":/style/style.qss");
    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
        a.setStyleSheet(QString::fromUtf8(styleFile.readAll()));
    }

    MainWindow w;
    w.show();
    return QCoreApplication::exec();
}
