#ifndef TASKMODEL_H
#define TASKMODEL_H

#include <QSqlRelationalTableModel>
#include <QVariant>
#include <QBrush>
#include "TaskDatabase.h"

class TaskModel : public QSqlRelationalTableModel
{
    Q_OBJECT
public:
    explicit TaskModel(QObject *parent = nullptr, QSqlDatabase db = QSqlDatabase());

    // 重写data方法：自定义显示（优先级中文、完成状态颜色）
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // 重写headerData：设置列名
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

private:
    // 优先级转中文
    QString priorityToText(TaskPriority priority) const;
};

#endif // TASKMODEL_H
