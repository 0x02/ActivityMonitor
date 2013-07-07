#include "PieWidget.h"
#include <QtCore>
#include <QtGui>

PieWidget::PieWidget(QWidget *parent) :
    QWidget(parent)
{
}

void PieWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);

    const double size = width();
    const double radius = size/2;
    const double margin = 8;
    const QRectF rect(margin/2, margin/2, size-margin, size-margin);
    const QRectF bgRect(0, 0, size, size);

    QRadialGradient gradient(radius, radius, radius, radius, radius, 0);
    gradient.setColorAt(1-margin/size, QColor("Silver"));
    gradient.setColorAt(1, palette().background().color());
    painter.setBrush(gradient);
    painter.drawEllipse(bgRect);

    int startAngle = 90 * 16;
    int nPie = qMin(m_Ratios.size(), m_Colors.size());
    for (int i = 0; i < nPie; ++i) {
        int spanAngle = m_Ratios[i] * 360 * 16;                
        painter.setBrush(m_Colors[i]);
        painter.drawPie(rect, startAngle, spanAngle);
        startAngle += spanAngle;
    }


}
