#include "SegmentSlider.h"

#include <QPainter>
#include <QStyle>
#include <QStyleOptionSlider>

SegmentSlider::SegmentSlider(QWidget* parent)
    : QSlider(Qt::Horizontal, parent)
{
}

void SegmentSlider::setSegment(qint64 inMs, qint64 outMs)
{
    m_inMs = inMs;
    m_outMs = outMs;
    update();
}

void SegmentSlider::clearSegment()
{
    m_inMs = 0;
    m_outMs = 0;
    update();
}

void SegmentSlider::paintEvent(QPaintEvent* event)
{
    // 先画标准滑条
    QSlider::paintEvent(event);

    if (m_outMs <= m_inMs || maximum() <= minimum())
        return;

    QStyleOptionSlider opt;
    initStyleOption(&opt);
    const QRect groove =
        style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, this);
    if (groove.width() <= 0)
        return;

    const int x1 = QStyle::sliderPositionFromValue(minimum(), maximum(), int(m_inMs), groove.width());
    const int x2 = QStyle::sliderPositionFromValue(minimum(), maximum(), int(m_outMs), groove.width());
    const QRect band(groove.x() + x1, groove.y(), qMax(1, x2 - x1), groove.height());

    QPainter p(this);
    p.fillRect(band, QColor(37, 99, 235, 90));
    p.setPen(QColor(96, 165, 250));
    p.drawRect(band.adjusted(0, 0, -1, -1));
}
