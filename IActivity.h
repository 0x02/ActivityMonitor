#ifndef IACTIVITY_H
#define IACTIVITY_H

#include <QWidget>

class IActivity: public QWidget
{
    Q_OBJECT

public:
    IActivity(QWidget* parent): QWidget(parent) { }
    virtual ~IActivity() { }

    virtual void Refresh() = 0;//{ }
};

#endif // IACTIVITY_H
