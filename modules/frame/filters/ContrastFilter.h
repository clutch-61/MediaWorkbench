#pragma once
#include "../../../contracts/IFrameFilter.h"

class ContrastFilter:public IFrameFilter
{
public:
	explicit ContrastFilter(double factor=1.0):m_factor(factor){}
	QString name() const override{return "对比度";}
	void setFactor(double f){m_factor=f;}
	double factor() const{return m_factor;}
	QImage apply(const QImage& input) const override;
private:
	double m_factor;
};
