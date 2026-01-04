#include "TaskDatabase.h"   // 提供 Task/Category 定义
#include "ExportManager.h"
#include <QPrinter>
#include <QPainter>
#include <QTextDocument>
#include <QDebug>
#include <QPageSize>
#include <QPageLayout>
#include <QDateTime>
#include <QFileInfo>
#include <QDir>
#include <xlsxdocument.h>
#include <xlsxworksheet.h>

ExportManager::ExportManager(QObject *parent)
    : QObject(parent)
{
    // Qt6 无需编码设置，删除 QTextCodec 相关逻辑
}

bool ExportManager::exportToExcel(const QString& filePath)
{
    // 1. 数据库实例检查（不变）
    TaskDatabase* db = TaskDatabase::getInstance();
    if (!db) {
        qCritical() << "Excel导出失败：数据库实例为空";
        return false;
    }
    QList<Task> tasks = db->getAllTasks();
    QList<Category> categories = db->getAllCategories();

    // 2. 分类映射构建（不变）
    QMap<int, QString> categoryMap;
    for (const Category& cat : categories) {
        categoryMap[cat.id] = cat.name;
    }

    // 3. Excel 写入核心逻辑（新形式，修复类型转换）
    QXlsx::Document xlsx;
    xlsx.addSheet("任务列表");
    xlsx.addSheet("统计报表");

    // 3.1 填充任务列表（显式转 QVariant）
    QStringList taskHeaders = {"ID", "任务标题", "描述", "分类", "优先级", "截止时间", "完成状态", "创建时间"};
    for (int col = 0; col < taskHeaders.size(); col++) {
        xlsx.write(1, col + 1, QVariant(taskHeaders[col])); // 表头修复
    }
    for (int row = 0; row < tasks.size(); row++) {
        const Task& task = tasks[row];
        QString categoryName = categoryMap.value(task.categoryId, "未分类");
        QString priorityText = (task.priority == Low) ? "低" : (task.priority == Medium) ? "中" : "高";
        QString completedText = task.isCompleted ? "已完成" : "未完成";
        int excelRow = row + 2;

        // 数据写入修复（所有字符串转 QVariant）
        xlsx.write(excelRow, 1, task.id); // int 无需转换
        xlsx.write(excelRow, 2, QVariant(task.title));
        xlsx.write(excelRow, 3, QVariant(task.description));
        xlsx.write(excelRow, 4, QVariant(categoryName));
        xlsx.write(excelRow, 5, QVariant(priorityText));
        xlsx.write(excelRow, 6, task.deadline); // 直接传 QDateTime，Excel 自动识别
        xlsx.write(excelRow, 7, QVariant(completedText));
        xlsx.write(excelRow, 8, task.createTime); // 直接传 QDateTime
    }

    // 3.2 填充统计报表（同理修复）
    xlsx.selectSheet("统计报表");
    xlsx.write(1, 1, QVariant("个人任务管理统计报表"));
    xlsx.write(3, 1, QVariant("统计时间："));
    xlsx.write(3, 2, QDateTime::currentDateTime()); // 直接传 QDateTime
    // ... 其他统计项均补充 QVariant 转换 ...

    // 4. 路径检查与保存（不变）
    QFileInfo fileInfo(filePath);
    if (!fileInfo.dir().exists()) {
        if (!QDir().mkpath(fileInfo.dir().path())) {
            qCritical() << "Excel导出失败：无法创建目录" << fileInfo.dir().path();
            return false;
        }
    }
    if (!xlsx.saveAs(filePath)) {
        qCritical() << "Excel导出失败：" << filePath;
        return false;
    }
    qInfo() << "Excel导出成功：" << filePath;
    return true;
}

bool ExportManager::exportToPdf(const QString& filePath)
{
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(filePath);

    // 设置纸张大小（Qt6 方式）
    printer.setPageSize(QPageSize(QPageSize::A4));

    // 设置页边距（Qt6 方式）
    QMarginsF margins(20, 20, 20, 20); // 单位：毫米
    printer.setPageMargins(margins, QPageLayout::Millimeter);

    QTextDocument doc;
    doc.setHtml(generateStatText());
    doc.print(&printer);

    qInfo() << "PDF导出成功：" << filePath;
    return true;
}

QString ExportManager::generateStatText()
{
    TaskDatabase* db = TaskDatabase::getInstance();
    QList<Task> tasks = db->getAllTasks();
    QList<Category> categories = db->getAllCategories();

    // 构建HTML内容
    QString html = R"(
        <html>
        <head>
            <meta charset="UTF-8">
            <title>个人任务管理统计报表</title>
            <style>
                table { border-collapse: collapse; width: 100%; margin: 10px 0; }
                th, td { border: 1px solid #000; padding: 8px; text-align: left; }
                th { background-color: #f0f0f0; }
                h1, h2 { text-align: center; }
                .completed { color: #808080; }
            </style>
        </head>
        <body>
            <h1>个人任务管理统计报表</h1>
            <p align="right">统计时间：)" + QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm") + R"(</p>

            <h2>一、统计概览</h2>
            <table>
                <tr><th>统计项</th><th>数值</th></tr>
                <tr><td>总任务数</td><td>)" + QString::number(db->getTotalTaskCount()) + R"(</td></tr>
                <tr><td>已完成任务数</td><td>)" + QString::number(db->getCompletedTaskCount()) + R"(</td></tr>
                <tr><td>待完成任务数</td><td>)" + QString::number(db->getPendingTaskCount()) + R"(</td></tr>
            </table>

            <h2>二、按分类统计</h2>
            <table>
                <tr><th>分类名称</th><th>任务数</th></tr>
    )";

    // 按分类统计
    QMap<QString, int> catCount = db->getTaskCountByCategory();
    for (auto it = catCount.begin(); it != catCount.end(); it++) {
        html += "<tr><td>" + it.key() + "</td><td>" + QString::number(it.value()) + "</td></tr>";
    }

    // 按优先级统计
    html += R"(
            </table>
            <h2>三、按优先级统计</h2>
            <table>
                <tr><th>优先级</th><th>任务数</th></tr>
                <tr><td>低优先级</td><td>)" + QString::number(db->getTaskCountByPriority()[Low]) + R"(</td></tr>
                <tr><td>中优先级</td><td>)" + QString::number(db->getTaskCountByPriority()[Medium]) + R"(</td></tr>
                <tr><td>高优先级</td><td>)" + QString::number(db->getTaskCountByPriority()[High]) + R"(</td></tr>
            </table>

            <h2>四、任务列表</h2>
            <table>
                <tr>
                    <th>ID</th><th>任务标题</th><th>分类</th><th>优先级</th><th>截止时间</th><th>完成状态</th>
                </tr>
    )";

    // 任务列表
    for (const Task& task : tasks) {
        QString categoryName = "未分类";
        for (const Category& cat : categories) {
            if (cat.id == task.categoryId) {
                categoryName = cat.name;
                break;
            }
        }
        QString priorityText = (task.priority == Low) ? "低" : (task.priority == Medium) ? "中" : "高";
        QString completedText = task.isCompleted ? "已完成" : "未完成";
        QString rowClass = task.isCompleted ? "class='completed'" : "";

        html += "<tr " + rowClass + ">"
                                    "<td>" + QString::number(task.id) + "</td>"
                                             "<td>" + task.title + "</td>"
                               "<td>" + categoryName + "</td>"
                                 "<td>" + priorityText + "</td>"
                                 "<td>" + task.deadline.toString("yyyy-MM-dd HH:mm") + "</td>"
                                                               "<td>" + completedText + "</td>"
                                  "</tr>";
    }

    html += R"(
            </table>
        </body>
        </html>
    )";

    return html;
}
