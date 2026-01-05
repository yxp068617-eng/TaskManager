#ifndef REMINDERWORKER_H
#define REMINDERWORKER_H

#include <QThread>
#include <QList>
#include "TaskDatabase.h"

class ReminderWorker : public QThread
{
    Q_OBJECT
public:
    ReminderWorker() = default;
    ~ReminderWorker() override;

    void stop(); // 停止线程的方法

signals:
    // 发送提醒信号（任务列表）
    void reminderTriggered(const QList<Task>& tasks);

protected:
    void run() override; // 线程执行函数

private:
    bool m_running = true; // 线程运行标志
};

#endif // REMINDERWORKER_H
