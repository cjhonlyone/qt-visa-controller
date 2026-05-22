#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QFont font("Times", 10, QFont::Normal, 0);
    a.setFont(font);

    // Qt6 默认 UTF-8，不再需要 QTextCodec。
    MainWindow w;
    w.show();
    return a.exec();
}
