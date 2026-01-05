#ifndef TASKDATABASE_H
#define TASKDATABASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QVariant>
#include <QDebug>
#include <QStandardPaths>

// 任务优先级枚举
enum TaskPriority {
    Low = 0,
    Medium,
    High
};

// 任务结构体
struct Task {
    int id;
    QString title;
    QString description;
    int categoryId;
    TaskPriority priority;
    QDateTime deadline;
    bool isCompleted;
    QDateTime createTime;
};

// 分类结构体
struct Category {
    int id;
    QString name;
};

class TaskDatabase : public QObject
{
    Q_OBJECT
public:
    static TaskDatabase* getInstance();
    ~TaskDatabase();
    QSqlDatabase getDatabaseConnection();

    // 初始化数据库（创建表）
    bool init();

    // 分类操作
    QList<Category> getAllCategories();
    bool addCategory(const QString& name);
    bool deleteCategory(int categoryId);

    // 任务操作
    QList<Task> getAllTasks();
    bool addTask(const Task& task);
    bool updateTask(const Task& task);
    bool deleteTask(int taskId);
    bool markTaskCompleted(int taskId, bool isCompleted);

    // 获取待提醒任务（截止时间前30分钟）
    QList<Task> getReminderTasks();

    // 统计数据
    int getTotalTaskCount();
    int getCompletedTaskCount();
    int getPendingTaskCount();
    QMap<QString, int> getTaskCountByCategory();
    QMap<TaskPriority, int> getTaskCountByPriority();

private:
    TaskDatabase();
    static TaskDatabase* m_instance;
    // 私有辅助方法
    QSqlDatabase createDatabaseConnection();
    bool executeQuery(QSqlQuery &query, const QString &queryString);
    QString getDatabasePath();
};

#endif // TASKDATABASE_H
