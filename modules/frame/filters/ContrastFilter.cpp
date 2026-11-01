#include "ContrastFilter.h"

QImage ContrastFilter::apply(const QImage& input) const
{
	QImage img=input.convertToFormat(QImage::Format_RGB32);
	for(int y=0;y<img.height();y++){
		QRgb* line=(QRgb*)img.scanLine(y);
		for(int x=0;x<img.width();x++){
			QRgb c=line[x];
			int r=qBound(0,(int)((qRed(c)-128)*m_factor+128),255);
			int g=qBound(0,(int)((qGreen(c)-128)*m_factor+128),255);
			int b=qBound(0,(int)((qBlue(c)-128)*m_factor+128),255);
			line[x]=qRgb(r,g,b);
		}
	}
	return img;
}
