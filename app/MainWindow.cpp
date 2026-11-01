#include "MainWindow.h"
#include "../contracts/ModuleFactories.h"
#include "../contracts/ToolModule.h"
#include <QTabWidget>
#include <QLabel>
#include <QProgressBar>
#include <QToolBar>
#include <QStatusBar>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget* parent):QMainWindow(parent)
{
	buildUi();
}

void MainWindow::buildUi()
{
	setWindowTitle("MediaWorkbench");
	resize(1100,700);

	m_tabs=new QTabWidget(this);
	m_video=createVideoModule(m_tabs);
	m_audio=createAudioModule(m_tabs);
	m_frame=createFrameModule(m_tabs);
	m_tabs->addTab(m_video,"视频工作台");
	m_tabs->addTab(m_audio,"音频工作台");
	m_tabs->addTab(m_frame,"图像帧工作台");
	setCentralWidget(m_tabs);

	auto* tb=addToolBar("main");
	auto* actOpen=tb->addAction("打开文件");
	auto* actSend=tb->addAction("发送到图像工作台");
	connect(actOpen,&QAction::triggered,this,[this](){
		QString path=QFileDialog::getOpenFileName(this,"打开","","Media (*.mp4 *.mkv *.wav *.mp3 *.png *.jpg)");
		if(path.isEmpty()) return;
		ToolModule* cur=qobject_cast<ToolModule*>(m_tabs->currentWidget());
		if(cur) cur->openFile(path);
	});
	connect(actSend,&QAction::triggered,this,&MainWindow::sendToFrame);

	m_status=new QLabel("就绪");
	m_prog=new QProgressBar;
	m_prog->setMaximumWidth(180);
	m_prog->setValue(0);
	statusBar()->addWidget(m_status,1);
	statusBar()->addPermanentWidget(m_prog);

	wireModule(m_video);
	wireModule(m_audio);
	wireModule(m_frame);
	connect(m_tabs,&QTabWidget::currentChanged,this,&MainWindow::onTabChanged);
	onTabChanged(0);
}

void MainWindow::wireModule(ToolModule* m)
{
	connect(m,&ToolModule::statusMessage,this,&MainWindow::onStatus);
	connect(m,&ToolModule::artifactReady,this,&MainWindow::onArtifact);
}

void MainWindow::onTabChanged(int index)
{
	for(int i=0;i<m_tabs->count();i++){
		ToolModule* m=qobject_cast<ToolModule*>(m_tabs->widget(i));
		if(m) m->setActive(i==index);
	}
}

void MainWindow::onStatus(const QString& msg)
{
	m_status->setText(msg);
}

void MainWindow::onArtifact(const QString& path,const QString& kind)
{
	m_status->setText("成果就绪:"+kind+" "+path);
	if(kind=="image") m_lastImage=path;
}

void MainWindow::sendToFrame()
{
	if(m_lastImage.isEmpty()){
		QMessageBox::information(this,"提示","还没有截帧图片");
		return;
	}
	m_tabs->setCurrentWidget(m_frame);
	m_frame->openFile(m_lastImage);
}
