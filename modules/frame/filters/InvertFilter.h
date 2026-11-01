#pragma once
#include "../../../contracts/IFrameFilter.h"

class InvertFilter:public IFrameFilter
{
public:
	QString name() const override{return "反色";}
	QImage apply(const QImage& input) const override;
};
