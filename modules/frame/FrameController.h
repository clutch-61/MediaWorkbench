#pragma once
#include <QObject>
#include <QImage>
#include <QString>
#include <QVector>
#include <memory>
#include "../../contracts/IFrameFilter.h"

class FrameController:public QObject
{
	Q_OBJECT
public:
	explicit FrameController(QObject* parent=nullptr);

	bool loadImage(const QString& path);
	bool loadFrameFromVideo(const QString& videoPath,double sec);
	void resetFilters();
	void addFilter(std::shared_ptr<IFrameFilter> f);
	void removeFilter(int index);
	void moveFilter(int from,int to);
	void rotate90();
	void scaleTo(double percent);
	QImage preview() const;
	QImage applyTo(const QImage& src) const;
	QImage original() const{return m_src;}
	bool hasImage() const{return !m_src.isNull();}
	const QVector<std::shared_ptr<IFrameFilter>>& filters() const{return m_filters;}

signals:
	void imageChanged();
	void error(const QString& msg);

private:
	QImage m_src;
	QVector<std::shared_ptr<IFrameFilter>> m_filters;
};
