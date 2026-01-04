#ifndef EXPORTMANAGER_H
#define EXPORTMANAGER_H

#include <QObject>
#include <QString>
#include "TaskDatabase.h"

// QXlsx 在 Qt6 中可能有不同的头文件包含方式
#include <xlsxdocument.h>
#include <xlsxworksheet.h>
#include <xlsxdocument.h>
#include <xlsxworksheet.h>

class ExportManager : public QObject
{
    Q_OBJECT
public:
    explicit ExportManager(QObject *parent = nullptr);

    // 导出Excel报表（任务列表 + 统计数据）
    bool exportToExcel(const QString& filePath);

    // 导出PDF报表（任务列表 + 统计数据）
    bool exportToPdf(const QString& filePath);

private:
    // 生成统计文本
    QString generateStatText();
};

#endif // EXPORTMANAGER_H
