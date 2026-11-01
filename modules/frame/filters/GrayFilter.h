#pragma once
#include "../../../contracts/IFrameFilter.h"

class GrayFilter:public IFrameFilter
{
public:
	QString name() const override{return "灰度";}
	QImage apply(const QImage& input) const override;
};
