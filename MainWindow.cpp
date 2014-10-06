#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "FBSDMemory.h"

#include "FormatSize.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    //setWindowFlags(Qt::WindowStaysOnTopHint);
    ui->setupUi(this);
    SetupTabs();

    m_KVM.Open();

    connect(ui->editFilter, SIGNAL(textChanged(QString)), this, SLOT(SlotFilterProc(QString)));

    UpdateActivity();
    connect(&m_Timer, SIGNAL(timeout()), this, SLOT(UpdateActivity()));
    m_Timer.start(500);
}

MainWindow::~MainWindow()
{    
    m_Timer.stop();
    delete ui;
    m_KVM.Close();
}

void MainWindow::SetupTabs()
{    
    FBSDMemory* mem = new FBSDMemory;
    ui->tabWidget->addTab(mem, tr("Memory"));
}

void MainWindow::SlotFilterProc(QString text)
{
    if (text.isEmpty()) {
        m_KVM.ClearProcessFilter();
    } else {
        auto strs = text.split(QRegExp("\\s+"));
        std::vector<std::string> filter;
        filter.reserve(strs.size());
        for (int i = 0; i < strs.size(); ++i) {
            if (!strs[i].isEmpty()) {
                filter.push_back(strs[i].toLocal8Bit().data());
            }
        }
        m_KVM.SetProcessFilter(filter);
    }
}

void MainWindow::UpdateActivity()
{
    int ntab = ui->tabWidget->count();
    for (int i = 0; i < ntab; ++i) {
        IActivity* activity = dynamic_cast<IActivity*>(ui->tabWidget->widget(i));
        if (activity != NULL)
            activity->Refresh();
    }

    m_KVM.UpdateProcessInfo();
    const auto& procinfo = m_KVM.ProcessInfo();

    auto sel = ui->processTree->selectedItems();
    QSet<QString> pidsets;
    for (int i = 0; i < sel.size(); ++i) {
        pidsets.insert(sel[i]->text(0));
    }

    int n = procinfo.size() - ui->processTree->topLevelItemCount();
    if (n > 0) {
        ui->processTree->setColumnCount(7);
        QList<QTreeWidgetItem *> items;
        for (int i = 0; i < n; ++i)
             items.append(new QTreeWidgetItem());
        ui->processTree->addTopLevelItems(items);
    } else if (n < 0) {
        for (int i = n; i < 0; ++i) {
            delete ui->processTree->takeTopLevelItem(0);
        }
    }

    typedef decltype(procinfo[0]) proc_t;
    auto pcpu = [](const proc_t& proc) -> int {
        return 100 * proc.ki_pctcpu / FSCALE;
    };

    for (size_t i = 0; i < procinfo.size(); ++i) {
        auto item = ui->processTree->topLevelItem(i);
        item->setText(0, QString::number(procinfo[i].ki_pid));
        item->setText(1, QString::fromUtf8(procinfo[i].ki_comm));

        item->setText(2, PrettySize(procinfo[i].ki_rssize, m_PageInfo.shift));
        item->setTextAlignment(2, Qt::AlignRight);

        item->setText(3, QString::number(pcpu(procinfo[i])));
        item->setTextAlignment(3, Qt::AlignRight);

        item->setText(4, PrettySize(procinfo[i].ki_tsize, m_PageInfo.shift));
        item->setTextAlignment(4, Qt::AlignRight);

        item->setText(5, PrettySize(procinfo[i].ki_dsize, m_PageInfo.shift));
        item->setTextAlignment(5, Qt::AlignRight);

        item->setText(6, QString::number(procinfo[i].ki_numthreads));

        if (pidsets.contains(item->text(0))) {
            item->setSelected(true);
        } else {
            item->setSelected(false);
        }
    }
}
