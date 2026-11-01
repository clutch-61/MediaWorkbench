#include "../modules/frame/FrameModuleWidget.h"
#include "../modules/frame/filters/GrayFilter.h"
#include "common/AppStyle.h"
#include <QApplication>
#include <QImage>
#include <cassert>

static void checkFilter()
{
	QImage img(2,1,QImage::Format_RGB32);
	img.setPixel(0,0,qRgb(255,0,0));
	img.setPixel(1,0,qRgb(0,255,0));
	QImage g=GrayFilter().apply(img);
	assert(qRed(g.pixel(0,0))==qGreen(g.pixel(0,0)));
	assert(qRed(g.pixel(0,0))==qBlue(g.pixel(0,0)));
}

int main(int argc,char* argv[])
{
	QApplication a(argc,argv);
	a.setStyleSheet(appStyleSheet());
	checkFilter();
	FrameModuleWidget w;
	w.setWindowTitle("frame_demo");
	w.resize(900,600);
	w.show();
	return a.exec();
}
