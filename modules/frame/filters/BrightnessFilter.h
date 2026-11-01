#pragma once
#include "../../../contracts/IFrameFilter.h"

class BrightnessFilter:public IFrameFilter
{
public:
	explicit BrightnessFilter(int delta=0):m_delta(delta){}
	QString name() const override{return "亮度";}
	void setDelta(int d){m_delta=d;}
	int delta() const{return m_delta;}
	QImage apply(const QImage& input) const override;
private:
	int m_delta;
};
