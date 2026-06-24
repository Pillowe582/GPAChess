#ifndef REGISTRATION_H
#define REGISTRATION_H
#include <QDialog>
#include "state.h"
namespace Ui
{
    class registrationWindow;
}
class GameManager;

class RegistrationWindow : public QDialog
{
    Q_OBJECT
public:
    struct RegistrationInfo
    {
        RoundInfo roundInfo;
        bool isSucceed = true; // 是否中签
    };
    RegistrationWindow(QWidget *parent = nullptr, GameManager *gm = nullptr);
    ~RegistrationWindow();

protected:
    void showEvent(QShowEvent *event) override;

private:
    Ui::registrationWindow *ui;
    GameManager *m_gameManager;
    std::vector<RegistrationInfo *> m_regInfos;
    bool m_coursesChanged = false;

    void onCourseCountChanged(int count, GameManager &gameManager);
    void onSubmitCoursesClicked();
    void updateCourseList();

    void deleteDroppedCourses(std::vector<RoundInfo> &roundInfos, int &maxRounds);
};

#endif