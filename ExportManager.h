#ifndef EXPORTMANAGER_H
#define EXPORTMANAGER_H

#include <QObject>
#include <QString>
#include "TaskDatabase.h"

// 删除重复的头文件包含
#include <xlsxdocument.h>
#include <xlsxworksheet.h>

class ExportManager : public QObject
{
    Q_OBJECT
public:
    explicit ExportManager(QObject *parent = nullptr);

    bool exportToExcel(const QString& filePath);
    bool exportToPdf(const QString& filePath);

private:
    QString generateStatText();
};

#endif // EXPORTMANAGER_H
