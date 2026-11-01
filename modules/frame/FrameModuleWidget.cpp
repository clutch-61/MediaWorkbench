#include "FrameModuleWidget.h"
#include "filters/GrayFilter.h"
#include "filters/InvertFilter.h"
#include "filters/BrightnessFilter.h"
#include "filters/ContrastFilter.h"
#include "filters/BlurFilter.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QProgressBar>
#include <QScrollArea>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QCheckBox>
#include <QFileInfo>
#include <QApplication>
#include <memory>

FrameModuleWidget::FrameModuleWidget(QWidget* parent):ToolModule(parent)
{
	m_ctrl=new FrameController(this);
	buildUi();
	connect(m_ctrl,&FrameController::imageChanged,this,&FrameModuleWidget::refreshPreview);
	connect(m_ctrl,&FrameController::error,this,[this](const QString& e){
		QMessageBox::warning(this,"错误",e);
		emit statusMessage(e);
	});
}

void FrameModuleWidget::buildUi()
{
	auto* root=new QHBoxLayout(this);
	auto* split=new QSplitter(Qt::Horizontal,this);

	auto* left=new QWidget;
	auto* lv=new QVBoxLayout(left);
	auto* area=new QScrollArea;
	m_view=new QLabel("未加载图片");
	m_view->setAlignment(Qt::AlignCenter);
	m_view->setMinimumSize(200,200);
	area->setWidget(m_view);
	area->setWidgetResizable(true);
	lv->addWidget(area,1);
	m_showOrig=new QCheckBox("看原图");
	lv->addWidget(m_showOrig);
	connect(m_showOrig,&QCheckBox::toggled,this,&FrameModuleWidget::refreshPreview);

	auto* right=new QWidget;
	auto* rv=new QVBoxLayout(right);
	auto* btnOpen=new QPushButton("打开图片");
	auto* btnVid=new QPushButton("从视频导入");
	auto* btnRot=new QPushButton("旋转90度");
	auto* btnScale=new QPushButton("缩放");
	m_list=new QListWidget;
	auto* btnAdd=new QPushButton("+ 添加滤镜");
	auto* btnDel=new QPushButton("- 移除");
	auto* btnUp=new QPushButton("上移");
	auto* btnDown=new QPushButton("下移");
	auto* btnReset=new QPushButton("重置滤镜");
	m_btnExport=new QPushButton("导出当前图");
	m_btnExport->setObjectName("primaryButton");
	m_btnBatch=new QPushButton("批量应用导出");
	m_btnCancel=new QPushButton("取消");
	m_btnCancel->setEnabled(false);
	m_prog=new QProgressBar;m_prog->setValue(0);
	rv->addWidget(btnOpen);
	rv->addWidget(btnVid);
	rv->addWidget(btnRot);
	rv->addWidget(btnScale);
	rv->addWidget(new QLabel("滤镜链"));
	rv->addWidget(m_list,1);
	rv->addWidget(btnAdd);
	rv->addWidget(btnDel);
	rv->addWidget(btnUp);
	rv->addWidget(btnDown);
	rv->addWidget(btnReset);
	rv->addWidget(m_btnExport);
	rv->addWidget(m_btnBatch);
	rv->addWidget(m_btnCancel);
	rv->addWidget(m_prog);

	split->addWidget(left);
	split->addWidget(right);
	split->setStretchFactor(0,7);
	split->setStretchFactor(1,3);
	root->addWidget(split);

	connect(btnOpen,&QPushButton::clicked,this,&FrameModuleWidget::onOpenImage);
	connect(btnVid,&QPushButton::clicked,this,&FrameModuleWidget::onOpenVideoFrame);
	connect(btnAdd,&QPushButton::clicked,this,&FrameModuleWidget::onAddFilter);
	connect(m_btnExport,&QPushButton::clicked,this,&FrameModuleWidget::onExport);
	connect(m_btnBatch,&QPushButton::clicked,this,&FrameModuleWidget::onBatch);
	connect(m_btnCancel,&QPushButton::clicked,this,&FrameModuleWidget::onCancel);
	connect(btnRot,&QPushButton::clicked,this,[this](){m_ctrl->rotate90();});
	connect(btnScale,&QPushButton::clicked,this,[this](){
		bool ok=false;
		double p=QInputDialog::getDouble(this,"缩放","百分比:",50,1,400,1,&ok);
		if(ok) m_ctrl->scaleTo(p);
	});
	connect(btnReset,&QPushButton::clicked,this,[this](){
		m_ctrl->resetFilters();
		m_list->clear();
	});
	connect(btnDel,&QPushButton::clicked,this,[this](){
		int i=m_list->currentRow();
		if(i<0) return;
		m_ctrl->removeFilter(i);
		delete m_list->takeItem(i);
	});
	connect(btnUp,&QPushButton::clicked,this,[this](){
		int i=m_list->currentRow();
		if(i<0){QMessageBox::information(this,"提示","先点一下滤镜列表里的某一项");return;}
		if(i==0) return; //已经在最上面
		m_ctrl->moveFilter(i,i-1);
		auto* it=m_list->takeItem(i);
		m_list->insertItem(i-1,it);
		m_list->setCurrentRow(i-1);
	});
	connect(btnDown,&QPushButton::clicked,this,[this](){
		int i=m_list->currentRow();
		if(i<0){QMessageBox::information(this,"提示","先点一下滤镜列表里的某一项");return;}
		if(i>=m_list->count()-1) return; //已经在最下面
		m_ctrl->moveFilter(i,i+1);
		auto* it=m_list->takeItem(i);
		m_list->insertItem(i+1,it);
		m_list->setCurrentRow(i+1);
	});
}

void FrameModuleWidget::openFile(const QString& filePath)
{
	if(!m_ctrl->loadImage(filePath)) return;
	emit statusMessage("图片已加载:"+filePath);
}

void FrameModuleWidget::setActive(bool){}

void FrameModuleWidget::onOpenImage()
{
	QString path=QFileDialog::getOpenFileName(this,"打开图片","","Image (*.png *.jpg *.jpeg)");
	if(!path.isEmpty()) openFile(path);
}

void FrameModuleWidget::onOpenVideoFrame()
{
	QString path=QFileDialog::getOpenFileName(this,"选择视频","","Video (*.mp4 *.mkv *.avi)");
	if(path.isEmpty()) return;
	bool ok=false;
	double sec=QInputDialog::getDouble(this,"时间点","秒:",1,0,99999,2,&ok);
	if(!ok) return;
	emit statusMessage("正在取帧...");
	if(m_ctrl->loadFrameFromVideo(path,sec))
		emit statusMessage("取帧成功");
}

void FrameModuleWidget::onAddFilter()
{
	if(!m_ctrl->hasImage()){
		QMessageBox::information(this,"提示","先打开图片");
		return;
	}
	QStringList names={"灰度","反色","亮度","对比度","模糊"};
	bool ok=false;
	QString name=QInputDialog::getItem(this,"添加滤镜","选择:",names,0,false,&ok);
	if(!ok) return;
	std::shared_ptr<IFrameFilter> f;
	if(name=="灰度") f=std::make_shared<GrayFilter>();
	else if(name=="反色") f=std::make_shared<InvertFilter>();
	else if(name=="亮度"){
		int d=QInputDialog::getInt(this,"亮度","-100~100:",20,-100,100,1,&ok);
		if(!ok) return;
		f=std::make_shared<BrightnessFilter>(d);
	}else if(name=="对比度"){
		double fac=QInputDialog::getDouble(this,"对比度","倍数:",1.2,0.1,3.0,1,&ok);
		if(!ok) return;
		f=std::make_shared<ContrastFilter>(fac);
	}else{
		int r=QInputDialog::getInt(this,"模糊","半径:",1,1,5,1,&ok);
		if(!ok) return;
		f=std::make_shared<BlurFilter>(r);
	}
	m_ctrl->addFilter(f);
	m_list->addItem(f->name());
}

void FrameModuleWidget::onExport()
{
	if(!m_ctrl->hasImage()){
		QMessageBox::information(this,"提示","没有图片");
		return;
	}
	QString path=QFileDialog::getSaveFileName(this,"导出","","PNG (*.png);;JPEG (*.jpg)");
	if(path.isEmpty()) return;
	QImage out=m_ctrl->preview();
	if(!out.save(path)){
		QMessageBox::warning(this,"错误","保存失败");
		return;
	}
	emit statusMessage("已导出:"+path);
	emit artifactReady(path,"image");
}

void FrameModuleWidget::onBatch()
{
	QStringList files=QFileDialog::getOpenFileNames(this,"选择多张图","","Image (*.png *.jpg *.jpeg)");
	if(files.isEmpty()) return;
	QString dir=QFileDialog::getExistingDirectory(this,"输出目录");
	if(dir.isEmpty()) return;

	m_cancel=false;
	m_btnExport->setEnabled(false);
	m_btnBatch->setEnabled(false);
	m_btnCancel->setEnabled(true);
	m_prog->setMaximum(files.size());
	m_prog->setValue(0);

	int okCnt=0;
	for(int i=0;i<files.size();i++){
		if(m_cancel) break;
		emit statusMessage(QString("[%1/%2] %3").arg(i+1).arg(files.size()).arg(files[i]));
		QImage img(files[i]);
		if(img.isNull()){
			m_prog->setValue(i+1);
			QApplication::processEvents();
			continue;
		}
		QImage out=m_ctrl->applyTo(img);
		QString name=QFileInfo(files[i]).completeBaseName()+"_out.png";
		if(out.save(dir+"/"+name)) okCnt++;
		m_prog->setValue(i+1);
		QApplication::processEvents();
	}

	m_btnExport->setEnabled(true);
	m_btnBatch->setEnabled(true);
	m_btnCancel->setEnabled(false);
	if(m_cancel) emit statusMessage("已取消");
	else emit statusMessage(QString("批量完成 %1/%2").arg(okCnt).arg(files.size()));
}

void FrameModuleWidget::onCancel()
{
	m_cancel=true;
}

void FrameModuleWidget::refreshPreview()
{
	if(!m_ctrl->hasImage()){
		m_view->setText("未加载图片");
		return;
	}
	QImage img=m_showOrig->isChecked()?m_ctrl->original():m_ctrl->preview();
	QSize sz=m_view->size();
	if(sz.width()<50) sz=QSize(400,300);
	m_view->setPixmap(QPixmap::fromImage(img).scaled(sz,Qt::KeepAspectRatio,Qt::SmoothTransformation));
}
