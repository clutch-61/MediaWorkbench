#include "GrayFilter.h"

QImage GrayFilter::apply(const QImage& input) const
{
	QImage img=input.convertToFormat(QImage::Format_RGB32);
	for(int y=0;y<img.height();y++){
		QRgb* line=(QRgb*)img.scanLine(y);
		for(int x=0;x<img.width();x++){
			int g=qGray(line[x]);
			line[x]=qRgb(g,g,g);
		}
	}
	return img;
}
