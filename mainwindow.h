#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QListWidgetItem>
#include <QMainWindow>
#include <QSqlTableModel>
#include <QSortFilterProxyModel>
#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QTextEdit>
#include <QMessageBox>
#include "TaskModel.h"
#include "ReminderWorker.h"
#include "ExportManager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // 功能按钮槽函数
    void on_addTaskBtn_clicked();
    void on_editTaskBtn_clicked();
    void on_deleteTaskBtn_clicked();
    void on_markCompletedBtn_clicked();
    void on_exportExcelBtn_clicked();
    void on_exportPdfBtn_clicked();

    // 分类过滤槽函数
    void on_categoryList_itemClicked(QListWidgetItem *item);

    // 搜索过滤槽函数
    void on_searchEdit_textChanged(const QString &text);

    // 优先级过滤槽函数
    void on_priorityCombo_currentIndexChanged(int index);

    // 提醒信号槽函数
    void showReminder(const QList<Task>& tasks);

private:
    Ui::MainWindow *ui;
    TaskModel* m_taskModel;
    QSortFilterProxyModel* m_proxyModel; // 过滤/排序代理模型
    ReminderWorker* m_reminderWorker;
    ExportManager* m_exportManager;

    // 初始化UI
    void initUI();

    // 加载分类列表
    void loadCategories();

    // 刷新任务表格
    void refreshTaskTable();

    // 显示任务编辑对话框（添加/编辑共用）
    bool showTaskEditDialog(Task& task, bool isEdit = false);
};

// 任务编辑对话框
class TaskEditDialog : public QDialog
{
    Q_OBJECT
public:
    explicit TaskEditDialog(const QList<Category>& categories, const Task& task = Task(), bool isEdit = false, QWidget *parent = nullptr);
    Task getTask() const;

private:
    QLineEdit* m_titleEdit;
    QTextEdit* m_descEdit;
    QComboBox* m_categoryCombo;
    QComboBox* m_priorityCombo;
    QDateTimeEdit* m_deadlineEdit;
    Task m_task;
    QList<Category> m_categories;
};

#endif // MAINWINDOW_H
