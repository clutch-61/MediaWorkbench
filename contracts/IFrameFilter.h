#pragma once
#include <QString>
#include <QImage>

//滤镜抽象接口 C模块用
class IFrameFilter
{
public:
	virtual ~IFrameFilter()=default;
	virtual QString name() const=0;
	virtual QImage apply(const QImage& input) const=0;
};
