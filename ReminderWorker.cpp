#include "ReminderWorker.h"
#include <QTimer>
#include <QDebug>

void ReminderWorker::run()
{
    m_running = true;

    while (m_running) {
        // 获取待提醒任务
        QList<Task> reminderTasks = TaskDatabase::getInstance()->getReminderTasks();

        if (!reminderTasks.isEmpty()) {
            // 发送提醒信号
            emit reminderTriggered(reminderTasks);
        }

        QThread::sleep(60);
    }
}

// 停止线程
void ReminderWorker::stop()
{
    m_running = false;
}

ReminderWorker::~ReminderWorker()
{
    if (isRunning()) {
        stop();
        wait();
    }
}
