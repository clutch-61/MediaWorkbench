#include "MainWindow.h"
#include "common/AppStyle.h"
#include <QApplication>

int main(int argc,char* argv[])
{
	QApplication a(argc,argv);
	a.setStyleSheet(appStyleSheet());
	MainWindow w;
	w.show();
	return a.exec();
}
