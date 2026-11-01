#include "InvertFilter.h"

QImage InvertFilter::apply(const QImage& input) const
{
	QImage img=input.convertToFormat(QImage::Format_RGB32);
	for(int y=0;y<img.height();y++){
		QRgb* line=(QRgb*)img.scanLine(y);
		for(int x=0;x<img.width();x++){
			QRgb c=line[x];
			line[x]=qRgb(255-qRed(c),255-qGreen(c),255-qBlue(c));
		}
	}
	return img;
}
