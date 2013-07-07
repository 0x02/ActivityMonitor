#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtCore>
#include <QtGui>

#include "IActivity.h"
#include "FreeBSD.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private:
    void SetupTabs();

private slots:
    void SlotFilterProc(QString text);
    void UpdateActivity();

private:
    Ui::MainWindow *ui;
    QTimer m_Timer;
    IActivity* m_Activity;

    FreeBSD::KVM m_KVM;
    FreeBSD::PageInfo m_PageInfo;
};

#endif // MAINWINDOW_H
