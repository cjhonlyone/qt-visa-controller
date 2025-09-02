#include "mainwindow.h"

#include <QApplication>
#include <QStringConverter>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QFont font("Times",10,QFont::Normal,0);

    a.setFont(font);
#if QT_VERSION < QT_VERSION_CHECK(5,0,0)
#if defined(_MSC_VER)
    QTextCodec *codec = QTextCodec::codecForName("gbk");
#else
    QTextCodec *codec = QTextCodec::codecForName("utf-8");
#endif
    QTextCodec::setCodecForLocale(codec);
    QTextCodec::setCodecForCStrings(codec);
    QTextCodec::setCodecForTr(codec);
#elif QT_VERSION < QT_VERSION_CHECK(6,0,0)
    // Qt 5 不支持 setCodecForCStrings/setCodecForTr，只能设置 locale
    QTextCodec *codec = QTextCodec::codecForName("utf-8");
    QTextCodec::setCodecForLocale(codec);
#else
    // Qt 6 不支持 QTextCodec，已移除相关 API，无需设置编码
    // 所有字符串默认 UTF-8
    // 你可以什么都不写
#endif
    MainWindow w;
    w.show();
    return a.exec();
}
