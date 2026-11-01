#include "common/AppStyle.h"
#include "contracts/ModuleFactories.h"
#include <QApplication>

int main(int argc,char** argv)
{
	QApplication app(argc,argv);
	app.setStyleSheet(appStyleSheet());
	ToolModule* mod=createVideoModule();
	mod->setWindowTitle("video_demo");
	mod->resize(1200,720);
	mod->show();
	if(argc>1) mod->openFile(QString::fromLocal8Bit(argv[1]));
	return app.exec();
}
