#include "BlurFilter.h"

QImage BlurFilter::apply(const QImage& input) const
{
	QImage src=input.convertToFormat(QImage::Format_RGB32);
	QImage dst=src.copy();
	int r=m_radius<1?1:m_radius;
	int w=src.width(),h=src.height();
	for(int y=0;y<h;y++){
		QRgb* out=(QRgb*)dst.scanLine(y);
		for(int x=0;x<w;x++){
			int sumR=0,sumG=0,sumB=0,cnt=0;
			for(int dy=-r;dy<=r;dy++){
				int yy=y+dy;
				if(yy<0||yy>=h) continue;
				const QRgb* line=(const QRgb*)src.scanLine(yy);
				for(int dx=-r;dx<=r;dx++){
					int xx=x+dx;
					if(xx<0||xx>=w) continue;
					QRgb c=line[xx];
					sumR+=qRed(c);sumG+=qGreen(c);sumB+=qBlue(c);
					cnt++;
				}
			}
			out[x]=qRgb(sumR/cnt,sumG/cnt,sumB/cnt);
		}
	}
	return dst;
}
