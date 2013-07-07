#ifndef FBSDMEMORY_H
#define FBSDMEMORY_H

#include <QWidget>

#include "FreeBSD.h"
#include "IActivity.h"
#include "PieWidget.h"

namespace Ui {
class FBSDMemory;
}

class FBSDMemory: public IActivity
{
    Q_OBJECT
    
public:
    explicit FBSDMemory(QWidget *parent = 0);
    ~FBSDMemory();
    
    virtual void Refresh();

private:
    Ui::FBSDMemory *ui;
    PieWidget* m_Pie;
    FreeBSD::VMStats m_VMStats;
    FreeBSD::ZFSStats m_ZFSStats;
    FreeBSD::KVM m_KVM;
    FreeBSD::PageInfo m_PageInfo;
};

#endif // FBSDMEMORY_H
