#include "TaskModel.h"
#include <QColor>

TaskModel::TaskModel(QObject *parent, QSqlDatabase db)
    : QSqlRelationalTableModel(parent, db)
{
    setTable("tasks");
    setRelation(3, QSqlRelation("categories", "id", "name")); // category_id列

    // 列头名称
    setHeaderData(0, Qt::Horizontal, tr("ID"));
    setHeaderData(1, Qt::Horizontal, tr("任务标题"));
    setHeaderData(2, Qt::Horizontal, tr("任务描述"));
    setHeaderData(3, Qt::Horizontal, tr("分类"));
    setHeaderData(4, Qt::Horizontal, tr("优先级"));
    setHeaderData(5, Qt::Horizontal, tr("截止时间"));
    setHeaderData(6, Qt::Horizontal, tr("完成状态"));
    setHeaderData(7, Qt::Horizontal, tr("创建时间"));
    setSort(5, Qt::AscendingOrder); // 按截止时间升序排序
    select();
}

QVariant TaskModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    int column = index.column();

    // 优先级：显示中文
    if (column == 4 && role == Qt::DisplayRole) {
        QModelIndex sourceIndex = this->index(index.row(), 4); // 注意：这里是优先级列的原始索引
        int priority = QSqlRelationalTableModel::data(sourceIndex, Qt::DisplayRole).toInt();
        return priorityToText(static_cast<TaskPriority>(priority));
    }

    // 分类列：显示分类名称
    if (column == 3 && role == Qt::DisplayRole) {
        return QSqlRelationalTableModel::data(index, role);
    }

    // 完成状态列：显示"已完成"/"未完成"
    if (column == 6) {
        if (role == Qt::DisplayRole) {
            bool isCompleted = QSqlRelationalTableModel::data(index, Qt::DisplayRole).toBool();
            return isCompleted ? tr("已完成") : tr("未完成");
        }
        if (role == Qt::ForegroundRole) {
            bool isCompleted = QSqlRelationalTableModel::data(index, Qt::DisplayRole).toBool();
            if (isCompleted) {
                return QBrush(QColor(128, 128, 128)); // 已完成项灰色
            }
        }
    }

    // 截止时间和创建时间列：格式化显示
    if ((column == 5 || column == 7) && role == Qt::DisplayRole) {
        QDateTime dateTime = QSqlRelationalTableModel::data(index, Qt::DisplayRole).toDateTime();
        return dateTime.toString("yyyy-MM-dd HH:mm");
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
