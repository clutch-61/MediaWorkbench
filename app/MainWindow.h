#pragma once
#include <QMainWindow>
#include <QString>

class QTabWidget;
class QLabel;
class QProgressBar;
class ToolModule;

class MainWindow:public QMainWindow
{
	Q_OBJECT
public:
	explicit MainWindow(QWidget* parent=nullptr);

private slots:
	void onTabChanged(int index);
	void onStatus(const QString& msg);
	void onArtifact(const QString& path,const QString& kind);
	void sendToFrame();

private:
	void buildUi();
	void wireModule(ToolModule* m);

	QTabWidget* m_tabs=nullptr;
	QLabel* m_status=nullptr;
	QProgressBar* m_prog=nullptr;
	ToolModule* m_video=nullptr;
	ToolModule* m_audio=nullptr;
	ToolModule* m_frame=nullptr;
	QString m_lastImage;
};
