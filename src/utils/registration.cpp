#include "registration.h"
#include "ui_registration.h"
#include "game_manager.h"
#include "print.h"

#include <QRandomGenerator>
#include <QMessageBox>
// 基本窗口属性
RegistrationWindow::RegistrationWindow(QWidget *parent, GameManager *gm)
    : QDialog(parent), ui(new Ui::registrationWindow), m_gameManager(gm)
{
    ui->setupUi(this);

    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint);
    setWindowModality(Qt::WindowModal);
    m_coursesChanged = false;

    // 提交选课按钮点击事件
    connect(ui->submitCourses, &QPushButton::clicked, this, &RegistrationWindow::onSubmitCoursesClicked);

    // 选课数改变
    QIntValidator *validator = new QIntValidator(1, 1145, this);
    ui->roundCountEdit->setValidator(validator);
    connect(ui->roundCountEdit, &QLineEdit::editingFinished, this, [this]()
            {
        bool ok;
        int count = ui->roundCountEdit->text().toInt(&ok);
        if (ok)
            onCourseCountChanged(count, *m_gameManager); });
    updateCourseList();
}

RegistrationWindow::~RegistrationWindow()
{
    delete ui;
}

void RegistrationWindow::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    print("显示选课界面");
}

// % 选课数改变时
void RegistrationWindow::onCourseCountChanged(int count, GameManager &gameManager)
{
    gameManager.generateRoundInfos(gameManager.getRoundInfos(), count);
    m_regInfos.clear();
    for (const RoundInfo &info : gameManager.getRoundInfos())
    {
        m_regInfos.push_back(new RegistrationInfo{info});
    }
    updateCourseList();
    m_coursesChanged = true;
}

// % 刷新选课列表
void RegistrationWindow::updateCourseList()
{
    ui->courseList->clear();
    bool hasCourse = !m_regInfos.empty();
    ui->submitCourses->setEnabled(hasCourse);
    for (const RegistrationInfo *regInfo : m_regInfos)
    {
        const RoundInfo &info = regInfo->roundInfo;

        QString courseText = QString(
                                 "%1第%2门     %3cr")
                                 .arg(regInfo->isSucceed ? "" : "候选：")
                                 .arg(info.roundNumber)
                                 .arg(info.creditWorth);

        QListWidgetItem *item = new QListWidgetItem(courseText);
        QFont font;
        font.setPointSize(12);
        font.setBold(1);
        item->setFont(font);
        item->setForeground(regInfo->isSucceed ? QColor("#ee971d") : QColor("#5ba3d1"));

        ui->courseList->addItem(item);
    }
}

// % 提交选课
void RegistrationWindow::onSubmitCoursesClicked()
{
    print("提交选课");
    // 显示提示
    QMessageBox msgBox = QMessageBox();

    if (m_coursesChanged)
    {
        QRandomGenerator *rng = QRandomGenerator::global();
        int droppedCourses = 0;
        for (RegistrationInfo *regInfo : m_regInfos)
        {
            if (rng->bounded(0, 10) != 0) // 10% 概率掉课
            {
                continue;
            }
            regInfo->isSucceed = false;
            droppedCourses++;
            print(QString("第 %1 门课掉课！").arg(regInfo->roundInfo.roundNumber));
        }
        if (droppedCourses >= m_regInfos.size())
        {
            print("全掉了也是无敌了");
            droppedCourses--;
            m_regInfos[0]->isSucceed = true; // 至少保留一门课
        }
        m_coursesChanged = false;
        updateCourseList();
        msgBox.setWindowTitle("抽签完成");
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setText("喜报：你掉了 " + QString::number(droppedCourses) + " 门课" +
                       (droppedCourses == 0 ? "，蓝零王！            " : "               "));
        QPushButton *acceptBtn = msgBox.addButton("就这样吧", QMessageBox::AcceptRole);
        QPushButton *retryBtn = msgBox.addButton("重开", QMessageBox::RejectRole);
        msgBox.setDefaultButton(acceptBtn); // 设置默认按钮（按 Enter 触发）
        msgBox.exec();
        if (msgBox.clickedButton() == acceptBtn)
        {
            // 就这样吧
            print("掉了就掉了吧");
            deleteDroppedCourses(m_gameManager->getRoundInfos(), m_gameManager->getMaxRounds());
            this->close();
        }
    }
    else
    {
        msgBox.setWindowTitle("不补退选了？");
        msgBox.setText("编辑一下输入框可以重新抽卡的                       ");
        msgBox.setIcon(QMessageBox::Question);
        QPushButton *acceptBtn = msgBox.addButton("1", QMessageBox::AcceptRole);
        QPushButton *retryBtn = msgBox.addButton("0", QMessageBox::RejectRole);
        msgBox.exec();
        if (msgBox.clickedButton() == acceptBtn)
        {
            print("开学！");
            deleteDroppedCourses(m_gameManager->getRoundInfos(), m_gameManager->getMaxRounds());
            this->close();
        }
    }
}

// % 删除掉了的课
void RegistrationWindow::deleteDroppedCourses(std::vector<RoundInfo> &roundInfos,
                                              int &maxRounds)
{
    std::vector<RoundInfo> newRoundInfos;
    int roundCount = 0;
    for (size_t i = 0; i < m_regInfos.size(); ++i)
    {
        if (m_regInfos[i]->isSucceed)
        {
            ++roundCount;
            newRoundInfos.push_back(m_regInfos[i]->roundInfo);
            newRoundInfos.back().roundNumber = roundCount; // 重新编号
        }
    }
    roundInfos = newRoundInfos;
    maxRounds = roundCount;
}