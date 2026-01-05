// MainWindow.cpp
#include "MainWindow.h"
#include "ui_MainWindow.h"
#include <QFileDialog>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    initUI();
}

MainWindow::~MainWindow()
{
    m_reminderWorker->quit();
    m_reminderWorker->wait();
    delete ui;
}

void MainWindow::initUI()
{
    // 初始化数据库
    if (!TaskDatabase::getInstance()->init()) {
        QMessageBox::critical(this, "错误", "数据库初始化失败，程序将退出！");
        qApp->quit();
    }

    // 初始化Model/View
    m_taskModel = new TaskModel(this, TaskDatabase::getInstance()->database());
    m_proxyModel = new QSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_taskModel);
    m_proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive); // 不区分大小写过滤
    ui->taskTable->setModel(m_proxyModel);
    ui->taskTable->setColumnHidden(0, true); // 隐藏ID列
    ui->taskTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch); // 列宽自适应
    ui->taskTable->setSelectionBehavior(QAbstractItemView::SelectRows); // 整行选择

    // 初始化优先级过滤下拉框
    ui->priorityCombo->addItem("全部优先级");
    ui->priorityCombo->addItem("低优先级");
    ui->priorityCombo->addItem("中优先级");
    ui->priorityCombo->addItem("高优先级");

    // 加载分类列表
    loadCategories();

    // 初始化后台提醒线程
    m_reminderWorker = new ReminderWorker();
    connect(m_reminderWorker, &ReminderWorker::reminderTriggered, this, &MainWindow::showReminder);
    m_reminderWorker->start();

    // 初始化导出管理器
    m_exportManager = new ExportManager();

    // 设置窗口标题
    setWindowTitle("个人工作与任务管理系统");
}

void MainWindow::loadCategories()
{
    ui->categoryList->clear();
    ui->categoryList->addItem("全部分类");
    QList<Category> categories = TaskDatabase::getInstance()->getAllCategories();
    for (const Category& cat : categories) {
        ui->categoryList->addItem(cat.name);
    }
    ui->categoryList->setCurrentRow(0);
}


// 任务编辑对话框实现
TaskEditDialog::TaskEditDialog(const QList<Category>& categories, const Task& task, bool isEdit, QWidget *parent)
    : QDialog(parent)
    , m_task(task)
    , m_categories(categories)
{
    setWindowTitle(isEdit ? "编辑任务" : "添加任务");
    setMinimumSize(400, 300);

    // 布局设置
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // 任务标题
    QHBoxLayout* titleLayout = new QHBoxLayout();
    QLabel* titleLabel = new QLabel("任务标题：");
    m_titleEdit = new QLineEdit(task.title);
    titleLayout->addWidget(titleLabel);
    titleLayout->addWidget(m_titleEdit);
    mainLayout->addLayout(titleLayout);

    // 任务描述
    QHBoxLayout* descLayout = new QHBoxLayout();
    QLabel* descLabel = new QLabel("任务描述：");
    m_descEdit = new QTextEdit(task.description);
    descLayout->addWidget(descLabel);
    descLayout->addWidget(m_descEdit);
    mainLayout->addLayout(descLayout);

    // 分类选择
    QHBoxLayout* catLayout = new QHBoxLayout();
    QLabel* catLabel = new QLabel("分类：");
    m_categoryCombo = new QComboBox();
    for (const Category& cat : categories) {
        m_categoryCombo->addItem(cat.name, cat.id);
        if (cat.id == task.categoryId) {
            m_categoryCombo->setCurrentIndex(m_categoryCombo->count() - 1);
        }
    }
    catLayout->addWidget(catLabel);
    catLayout->addWidget(m_categoryCombo);
    mainLayout->addLayout(catLayout);

    // 优先级选择
    QHBoxLayout* prioLayout = new QHBoxLayout();
    QLabel* prioLabel = new QLabel("优先级：");
    m_priorityCombo = new QComboBox();
    m_priorityCombo->addItem("低", Low);
    m_priorityCombo->addItem("中", Medium);
    m_priorityCombo->addItem("高", High);
    m_priorityCombo->setCurrentIndex(task.priority);
    prioLayout->addWidget(prioLabel);
    prioLayout->addWidget(m_priorityCombo);
    mainLayout->addLayout(prioLayout);

    // 截止时间
    QHBoxLayout* deadlineLayout = new QHBoxLayout();
    QLabel* deadlineLabel = new QLabel("截止时间：");
    m_deadlineEdit = new QDateTimeEdit(task.deadline.isValid() ? task.deadline : QDateTime::currentDateTime().addDays(1));
    m_deadlineEdit->setCalendarPopup(true);
    deadlineLayout->addWidget(deadlineLabel);
    deadlineLayout->addWidget(m_deadlineEdit);
    mainLayout->addLayout(deadlineLayout);

    // 按钮
    QHBoxLayout* btnLayout = new QHBoxLayout();
    QPushButton* okBtn = new QPushButton("确定");
    QPushButton* cancelBtn = new QPushButton("取消");
    btnLayout->addStretch();
    btnLayout->addWidget(okBtn);
    btnLayout->addWidget(cancelBtn);
    mainLayout->addLayout(btnLayout);

    // 信号槽连接
    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

Task TaskEditDialog::getTask() const
{
    Task task = m_task;
    task.title = m_titleEdit->text().trimmed();
    task.description = m_descEdit->toPlainText().trimmed();
    task.categoryId = m_categoryCombo->currentData().toInt();
    task.priority = static_cast<TaskPriority>(m_priorityCombo->currentData().toInt());
    task.deadline = m_deadlineEdit->dateTime();
    if (!task.id) { // 新增任务
        task.isCompleted = false;
        task.createTime = QDateTime::currentDateTime();
    }
    return task;
}

// 主窗口槽函数实现
void MainWindow::on_addTaskBtn_clicked()
{
    Task task;
    if (showTaskEditDialog(task)) {
        if (TaskDatabase::getInstance()->addTask(task)) {
            QMessageBox::information(this, "成功", "任务添加成功！");
            refreshTaskTable();
        } else {
            QMessageBox::warning(this, "失败", "任务添加失败！");
        }
    }
}

void MainWindow::on_editTaskBtn_clicked()
{
    QModelIndexList selectedRows = ui->taskTable->selectionModel()->selectedRows();
    if (selectedRows.isEmpty()) {
        QMessageBox::warning(this, "提示", "请选择要编辑的任务！");
        return;
    }

    QModelIndex proxyIndex = selectedRows.first();
    QModelIndex sourceIndex = m_proxyModel->mapToSource(proxyIndex);
    int taskId = m_taskModel->data(m_taskModel->index(sourceIndex.row(), 0)).toInt();

    // 获取当前任务
    QList<Task> tasks = TaskDatabase::getInstance()->getAllTasks();
    Task task;
    for (const Task& t : tasks) {
        if (t.id == taskId) {
            task = t;
            break;
        }
    }

    if (showTaskEditDialog(task, true)) {
        if (TaskDatabase::getInstance()->updateTask(task)) {
            QMessageBox::information(this, "成功", "任务编辑成功！");
            refreshTaskTable();
        } else {
            QMessageBox::warning(this, "失败", "任务编辑失败！");
        }
    }
}

void MainWindow::on_deleteTaskBtn_clicked()
{
    QModelIndexList selectedRows = ui->taskTable->selectionModel()->selectedRows();
    if (selectedRows.isEmpty()) {
        QMessageBox::warning(this, "提示", "请选择要删除的任务！");
        return;
    }

    if (QMessageBox::question(this, "确认", "确定要删除选中的任务吗？", QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
        return;
    }

    QModelIndex proxyIndex = selectedRows.first();
    QModelIndex sourceIndex = m_proxyModel->mapToSource(proxyIndex);
    int taskId = m_taskModel->data(m_taskModel->index(sourceIndex.row(), 0)).toInt();

    if (TaskDatabase::getInstance()->deleteTask(taskId)) {
        QMessageBox::information(this, "成功", "任务删除成功！");
        refreshTaskTable();
    } else {
        QMessageBox::warning(this, "失败", "任务删除失败！");
    }
}

void MainWindow::on_markCompletedBtn_clicked()
{
    QModelIndexList selectedRows = ui->taskTable->selectionModel()->selectedRows();
    if (selectedRows.isEmpty()) {
        QMessageBox::warning(this, "提示", "请选择要标记的任务！");
        return;
    }

    QModelIndex proxyIndex = selectedRows.first();
    QModelIndex sourceIndex = m_proxyModel->mapToSource(proxyIndex);
    int taskId = m_taskModel->data(m_taskModel->index(sourceIndex.row(), 0)).toInt();
    bool isCompleted = m_taskModel->data(m_taskModel->index(sourceIndex.row(), 6)).toBool();

    if (TaskDatabase::getInstance()->markTaskCompleted(taskId, !isCompleted)) {
        QMessageBox::information(this, "成功", "任务状态更新成功！");
        refreshTaskTable();
    } else {
        QMessageBox::warning(this, "失败", "任务状态更新失败！");
    }
}

void MainWindow::on_exportExcelBtn_clicked()
{
    QString filePath = QFileDialog::getSaveFileName(this, "导出Excel", "任务统计报表.xlsx", "Excel Files (*.xlsx)");
    if (filePath.isEmpty()) {
        return;
    }

    if (m_exportManager->exportToExcel(filePath)) {
        QMessageBox::information(this, "成功", "Excel报表导出成功！");
    } else {
        QMessageBox::warning(this, "失败", "Excel报表导出失败！");
    }
}

void MainWindow::on_exportPdfBtn_clicked()
{
    QString filePath = QFileDialog::getSaveFileName(this, "导出PDF", "任务统计报表.pdf", "PDF Files (*.pdf)");
    if (filePath.isEmpty()) {
        return;
    }

    if (m_exportManager->exportToPdf(filePath)) {
        QMessageBox::information(this, "成功", "PDF报表导出成功！");
    } else {
        QMessageBox::warning(this, "失败", "PDF报表导出失败！");
    }
}

void MainWindow::on_categoryList_itemClicked(QListWidgetItem *item)
{
    QString category = item->text();
    if (category == "全部分类") {
        m_proxyModel->setFilterFixedString(""); // 清空分类过滤
    } else {
        m_proxyModel->setFilterKeyColumn(3); // 按分类列过滤
        m_proxyModel->setFilterFixedString(category);
    }
}

void MainWindow::on_searchEdit_textChanged(const QString &text)
{
    m_proxyModel->setFilterKeyColumn(-1); // 全列搜索
    m_proxyModel->setFilterFixedString(text);
}

void MainWindow::on_priorityCombo_currentIndexChanged(int index)
{
    if (index == 0) {
        m_proxyModel->setFilterFixedString(""); // 全部优先级
    } else {
        QString priorityText = ui->priorityCombo->itemText(index);
        m_proxyModel->setFilterKeyColumn(4); // 按优先级列过滤
        m_proxyModel->setFilterFixedString(priorityText);
    }
}

void MainWindow::showReminder(const QList<Task>& tasks)
{
    QString reminderText = "以下任务即将截止：\n";
    for (const Task& task : tasks) {
        reminderText += QString("- %1（截止时间：%2）\n").arg(task.title).arg(task.deadline.toString("yyyy-MM-dd HH:mm"));
    }
    QMessageBox::information(this, "任务提醒", reminderText);
}

bool MainWindow::showTaskEditDialog(Task& task, bool isEdit)
{
    QList<Category> categories = TaskDatabase::getInstance()->getAllCategories();
    TaskEditDialog dialog(categories, task, isEdit, this);
    if (dialog.exec() == QDialog::Accepted) {
        task = dialog.getTask();
        if (task.title.isEmpty()) {
            QMessageBox::warning(this, "提示", "任务标题不能为空！");
            return false;
        }
        return true;
    }
    return false;
}

void MainWindow::refreshTaskTable()
{
    m_taskModel->select();
}
