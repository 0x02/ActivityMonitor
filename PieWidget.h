#ifndef PIEWIDGET_H
#define PIEWIDGET_H

#include <QWidget>

class PieWidget : public QWidget
{
public:
    explicit PieWidget(QWidget *parent = 0);

    void SetColors(const QList<QColor>& colors) { m_Colors = colors; }
    void SetRatios(const QList<double>& ratios) { m_Ratios = ratios; }

private:
    virtual void paintEvent(QPaintEvent*);

private:
    QList<QColor> m_Colors;
    QList<double> m_Ratios;
};

#endif // PIEWIDGET_H
