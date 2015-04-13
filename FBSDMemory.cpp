#include "FBSDMemory.h"
#include "ui_FBSDMemory.h"
#include "FormatSize.h"

FBSDMemory::FBSDMemory(QWidget *parent) :
    IActivity(parent),
    ui(new Ui::FBSDMemory)
{
    ui->setupUi(this);

    ui->free->setStyleSheet("QLabel { color: rgb(40, 167, 0); }");
    ui->inactive->setStyleSheet("QLabel { color: rgb(24, 68, 195); }");
    ui->active->setStyleSheet("QLabel { color: rgb(167, 167, 0); }");
    ui->wired->setStyleSheet("QLabel { color: rgb(165, 19, 0); }");
    ui->cache->setStyleSheet("QLabel { color: DimGray; }");
    ui->buf->setStyleSheet("QLabel { color: Indigo; }");

    m_Pie = new PieWidget();
    m_Pie->resize(80, 80);
    m_Pie->setMaximumSize(80, 80);
    m_Pie->setMinimumSize(80, 80);
    ui->pieLayout->addWidget(m_Pie);

    QList<QColor> colors;
    colors << QColor(64, 250, 0) << QColor(43, 50, 255) << QColor(252, 251, 0)
           << QColor(249, 34, 0) << QColor(75, 0, 130) << QColor(105, 105, 105);

    QList<double> ratios;
    ratios << 0.8;

    m_Pie->SetColors(colors);
    m_Pie->SetRatios(ratios);

    m_KVM.Open();
}

FBSDMemory::~FBSDMemory()
{
    delete ui;

    m_KVM.Close();
}

void FBSDMemory::Refresh()
{    
    m_VMStats.Update();    

    ui->free->setText(PrettySize(m_VMStats.free, m_PageInfo.shift));
    ui->inactive->setText(PrettySize(m_VMStats.inactive, m_PageInfo.shift));
    ui->active->setText(PrettySize(m_VMStats.active, m_PageInfo.shift));
    ui->wired->setText(PrettySize(m_VMStats.wire, m_PageInfo.shift));
    ui->buf->setText(PrettySize(m_VMStats.buf));
    ui->cache->setText(PrettySize(m_VMStats.cache, m_PageInfo.shift));

    if (m_ZFSStats.HasArc()) {
        ui->arcWidget->show();
        m_ZFSStats.Update();
        ui->arc->setText(PrettySize(m_ZFSStats.arc_size));
        ui->mfu->setText(PrettySize(m_ZFSStats.mfu));
        ui->mru->setText(PrettySize(m_ZFSStats.mru));
        ui->anon->setText(PrettySize(m_ZFSStats.anon));
        ui->header->setText(PrettySize(m_ZFSStats.arc_hdr + m_ZFSStats.arc_l2hdr));
        ui->other->setText(PrettySize(m_ZFSStats.arc_other));
    } else {
        ui->arcWidget->hide();
    }

    m_KVM.UpdateSwapInfo();
    const auto& swapinfo = m_KVM.SwapInfo();
    if (!swapinfo.empty()) {
        ui->swapWidget->show();
        const auto& swap = swapinfo[swapinfo.size()-1];
        ui->pgin->setText(PrettySize(m_VMStats.swapin, m_PageInfo.shift));
        ui->pgout->setText(PrettySize(m_VMStats.swapout, m_PageInfo.shift));
        ui->swapUsed->setText(PrettySize(swap.ksw_used, m_PageInfo.shift));
        ui->swapTotal->setText(PrettySize(swap.ksw_total, m_PageInfo.shift));
        ui->swapFree->setText(PrettySize(swap.ksw_total-swap.ksw_used, m_PageInfo.shift));
    } else {
        ui->swapWidget->hide();
    }

    QList<double> ratios;
    ratios.reserve(6);
    {
        double total = m_VMStats.free + m_VMStats.inactive
                + m_VMStats.active + m_VMStats.wire + (m_VMStats.buf/m_PageInfo.size) + m_VMStats.cache;
        ratios << m_VMStats.free / total;
        ratios << m_VMStats.inactive / total;
        ratios << m_VMStats.active / total;
        ratios << m_VMStats.wire / total;
        ratios << (m_VMStats.buf/m_PageInfo.size) / total;
        ratios << m_VMStats.cache / total;

    }
    m_Pie->SetRatios(ratios);
}
