#include "TaskModel.h"
#include <QColor>

TaskModel::TaskModel(QObject *parent, QSqlDatabase db)
    : QSqlRelationalTableModel(parent, db)
{
    // 设置关联表（任务表与分类表）
    setTable("tasks");
    setRelation(3, QSqlRelation("categories", "id", "name")); // 第3列（category_id）关联分类表名称
    setSort(5, Qt::AscendingOrder); // 按截止时间升序排序
    select(); // 加载数据
}

QVariant TaskModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    // 优先级列（第4列）：显示中文
    if (index.column() == 4 && role == Qt::DisplayRole) {
        int priority = QSqlRelationalTableModel::data(index, Qt::DisplayRole).toInt();
        return priorityToText(static_cast<TaskPriority>(priority));
    }

    // 完成状态列（第6列）：显示"已完成"/"未完成"，且已完成项标灰
    if (index.column() == 6) {
        bool isCompleted = QSqlRelationalTableModel::data(index, Qt::DisplayRole).toBool();
        if (role == Qt::DisplayRole) {
            return isCompleted ? "已完成" : "未完成";
        }
        if (role == Qt::ForegroundRole && isCompleted) {
            return QBrush(QColor(128, 128, 128)); // 已完成项灰色
        }
    }

    // 截止时间列（第5列）：格式化时间显示
    if (index.column() == 5 && role == Qt::DisplayRole) {
        QDateTime deadline = QSqlRelationalTableModel::data(index, Qt::DisplayRole).toDateTime();
        return deadline.toString("yyyy-MM-dd HH:mm");
    }

    // 创建时间列（第7列）：格式化时间显示
    if (index.column() == 7 && role == Qt::DisplayRole) {
        QDateTime createTime = QSqlRelationalTableModel::data(index, Qt::DisplayRole).toDateTime();
        return createTime.toString("yyyy-MM-dd HH:mm");
    }

    return QSqlRelationalTableModel::data(index, role);
}

QVariant TaskModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        // 自定义列名
        switch (section) {
        case 0: return "ID";
        case 1: return "任务标题";
        case 2: return "任务描述";
        case 3: return "分类";
        case 4: return "优先级";
        case 5: return "截止时间";
        case 6: return "完成状态";
        case 7: return "创建时间";
        default: return QSqlRelationalTableModel::headerData(section, orientation, role);
        }
    }
    return QSqlRelationalTableModel::headerData(section, orientation, role);
}

QString TaskModel::priorityToText(TaskPriority priority) const
{
    switch (priority) {
    case Low: return "低";
    case Medium: return "中";
    case High: return "高";
    default: return "未知";
    }
}
