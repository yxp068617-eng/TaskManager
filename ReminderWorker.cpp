// ReminderWorker.cpp
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

        // 每分钟检查一次
        QThread::sleep(60);
    }
}

// 停止线程的方法
void ReminderWorker::stop()
{
    m_running = false;
}

ReminderWorker::~ReminderWorker()
{
    if (isRunning()) {
        stop();
        wait();  // 等待线程结束
    }
}
