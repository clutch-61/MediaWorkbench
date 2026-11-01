#include "FrameController.h"
#include "../../contracts/FFmpegHelper.h"
#include <QDir>
#include <QFile>
#include <QTransform>

FrameController::FrameController(QObject* parent):QObject(parent){}

bool FrameController::loadImage(const QString& path)
{
	QImage img(path);
	if(img.isNull()){
		emit error("图片打不开");
		return false;
	}
	m_src=img.convertToFormat(QImage::Format_RGB32);
	emit imageChanged();
	return true;
}

bool FrameController::loadFrameFromVideo(const QString& videoPath,double sec)
{
	if(videoPath.isEmpty()||!QFile::exists(videoPath)){
		emit error("视频不存在");
		return false;
	}
	QString tmp=QDir::temp().filePath("mw_frame.png");
	if(!FFmpegHelper::extractFrame(videoPath,sec,tmp)){
		emit error("取帧失败,检查ffmpeg");
		return false;
	}
	bool ok=loadImage(tmp);
	QFile::remove(tmp);
	return ok;
}

void FrameController::resetFilters(){m_filters.clear();emit imageChanged();}
void FrameController::addFilter(std::shared_ptr<IFrameFilter> f){m_filters.push_back(f);emit imageChanged();}
void FrameController::removeFilter(int index)
{
	if(index<0||index>=m_filters.size()) return;
	m_filters.removeAt(index);
	emit imageChanged();
}
void FrameController::moveFilter(int from,int to)
{
	if(from<0||from>=m_filters.size()||to<0||to>=m_filters.size()) return;
	m_filters.move(from,to);
	emit imageChanged();
}

void FrameController::rotate90()
{
	if(m_src.isNull()) return;
	QTransform t;t.rotate(90);
	m_src=m_src.transformed(t);
	emit imageChanged();
}

void FrameController::scaleTo(double percent)
{
	if(m_src.isNull()||percent<=0) return;
	int w=(int)(m_src.width()*percent/100.0);
	int h=(int)(m_src.height()*percent/100.0);
	if(w<1) w=1;if(h<1) h=1;
	m_src=m_src.scaled(w,h,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
	emit imageChanged();
}

QImage FrameController::preview() const
{
	QImage cur=m_src;
	for(const auto& f:m_filters) cur=f->apply(cur);
	return cur;
}

QImage FrameController::applyTo(const QImage& src) const
{
	QImage cur=src.convertToFormat(QImage::Format_RGB32);
	for(const auto& f:m_filters) cur=f->apply(cur);
	return cur;
}
