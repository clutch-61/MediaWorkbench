#include "BrightnessFilter.h"

QImage BrightnessFilter::apply(const QImage& input) const
{
	QImage img=input.convertToFormat(QImage::Format_RGB32);
	for(int y=0;y<img.height();y++){
		QRgb* line=(QRgb*)img.scanLine(y);
		for(int x=0;x<img.width();x++){
			QRgb c=line[x];
			int r=qBound(0,qRed(c)+m_delta,255);
			int g=qBound(0,qGreen(c)+m_delta,255);
			int b=qBound(0,qBlue(c)+m_delta,255);
			line[x]=qRgb(r,g,b);
		}
	}
	return img;
}
