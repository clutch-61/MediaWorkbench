#include "WaveformWidget.h"

#include <QMouseEvent>
#include <QPainter>

#include <algorithm>

namespace {
const int kHandleHitPx=6;

QString formatMs(qint64 ms)
{
	if(ms<0) ms=0;
	const qint64 totalSec=ms/1000;
	const qint64 m=totalSec/60;
	const qint64 s=totalSec%60;
	return QStringLiteral("%1:%2").arg(m).arg(s,2,10,QLatin1Char('0'));
}
}

WaveformWidget::WaveformWidget(QWidget* parent):QWidget(parent)
{
	setMinimumHeight(140);
	setMouseTracking(true);
	setCursor(Qt::PointingHandCursor);
}

void WaveformWidget::setWave(const QVector<WavePoint>& pts)
{
	m_pts=pts;
	update();
}

void WaveformWidget::setRange(qint64 inMs,qint64 outMs,qint64 durMs)
{
	m_inMs=qMax<qint64>(0,inMs);
	m_outMs=outMs;
	m_durMs=durMs>0?durMs:0;
	update();
}

void WaveformWidget::setPlayPos(qint64 ms)
{
	m_posMs=ms;
	update();
}

qint64 WaveformWidget::xToMs(int x) const
{
	if(m_durMs<=0||width()<=0) return 0;
	const qint64 ms=qint64(x)*m_durMs/width();
	return qBound<qint64>(0,ms,m_durMs);
}

int WaveformWidget::msToX(qint64 ms) const
{
	if(m_durMs<=0) return 0;
	return int(double(ms)*width()/double(m_durMs));
}

bool WaveformWidget::nearStart(int x) const
{
	return m_durMs>0&&qAbs(x-msToX(m_inMs))<=kHandleHitPx;
}

bool WaveformWidget::nearEnd(int x) const
{
	return m_durMs>0&&qAbs(x-msToX(m_outMs))<=kHandleHitPx;
}

void WaveformWidget::paintEvent(QPaintEvent*)
{
	QPainter p(this);
	p.fillRect(rect(),QColor(30,30,36));

	if(m_pts.isEmpty()){
		p.setPen(QColor(150,150,160));
		p.drawText(rect(),Qt::AlignCenter,QStringLiteral("暂无波形"));
		return;
	}

	const int w=width();
	const int h=height();
	const int mid=h/2;

	// 1) 先铺选区底色，波形画在它上面，否则波形被蓝色盖住看不清
	int xIn=0;
	int xOut=0;
	if(m_durMs>0){
		xIn=msToX(m_inMs);
		xOut=msToX(m_outMs);
		if(xOut>xIn)
			p.fillRect(QRect(xIn,0,xOut-xIn,h),QColor(37,99,235,55));
	}

	// 2) 中线
	p.setPen(QPen(QColor(70,70,82),1));
	p.drawLine(0,mid,w,mid);

	// 3) 波形：按像素分桶，每列取该列覆盖区间的 min/max
	//    播放位置左侧（已播过）亮色，右侧暗色
	const int playX=(m_durMs>0&&m_posMs>0)?msToX(m_posMs):-1;
	const QColor playedColor(140,205,255);
	const QColor restColor(70,112,168);
	const int n=m_pts.size();
	for(int x=0;x<w;++x){
		int i0=int(qint64(x)*n/w);
		int i1=int(qint64(x+1)*n/w);
		if(i1<=i0) i1=i0+1;
		if(i0>=n) break;

		float mn=0.0f;
		float mx=0.0f;
		bool first=true;
		for(int i=i0;i<i1&&i<n;++i){
			if(first){
				mn=m_pts.at(i).mn;
				mx=m_pts.at(i).mx;
				first=false;
			}else{
				mn=std::min(mn,m_pts.at(i).mn);
				mx=std::max(mx,m_pts.at(i).mx);
			}
		}
		if(first) continue;

		int y1=mid-int(mx*mid);
		int y2=mid-int(mn*mid);
		if(y1==y2) --y1;                 // 振幅为 0 时也留一条线
		p.setPen(x<=playX?playedColor:restColor);
		p.drawLine(x,y1,x,y2);
	}

	// 4) 选区边界 + 时间标注
	if(m_durMs>0){
		if(xOut>xIn){
			p.setPen(QPen(QColor(90,160,255),1));
			p.drawLine(xIn,0,xIn,h);
			p.drawLine(xOut,0,xOut,h);
		}

		p.setPen(QColor(220,220,220));
		p.drawText(4,h-6,formatMs(m_inMs));
		const QString outText=formatMs(m_outMs);
		p.drawText(w-4-p.fontMetrics().horizontalAdvance(outText),h-6,outText);
	}

	// 5) 播放位置
	if(m_durMs>0&&m_posMs>=0){
		const int xp=msToX(m_posMs);
		p.setPen(QPen(QColor(255,180,60),1));
		p.drawLine(xp,0,xp,h);
	}
}


void WaveformWidget::mousePressEvent(QMouseEvent* e)
{
	if(m_durMs<=0) return;

	m_pressX=e->position().toPoint().x();
	m_pressMs=xToMs(m_pressX);
	m_anchorMs=m_pressMs;
	m_dragged=false;

	if(nearStart(m_pressX)) m_drag=DragMode::MoveStart;
	else if(nearEnd(m_pressX)) m_drag=DragMode::MoveEnd;
	else m_drag=DragMode::Select;
}

void WaveformWidget::mouseMoveEvent(QMouseEvent* e)
{
	if(m_drag==DragMode::None||m_durMs<=0) return;

	const int x=e->position().toPoint().x();
	if(qAbs(x-m_pressX)>3) m_dragged=true;

	qint64 ms=xToMs(x);
	switch(m_drag){
	case DragMode::MoveStart:
		ms=qBound<qint64>(0,ms,m_outMs-1);
		if(ms<0) ms=0;
		m_inMs=ms;
		emit rangeChanged(m_inMs,m_outMs);
		break;
	case DragMode::MoveEnd:
		ms=qBound<qint64>(m_inMs+1,ms,m_durMs);
		m_outMs=ms;
		emit rangeChanged(m_inMs,m_outMs);
		break;
	case DragMode::Select:{
		qint64 a=m_anchorMs;
		qint64 b=ms;
		if(a>b) std::swap(a,b);
		if(b-a<10) return;
		m_inMs=a;
		m_outMs=b;
		emit rangeChanged(m_inMs,m_outMs);
		break;
	}
	default:
		break;
	}
	update();
}

void WaveformWidget::mouseReleaseEvent(QMouseEvent* e)
{
	if(m_drag==DragMode::Select&&!m_dragged)
		emit seekRequested(m_pressMs);

	// 拖拽移动手柄时，落点再修正一次，保证边界合法
	if(m_drag==DragMode::MoveStart&&m_inMs>=m_outMs)
		m_inMs=qMax<qint64>(0,m_outMs-1);
	if(m_drag==DragMode::MoveEnd&&m_outMs<=m_inMs)
		m_outMs=qMin(m_inMs+1,m_durMs);

	m_drag=DragMode::None;
	m_dragged=false;
	Q_UNUSED(e);
	update();
}
