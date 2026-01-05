#include "mainwindow.h"
#include "TaskDatabase.h"
#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QMessageBox>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 检查SQLite驱动是否可用
    QStringList drivers = QSqlDatabase::drivers();
    qDebug() << "可用的数据库驱动：" << drivers;

    if (!drivers.contains("QSQLITE")) {
        QMessageBox::critical(nullptr, "错误", "SQLite数据库驱动未找到！请检查Qt安装。");
        return -1;
    }

    // 初始化数据库
    TaskDatabase* db = TaskDatabase::getInstance();
    if (!db->init()) {
        QMessageBox::critical(nullptr, "错误", "数据库初始化失败！程序将退出。");
        return -1;
    }

    qDebug() << "数据库初始化成功";

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "TaskManager_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }

    MainWindow w;
    w.show();
    return a.exec();
}
