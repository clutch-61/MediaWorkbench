#pragma once
#include <QWidget>
#include <QVector>

#include "AudioController.h"

// 自己画波形：按像素分桶，支持拖拽出处理区间
class WaveformWidget:public QWidget
{
	Q_OBJECT
public:
	explicit WaveformWidget(QWidget* parent=nullptr);

	void setWave(const QVector<WavePoint>& pts);
	void setRange(qint64 inMs,qint64 outMs,qint64 durMs);
	void setPlayPos(qint64 ms);

	qint64 inPoint() const{ return m_inMs; }
	qint64 outPoint() const{ return m_outMs; }

signals:
	void seekRequested(qint64 ms);
	void rangeChanged(qint64 inMs,qint64 outMs);

protected:
	void paintEvent(QPaintEvent* e) override;
	void mousePressEvent(QMouseEvent* e) override;
	void mouseMoveEvent(QMouseEvent* e) override;
	void mouseReleaseEvent(QMouseEvent* e) override;

private:
	enum class DragMode{ None,Select,MoveStart,MoveEnd };

	qint64 xToMs(int x) const;
	int msToX(qint64 ms) const;
	bool nearStart(int x) const;
	bool nearEnd(int x) const;

	QVector<WavePoint> m_pts;
	qint64 m_inMs=0;
	qint64 m_outMs=0;
	qint64 m_durMs=0;
	qint64 m_posMs=0;

	DragMode m_drag=DragMode::None;
	qint64 m_anchorMs=0;
	qint64 m_pressMs=0;
	int m_pressX=0;
	bool m_dragged=false;
};
