#pragma once
#include "../../../contracts/IFrameFilter.h"

class BlurFilter:public IFrameFilter
{
public:
	explicit BlurFilter(int radius=1):m_radius(radius){}
	QString name() const override{return "模糊";}
	void setRadius(int r){m_radius=r;}
	int radius() const{return m_radius;}
	QImage apply(const QImage& input) const override;
private:
	int m_radius;
};
