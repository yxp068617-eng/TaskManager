#include "TaskDatabase.h"
#include <QDir>
#include <QSqlDatabase>
#include <QSqlError>
#include <QDebug>
#include <QThread>

TaskDatabase* TaskDatabase::m_instance = nullptr;

TaskDatabase::TaskDatabase() : QObject()
{
}

TaskDatabase::~TaskDatabase()
{
    // 清理所有数据库连接3
    QStringList connections = QSqlDatabase::connectionNames();
    for (const QString& connection : connections) {
        if (connection.startsWith("TaskDBConnection_")) {
            QSqlDatabase::removeDatabase(connection);
        }
    }
}

TaskDatabase* TaskDatabase::getInstance()
{
    if (!m_instance) {
        m_instance = new TaskDatabase();
    }
    return m_instance;
}

QString TaskDatabase::getDatabasePath()
{
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    // 确保目录存在且可写
    QDir dir;
    if (!dir.mkpath(appDataPath)) {
        qCritical() << "无法创建数据库目录：" << appDataPath;
    }

    QString dbPath = appDataPath + "/task_manager.db";
    qDebug() << "数据库路径：" << dbPath;

    return dbPath;
}

QSqlDatabase TaskDatabase::createDatabaseConnection()
{
    QString connectionName = QString("TaskDBConnection_%1").arg((quintptr)QThread::currentThreadId());

    // 如果连接已存在，检查是否有效
    if (QSqlDatabase::contains(connectionName)) {
        QSqlDatabase existingDb = QSqlDatabase::database(connectionName);
        if (existingDb.isOpen()) {
            return existingDb;
        } else {
            // 如果连接存在但未打开，移除后重建
            qWarning() << "移除无效的数据库连接：" << connectionName;
            QSqlDatabase::removeDatabase(connectionName);
        }
    }

    // 创建新连接
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(getDatabasePath());

    if (!db.open()) {
        qCritical() << "无法打开数据库连接：" << db.lastError().text();
        qCritical() << "数据库错误类型：" << db.lastError().type();
        qCritical() << "数据库路径：" << getDatabasePath();
        // 不要返回无效的连接
        QSqlDatabase::removeDatabase(connectionName);
        return QSqlDatabase(); // 返回空连接
    }

    qDebug() << "数据库连接成功：" << connectionName;
    return db;
}

bool TaskDatabase::executeQuery(QSqlQuery &query, const QString &queryString)
{
    if (!query.prepare(queryString)) {
        qCritical() << "查询准备失败：" << query.lastError().text();
        qCritical() << "查询语句：" << queryString;
        return false;
    }

    if (!query.exec()) {
        qCritical() << "查询执行失败：" << query.lastError().text();
        qCritical() << "SQL错误类型：" << query.lastError().type();
        qCritical() << "SQL错误号：" << query.lastError().nativeErrorCode();
        return false;
    }

    return true;
}

bool TaskDatabase::init()
{
    QSqlDatabase db = createDatabaseConnection();
    if (!db.isOpen()) {
        qCritical() << "数据库初始化失败：无法打开数据库连接";
        return false;
    }

    // 创建分类表
    QString createCategoryTable = R"(
        CREATE TABLE IF NOT EXISTS categories (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL UNIQUE
        )
    )";

    QSqlQuery categoryQuery(db);
    if (!executeQuery(categoryQuery, createCategoryTable)) {
        qCritical() << "创建分类表失败";
        db.close();
        return false;
    }

    // 创建任务表
    QString createTaskTable = R"(
    CREATE TABLE IF NOT EXISTS tasks (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        title TEXT NOT NULL,
        description TEXT,
        category_id INTEGER,
        priority INTEGER NOT NULL DEFAULT 0,
        deadline DATETIME,
        completed BOOLEAN NOT NULL DEFAULT 0,
        create_time DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
        FOREIGN KEY (category_id) REFERENCES categories(id) ON DELETE SET NULL
    )
)";

    QSqlQuery taskQuery(db);
    if (!executeQuery(taskQuery, createTaskTable)) {
        qCritical() << "创建任务表失败";
        db.close();
        return false;
    }

    // 插入默认分类
    QSqlQuery checkQuery("SELECT COUNT(*) FROM categories", db);
    if (checkQuery.next() && checkQuery.value(0).toInt() == 0) {
        qDebug() << "插入默认分类数据";
        QSqlQuery insertQuery(db);
        bool success = insertQuery.exec("INSERT INTO categories (name) VALUES ('工作'), ('学习'), ('生活'), ('其他')");
        if (!success) {
            qWarning() << "插入默认分类失败：" << insertQuery.lastError().text();
        }
    }

    db.close();
    return true;
}

// 分类操作实现
QList<Category> TaskDatabase::getAllCategories()
{
    QList<Category> categories;
    QSqlDatabase db = createDatabaseConnection();
    if (!db.isOpen()) {
        qWarning() << "获取分类失败：数据库未打开";
        return categories;
    }

    QSqlQuery query("SELECT id, name FROM categories ORDER BY id", db);
    if (!query.exec()) {
        qCritical() << "查询分类失败：" << query.lastError().text();
        db.close();
        return categories;
    }

    while (query.next()) {
        Category cat;
        cat.id = query.value(0).toInt();
        cat.name = query.value(1).toString();
        categories.append(cat);
    }

    db.close();
    return categories;
}

bool TaskDatabase::addCategory(const QString& name)
{
    QSqlDatabase db = createDatabaseConnection();
    if (!db.isOpen()) {
        qWarning() << "添加分类失败：数据库未打开";
        return false;
    }

    QSqlQuery query(db);
    query.prepare("INSERT INTO categories (name) VALUES (:name)");
    query.bindValue(":name", name);

    bool success = query.exec();
    if (!success) {
        qCritical() << "添加分类失败：" << query.lastError().text();
    }

    db.close();
    return success;
}

bool TaskDatabase::deleteCategory(int categoryId)
{
    QSqlDatabase db = createDatabaseConnection();
    if (!db.isOpen()) {
        qWarning() << "删除分类失败：数据库未打开";
        return false;
    }

    QSqlQuery query(db);
    query.prepare("DELETE FROM categories WHERE id = :id");
    query.bindValue(":id", categoryId);

    bool success = query.exec();
    if (!success) {
        qCritical() << "删除分类失败：" << query.lastError().text();
    }

    db.close();
    return success;
}

// 任务操作实现
QList<Task> TaskDatabase::getAllTasks()
{
    QList<Task> tasks;
    QSqlDatabase db = createDatabaseConnection();
    if (!db.isOpen()) {
        qWarning() << "获取任务失败：数据库未打开";
        return tasks;
    }

    QSqlQuery query("SELECT id, title, description, category_id, priority, deadline, is_completed, create_time FROM tasks ORDER BY deadline", db);
    if (!query.exec()) {
        qCritical() << "查询任务失败：" << query.lastError().text();
        db.close();
        return tasks;
    }

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

    db.close();
    return tasks;
}

// TaskDatabase.cpp
bool TaskDatabase::addTask(const Task& task)
{
    // 1. 打开数据库（直接使用默认连接或指定路径）
    QSqlDatabase db = QSqlDatabase::database(); // 使用默认连接
    if (!db.isValid()) {
        // 如果默认连接不存在，就创建一个
        db = QSqlDatabase::addDatabase("QSQLITE");
        QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/task_manager.db";
        db.setDatabaseName(dbPath);
    }

    if (!db.isOpen()) {
        if (!db.open()) {
            qCritical() << "无法打开数据库：" << db.lastError().text();
            return false;
        }
    }

    // 2. 设置 createTime 为当前时间
    Task newTask = task;
    newTask.createTime = QDateTime::currentDateTime();

    // 3. 准备 SQL
    QSqlQuery query(db);
    QString insertSQL = R"(
        INSERT INTO tasks (title, description, category_id, priority, deadline, completed, create_time)
        VALUES (:title, :desc, :cat_id, :priority, :deadline, :completed, :create_time)
    )";

    if (!query.prepare(insertSQL)) {
        qCritical() << "SQL准备失败：" << query.lastError().text();
        db.close();
        return false;
    }

    // 4. 绑定参数
    query.bindValue(":title",       newTask.title);
    query.bindValue(":desc",        newTask.description);
    query.bindValue(":cat_id",      newTask.categoryId);
    query.bindValue(":priority",    static_cast<int>(newTask.priority));
    query.bindValue(":deadline",    newTask.deadline);
    query.bindValue(":completed",   newTask.isCompleted);
    query.bindValue(":create_time", newTask.createTime);

    // 5. 执行
    bool success = query.exec();
    if (!success) {
        qCritical() << "SQL执行失败：" << query.lastError().text();
        qCritical() << "SQL错误类型：" << query.lastError().type();
        qCritical() << "SQL错误号：" << query.lastError().nativeErrorCode();
    }

    // 6. 关闭数据库（可选，但建议在函数结束时关闭，避免锁文件）
    db.close();

    return success;
}

bool TaskDatabase::updateTask(const Task& task)
{
    QSqlDatabase db = createDatabaseConnection();
    if (!db.isOpen()) {
        qWarning() << "更新任务失败：数据库未打开";
        return false;
    }

    QSqlQuery query(db);
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

    bool success = query.exec();
    if (!success) {
        qCritical() << "更新任务失败：" << query.lastError().text();
    }

    db.close();
    return success;
}

bool TaskDatabase::deleteTask(int taskId)
{
    QSqlDatabase db = createDatabaseConnection();
    if (!db.isOpen()) {
        qWarning() << "删除任务失败：数据库未打开";
        return false;
    }

    QSqlQuery query(db);
    query.prepare("DELETE FROM tasks WHERE id = :id");
    query.bindValue(":id", taskId);

    bool success = query.exec();
    if (!success) {
        qCritical() << "删除任务失败：" << query.lastError().text();
    }

    db.close();
    return success;
}

bool TaskDatabase::markTaskCompleted(int taskId, bool isCompleted)
{
    QSqlDatabase db = createDatabaseConnection();
    if (!db.isOpen()) {
        qWarning() << "标记任务失败：数据库未打开";
        return false;
    }

    QSqlQuery query(db);
    query.prepare("UPDATE tasks SET is_completed = :completed WHERE id = :id");
    query.bindValue(":completed", isCompleted);
    query.bindValue(":id", taskId);

    bool success = query.exec();
    if (!success) {
        qCritical() << "标记任务失败：" << query.lastError().text();
    }

    db.close();
    return success;
}

// 获取待提醒任务
QList<Task> TaskDatabase::getReminderTasks()
{
    QList<Task> reminderTasks;
    QSqlDatabase db = createDatabaseConnection();
    if (!db.isOpen()) {
        qWarning() << "获取提醒任务失败：数据库未打开";
        return reminderTasks;
    }

    QDateTime now = QDateTime::currentDateTime();
    QDateTime future = now.addSecs(30 * 60); // 30分钟后

    QSqlQuery query(db);
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
    } else {
        qCritical() << "查询提醒任务失败：" << query.lastError().text();
    }

    db.close();
    return reminderTasks;
}

// 统计功能实现
int TaskDatabase::getTotalTaskCount()
{
    QSqlDatabase db = createDatabaseConnection();
    if (!db.isOpen()) {
        qWarning() << "统计任务失败：数据库未打开";
        return 0;
    }

    QSqlQuery query("SELECT COUNT(*) FROM tasks", db);
    int count = query.next() ? query.value(0).toInt() : 0;

    db.close();
    return count;
}

int TaskDatabase::getCompletedTaskCount()
{
    QSqlDatabase db = createDatabaseConnection();
    if (!db.isOpen()) {
        qWarning() << "统计已完成任务失败：数据库未打开";
        return 0;
    }

    QSqlQuery query("SELECT COUNT(*) FROM tasks WHERE is_completed = 1", db);
    int count = query.next() ? query.value(0).toInt() : 0;

    db.close();
    return count;
}

int TaskDatabase::getPendingTaskCount()
{
    return getTotalTaskCount() - getCompletedTaskCount();
}

QMap<QString, int> TaskDatabase::getTaskCountByCategory()
{
    QMap<QString, int> countMap;
    QSqlDatabase db = createDatabaseConnection();
    if (!db.isOpen()) {
        qWarning() << "按分类统计失败：数据库未打开";
        return countMap;
    }

    QSqlQuery query(R"(
        SELECT c.name, COUNT(t.id)
        FROM categories c
        LEFT JOIN tasks t ON c.id = t.category_id
        GROUP BY c.id, c.name
    )", db);

    if (query.exec()) {
        while (query.next()) {
            countMap[query.value(0).toString()] = query.value(1).toInt();
        }
    } else {
        qCritical() << "按分类统计失败：" << query.lastError().text();
    }

    db.close();
    return countMap;
}

QMap<TaskPriority, int> TaskDatabase::getTaskCountByPriority()
{
    QMap<TaskPriority, int> countMap;
    countMap[Low] = 0;
    countMap[Medium] = 0;
    countMap[High] = 0;

    QSqlDatabase db = createDatabaseConnection();
    if (!db.isOpen()) {
        qWarning() << "按优先级统计失败：数据库未打开";
        return countMap;
    }

    QSqlQuery query("SELECT priority, COUNT(*) FROM tasks GROUP BY priority", db);
    if (query.exec()) {
        while (query.next()) {
            TaskPriority priority = static_cast<TaskPriority>(query.value(0).toInt());
            countMap[priority] = query.value(1).toInt();
        }
    } else {
        qCritical() << "按优先级统计失败：" << query.lastError().text();
    }

    db.close();
    return countMap;
}

QSqlDatabase TaskDatabase::getDatabaseConnection()
{
    return createDatabaseConnection();
}
