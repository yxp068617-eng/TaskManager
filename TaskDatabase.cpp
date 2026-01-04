// TaskDatabase.cpp
#include "TaskDatabase.h"
#include <QDebug>
#include <QStandardPaths>
#include <QDir>

TaskDatabase* TaskDatabase::m_instance = nullptr;

TaskDatabase::TaskDatabase()
{
    // 连接SQLite数据库
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/task_manager.db";
    QDir().mkpath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    m_db.setDatabaseName(dbPath);
}

TaskDatabase* TaskDatabase::getInstance()
{
    if (!m_instance) {
        m_instance = new TaskDatabase();
    }
    return m_instance;
}

TaskDatabase::~TaskDatabase()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
}

bool TaskDatabase::init()
{
    if (!m_db.open()) {
        qCritical() << "数据库打开失败：" << m_db.lastError().text();
        return false;
    }

    // 创建分类表
    QSqlQuery categoryQuery;
    QString createCategoryTable = R"(
        CREATE TABLE IF NOT EXISTS categories (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL UNIQUE
        )
    )";
    if (!categoryQuery.exec(createCategoryTable)) {
        qCritical() << "创建分类表失败：" << categoryQuery.lastError().text();
        return false;
    }

    // 创建任务表
    QSqlQuery taskQuery;
    QString createTaskTable = R"(
        CREATE TABLE IF NOT EXISTS tasks (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            title TEXT NOT NULL,
            description TEXT,
            category_id INTEGER,
            priority INTEGER NOT NULL DEFAULT 1, -- 0=低，1=中，2=高
            deadline DATETIME,
            is_completed BOOLEAN NOT NULL DEFAULT 0,
            create_time DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY (category_id) REFERENCES categories(id) ON DELETE SET NULL
        )
    )";
    if (!taskQuery.exec(createTaskTable)) {
        qCritical() << "创建任务表失败：" << taskQuery.lastError().text();
        return false;
    }

    // 插入默认分类
    categoryQuery.exec("SELECT COUNT(*) FROM categories");
    if (categoryQuery.next() && categoryQuery.value(0).toInt() == 0) {
        categoryQuery.exec("INSERT INTO categories (name) VALUES ('工作'), ('学习'), ('生活'), ('其他')");
    }

    return true;
}

// 分类操作实现
QList<Category> TaskDatabase::getAllCategories()
{
    QList<Category> categories;
    QSqlQuery query("SELECT id, name FROM categories ORDER BY id");
    while (query.next()) {
        Category cat;
        cat.id = query.value(0).toInt();
        cat.name = query.value(1).toString();
        categories.append(cat);
    }
    return categories;
}

bool TaskDatabase::addCategory(const QString& name)
{
    QSqlQuery query;
    query.prepare("INSERT INTO categories (name) VALUES (:name)");
    query.bindValue(":name", name);
    return query.exec();
}

bool TaskDatabase::deleteCategory(int categoryId)
{
    QSqlQuery query;
    query.prepare("DELETE FROM categories WHERE id = :id");
    query.bindValue(":id", categoryId);
    return query.exec();
}

// 任务操作实现
QList<Task> TaskDatabase::getAllTasks()
{
    QList<Task> tasks;
    QSqlQuery query("SELECT id, title, description, category_id, priority, deadline, is_completed, create_time FROM tasks ORDER BY deadline");
    while (query.next()) {
        Task task;
        task.id = query.value(0).toInt();
        task.title = query.value(1).toString();
        task.description = query.value(2).toString();
        task.categoryId = query.value(3).toInt();
        task.priority = static_cast<TaskPriority>(query.value(4).toInt());
        task.deadline = query.value(5).toDateTime();
        task.isCompleted = query.value(6).toBool();
        task.createTime = query.value(7).toDateTime();
        tasks.append(task);
    }
    return tasks;
}

bool TaskDatabase::addTask(const Task& task)
{
    QSqlQuery query;
    query.prepare(R"(
        INSERT INTO tasks (title, description, category_id, priority, deadline, is_completed, create_time)
        VALUES (:title, :desc, :cat_id, :priority, :deadline, :completed, :create_time)
    )");
    query.bindValue(":title", task.title);
    query.bindValue(":desc", task.description);
    query.bindValue(":cat_id", task.categoryId);
    query.bindValue(":priority", task.priority);
    query.bindValue(":deadline", task.deadline);
    query.bindValue(":completed", task.isCompleted);
    query.bindValue(":create_time", task.createTime);
    return query.exec();
}

bool TaskDatabase::updateTask(const Task& task)
{
    QSqlQuery query;
    query.prepare(R"(
        UPDATE tasks SET title = :title, description = :desc, category_id = :cat_id,
        priority = :priority, deadline = :deadline, is_completed = :completed
        WHERE id = :id
    )");
    query.bindValue(":title", task.title);
    query.bindValue(":desc", task.description);
    query.bindValue(":cat_id", task.categoryId);
    query.bindValue(":priority", task.priority);
    query.bindValue(":deadline", task.deadline);
    query.bindValue(":completed", task.isCompleted);
    query.bindValue(":id", task.id);
    return query.exec();
}

bool TaskDatabase::deleteTask(int taskId)
{
    QSqlQuery query;
    query.prepare("DELETE FROM tasks WHERE id = :id");
    query.bindValue(":id", taskId);
    return query.exec();
}

bool TaskDatabase::markTaskCompleted(int taskId, bool isCompleted)
{
    QSqlQuery query;
    query.prepare("UPDATE tasks SET is_completed = :completed WHERE id = :id");
    query.bindValue(":completed", isCompleted);
    query.bindValue(":id", taskId);
    return query.exec();
}

// 获取待提醒任务（截止时间前30分钟内，且未完成）
QList<Task> TaskDatabase::getReminderTasks()
{
    QList<Task> reminderTasks;
    QDateTime now = QDateTime::currentDateTime();
    QDateTime future = now.addSecs(30 * 60); // 30分钟后

    QSqlQuery query;
    query.prepare(R"(
        SELECT id, title, description, category_id, priority, deadline, is_completed, create_time
        FROM tasks
        WHERE is_completed = 0 AND deadline BETWEEN :now AND :future
    )");
    query.bindValue(":now", now);
    query.bindValue(":future", future);

    if (query.exec()) {
        while (query.next()) {
            Task task;
            task.id = query.value(0).toInt();
            task.title = query.value(1).toString();
            task.description = query.value(2).toString();
            task.categoryId = query.value(3).toInt();
            task.priority = static_cast<TaskPriority>(query.value(4).toInt());
            task.deadline = query.value(5).toDateTime();
            task.isCompleted = query.value(6).toBool();
            task.createTime = query.value(7).toDateTime();
            reminderTasks.append(task);
        }
    }
    return reminderTasks;
}

// 统计功能实现
int TaskDatabase::getTotalTaskCount()
{
    QSqlQuery query("SELECT COUNT(*) FROM tasks");
    return query.next() ? query.value(0).toInt() : 0;
}

int TaskDatabase::getCompletedTaskCount()
{
    QSqlQuery query("SELECT COUNT(*) FROM tasks WHERE is_completed = 1");
    return query.next() ? query.value(0).toInt() : 0;
}

int TaskDatabase::getPendingTaskCount()
{
    return getTotalTaskCount() - getCompletedTaskCount();
}

QMap<QString, int> TaskDatabase::getTaskCountByCategory()
{
    QMap<QString, int> countMap;
    QSqlQuery query(R"(
        SELECT c.name, COUNT(t.id)
        FROM categories c
        LEFT JOIN tasks t ON c.id = t.category_id
        GROUP BY c.id, c.name
    )");
    while (query.next()) {
        countMap[query.value(0).toString()] = query.value(1).toInt();
    }
    return countMap;
}

QMap<TaskPriority, int> TaskDatabase::getTaskCountByPriority()
{
    QMap<TaskPriority, int> countMap;
    countMap[Low] = 0;
    countMap[Medium] = 0;
    countMap[High] = 0;

    QSqlQuery query("SELECT priority, COUNT(*) FROM tasks GROUP BY priority");
    while (query.next()) {
        TaskPriority priority = static_cast<TaskPriority>(query.value(0).toInt());
        countMap[priority] = query.value(1).toInt();
    }
    return countMap;
}
