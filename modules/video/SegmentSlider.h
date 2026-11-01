#pragma once

#include <QSlider>

// 在进度条上以高亮色标出导出选区（起点到终点）。
// 区间值单位与进度条一致（毫秒）。
class SegmentSlider : public QSlider
{
    Q_OBJECT

public:
    explicit SegmentSlider(QWidget* parent = nullptr);

    void setSegment(qint64 inMs, qint64 outMs);
    void clearSegment();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    qint64 m_inMs = 0;
    qint64 m_outMs = 0;
};
