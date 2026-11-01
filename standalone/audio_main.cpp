#include "../modules/audio/AudioModuleWidget.h"
#include "common/AppStyle.h"
#include <QApplication>

int main(int argc,char* argv[])
{
	QApplication a(argc,argv);
	a.setStyleSheet(appStyleSheet());
	AudioModuleWidget w;
	w.setWindowTitle("audio_demo");
	w.resize(1100,700);
	w.show();
	if(argc>1) w.openFile(QString::fromLocal8Bit(argv[1]));
	return a.exec();
}
