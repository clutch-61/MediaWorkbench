#include "VideoModuleWidget.h"
#include "VideoController.h"
#include "SegmentSlider.h"
#include "contracts/ModuleFactories.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QGroupBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QShortcut>
#include <QVideoWidget>
#include <QPushButton>
#include <QLabel>
#include <QSlider>
#include <QCheckBox>
#include <QProgressBar>

namespace{
const QList<double> kSpeeds={0.5,1.0,1.5,2.0};
}

VideoModuleWidget::VideoModuleWidget(QWidget* parent):ToolModule(parent)
{
	buildUi();
	m_ctrl=new VideoController(this);
	m_ctrl->setVideoWidget(m_video);
	wireSignals();
	refreshSegmentState();
	refreshExportUi();
	onPlaybackStateChanged();
}

VideoModuleWidget::~VideoModuleWidget()=default;

void VideoModuleWidget::buildUi()
{
	auto* root=new QHBoxLayout(this);
	auto* split=new QSplitter(Qt::Horizontal,this);

	auto* left=new QWidget;
	auto* lv=new QVBoxLayout(left);
	lv->setContentsMargins(0,0,0,0);
	m_video=new QVideoWidget;
	m_video->setMinimumSize(480,270);
	m_video->setAspectRatioMode(Qt::KeepAspectRatio);
	lv->addWidget(m_video,1);

	auto* bar=new QHBoxLayout;
	m_btnPlay=new QPushButton("播放");
	m_btnCapture=new QPushButton("截取当前帧");
	m_lblCur=new QLabel("00:00");
	m_lblCur->setMinimumWidth(56);
	m_lblCur->setAlignment(Qt::AlignCenter);
	m_slider=new SegmentSlider;
	m_slider->setOrientation(Qt::Horizontal);
	m_lblDur=new QLabel("00:00");
	m_lblDur->setMinimumWidth(56);
	m_lblDur->setAlignment(Qt::AlignCenter);
	m_cmbSpeed=new QComboBox;
	m_cmbSpeed->addItems({"0.5x","1.0x","1.5x","2.0x"});
	m_cmbSpeed->setCurrentIndex(1);
	m_volSlider=new QSlider(Qt::Horizontal);
	m_volSlider->setRange(0,100);
	m_volSlider->setValue(80);
	m_volSlider->setFixedWidth(96);
	m_volSlider->setToolTip("预览音量");
	bar->addWidget(m_btnPlay);
	bar->addWidget(m_btnCapture);
	bar->addWidget(m_lblCur);
	bar->addWidget(m_slider,1);
	bar->addWidget(m_lblDur);
	bar->addWidget(m_cmbSpeed);
	bar->addWidget(m_volSlider);
	lv->addLayout(bar);

	auto* right=new QWidget;
	right->setMinimumWidth(260);
	right->setMaximumWidth(360);
	auto* rv=new QVBoxLayout(right);
	rv->setContentsMargins(0,0,0,0);

	m_btnOpen=new QPushButton("打开视频…");
	m_lblFileName=new QLabel("未加载");
	m_lblFileName->setWordWrap(true);
	m_lblResolution=new QLabel("分辨率：—");
	m_lblDuration=new QLabel("时长：—");
	rv->addWidget(new QLabel("媒体信息"));
	rv->addWidget(m_btnOpen);
	rv->addWidget(m_lblFileName);
	rv->addWidget(m_lblResolution);
	rv->addWidget(m_lblDuration);

	auto* segBox=new QGroupBox("片段裁剪");
	auto* segLay=new QVBoxLayout(segBox);
	auto* segBtns=new QHBoxLayout;
	m_btnSetIn=new QPushButton("设为起点");
	m_btnSetOut=new QPushButton("设为终点");
	segBtns->addWidget(m_btnSetIn);
	segBtns->addWidget(m_btnSetOut);
	m_lblIn=new QLabel("起点：--:--");
	m_lblOut=new QLabel("终点：--:--");
	segLay->addLayout(segBtns);
	segLay->addWidget(m_lblIn);
	segLay->addWidget(m_lblOut);
	rv->addWidget(segBox);

	auto* expBox=new QGroupBox("导出设置");
	auto* expLay=new QVBoxLayout(expBox);
	expLay->addWidget(new QLabel("导出倍速"));
	m_cmbExportSpeed=new QComboBox;
	m_cmbExportSpeed->addItems({"1.0x（原速）","0.5x（慢速）","1.5x","2.0x"});
	m_chkMute=new QCheckBox("静音导出");
	expLay->addWidget(m_cmbExportSpeed);
	expLay->addWidget(m_chkMute);
	rv->addWidget(expBox);

	m_btnExport=new QPushButton("导出 MP4 视频");
	m_btnExport->setObjectName("primaryButton");
	m_progress=new QProgressBar;
	m_progress->setValue(0);
	m_btnCancel=new QPushButton("取消");
	m_lblHint=new QLabel;
	m_lblHint->setWordWrap(true);
	rv->addWidget(m_btnExport);
	rv->addWidget(m_progress);
	rv->addWidget(m_btnCancel);
	rv->addWidget(m_lblHint);
	rv->addStretch();

	split->addWidget(left);
	split->addWidget(right);
	split->setStretchFactor(0,7);
	split->setStretchFactor(1,3);
	split->setChildrenCollapsible(false);
	root->addWidget(split);
}

void VideoModuleWidget::wireSignals()
{
	connect(m_ctrl,&VideoController::mediaLoaded,this,&VideoModuleWidget::onMediaLoaded);
	connect(m_ctrl,&VideoController::mediaLoadFailed,this,&VideoModuleWidget::onMediaLoadFailed);
	connect(m_ctrl,&VideoController::mediaNeedsTranscode,this,&VideoModuleWidget::onMediaNeedsTranscode);
	connect(m_ctrl,&VideoController::playbackTranscodeReady,this,&VideoModuleWidget::onPlaybackTranscodeReady);
	connect(m_ctrl,&VideoController::playbackTranscodeFailed,this,&VideoModuleWidget::onPlaybackTranscodeFailed);
	connect(m_ctrl,&VideoController::positionChanged,this,&VideoModuleWidget::onPositionChanged);
	connect(m_ctrl,&VideoController::durationChanged,this,&VideoModuleWidget::onDurationChanged);
	connect(m_ctrl,&VideoController::playbackStateChanged,this,&VideoModuleWidget::onPlaybackStateChanged);
	connect(m_ctrl,&VideoController::exportProgress,this,&VideoModuleWidget::onExportProgress);
	connect(m_ctrl,&VideoController::exportFinished,this,&VideoModuleWidget::onExportFinished);
	connect(m_ctrl,&VideoController::captureFinished,this,&VideoModuleWidget::onCaptureFinished);
	connect(m_ctrl,&VideoController::captureFailed,this,&VideoModuleWidget::onCaptureFailed);
	connect(m_ctrl,&VideoController::statusMessage,this,&VideoModuleWidget::statusMessage);

	connect(m_btnOpen,&QPushButton::clicked,this,&VideoModuleWidget::onOpenClicked);
	connect(m_btnPlay,&QPushButton::clicked,this,&VideoModuleWidget::onPlayPause);
	connect(m_btnCapture,&QPushButton::clicked,this,&VideoModuleWidget::onCaptureFrame);
	connect(m_btnSetIn,&QPushButton::clicked,this,&VideoModuleWidget::onSetIn);
	connect(m_btnSetOut,&QPushButton::clicked,this,&VideoModuleWidget::onSetOut);
	connect(m_btnExport,&QPushButton::clicked,this,&VideoModuleWidget::onExportClicked);
	connect(m_btnCancel,&QPushButton::clicked,this,&VideoModuleWidget::onCancelExport);
	connect(m_cmbSpeed,QOverload<int>::of(&QComboBox::currentIndexChanged),this,&VideoModuleWidget::onSpeedChanged);
	connect(m_cmbExportSpeed,QOverload<int>::of(&QComboBox::currentIndexChanged),this,&VideoModuleWidget::onExportSpeedChanged);
	connect(m_volSlider,&QSlider::valueChanged,this,&VideoModuleWidget::onVolumeChanged);
	connect(m_slider,&QSlider::sliderPressed,this,&VideoModuleWidget::onSliderPressed);
	connect(m_slider,&QSlider::sliderReleased,this,&VideoModuleWidget::onSliderReleased);
	connect(m_slider,&QSlider::sliderMoved,this,[this](int value){
		m_lblCur->setText(VideoController::formatTime(value));
	});

	auto* shortcut=new QShortcut(QKeySequence(Qt::Key_Space),this);
	connect(shortcut,&QShortcut::activated,this,&VideoModuleWidget::onPlayPause);
}

void VideoModuleWidget::openFile(const QString& filePath){m_ctrl->openFile(filePath);}

void VideoModuleWidget::setActive(bool active)
{
	if(!active) m_ctrl->pause();
}

void VideoModuleWidget::onOpenClicked()
{
	QString path=QFileDialog::getOpenFileName(this,"打开视频文件","","视频文件 (*.mp4 *.mkv *.avi *.mov *.wmv *.flv *.ts);;所有文件 (*.*)");
	if(!path.isEmpty()) openFile(path);
}

void VideoModuleWidget::onPlayPause(){m_ctrl->playPause();}

void VideoModuleWidget::onCaptureFrame()
{
	if(!m_ctrl->mediaInfo().valid()){
		emit statusMessage("请先打开视频");
		return;
	}
	QString path=QFileDialog::getSaveFileName(this,"保存当前帧",defaultFrameName(),"PNG 图片 (*.png);;JPEG 图片 (*.jpg *.jpeg)");
	if(path.isEmpty()) return;
	m_ctrl->captureFrame(path);
}

void VideoModuleWidget::onSetIn()
{
	m_ctrl->setInPoint(m_ctrl->positionMs());
	refreshSegmentState();
}

void VideoModuleWidget::onSetOut()
{
	m_ctrl->setOutPoint(m_ctrl->positionMs());
	refreshSegmentState();
}

void VideoModuleWidget::onExportClicked()
{
	if(!m_ctrl->mediaInfo().valid()) return;
	qint64 start=m_ctrl->inPointMs();
	qint64 end=m_ctrl->outPointMs();
	if(end<=start) end=m_ctrl->durationMs();
	if(end<=start){
		QMessageBox::warning(this,"导出","无法确定导出区间，请先加载有效视频。");
		return;
	}
	QString outPath=QFileDialog::getSaveFileName(this,"导出视频",defaultExportName(),"MP4 视频 (*.mp4)");
	if(outPath.isEmpty()) return;
	if(outPath==m_ctrl->mediaInfo().filePath){
		QMessageBox::warning(this,"导出","输出文件不能覆盖源文件。");
		return;
	}
	VideoExportParams p;
	p.inputPath=m_ctrl->mediaInfo().filePath;
	p.outputPath=outPath;
	p.startMs=start;
	p.endMs=end;
	p.speed=exportSpeedValue();
	p.mute=m_chkMute->isChecked();
	p.hasAudio=m_ctrl->mediaInfo().hasAudio;
	m_ctrl->startExport(p);
}

void VideoModuleWidget::onCancelExport(){m_ctrl->cancelExport();}

void VideoModuleWidget::onSpeedChanged(int index)
{
	if(index>=0&&index<kSpeeds.size()) m_ctrl->setPlaybackRate(kSpeeds[index]);
}

void VideoModuleWidget::onExportSpeedChanged(int){refreshExportUi();}
void VideoModuleWidget::onVolumeChanged(int value){m_ctrl->setVolume(value);}

void VideoModuleWidget::onMediaLoaded(const VideoMediaInfo& info)
{
	m_lblFileName->setText(info.fileName);
	m_lblResolution->setText(QString("分辨率：%1×%2").arg(info.resolution.width()).arg(info.resolution.height()));
	m_lblDuration->setText(QString("时长：%1（%2 ms）").arg(VideoController::formatTime(info.durationMs)).arg(info.durationMs));
	refreshSegmentState();
	refreshExportUi();
	emit statusMessage("已加载："+info.fileName);
}

void VideoModuleWidget::onMediaLoadFailed(const QString& reason)
{
	QMessageBox::warning(this,"打开失败",reason);
	emit statusMessage(reason);
	refreshExportUi();
}

void VideoModuleWidget::onMediaNeedsTranscode(const QString& reason)
{
	auto answer=QMessageBox::question(this,"无法播放",
		reason+"\n\n是否用 FFmpeg 转码为 H.264 MP4 后再播放？\n（整片转码可能需要一些时间）",
		QMessageBox::Yes|QMessageBox::No,QMessageBox::Yes);
	if(answer==QMessageBox::Yes)
		m_ctrl->transcodeForPlayback(m_ctrl->mediaInfo().filePath);
}

void VideoModuleWidget::onPlaybackTranscodeReady(const QString& path)
{
	emit statusMessage("转码完成，正在打开："+QFileInfo(path).fileName());
	m_ctrl->openFile(path);
}

void VideoModuleWidget::onPlaybackTranscodeFailed(const QString& reason)
{
	m_progress->setValue(0);
	QMessageBox::warning(this,"转码失败",reason);
	emit statusMessage(reason);
	refreshExportUi();
}

void VideoModuleWidget::onPositionChanged(qint64 ms)
{
	if(!m_sliderDragging) m_slider->setValue(int(ms));
	m_lblCur->setText(VideoController::formatTime(ms));
}

void VideoModuleWidget::onDurationChanged(qint64 ms)
{
	m_slider->setRange(0,int(ms));
	m_lblDur->setText(VideoController::formatTime(ms));
}

void VideoModuleWidget::onPlaybackStateChanged()
{
	bool playing=m_ctrl->state()==VideoController::State::Playing;
	m_btnPlay->setText(playing?"暂停":"播放");
	refreshExportUi();
}

void VideoModuleWidget::onExportProgress(int percent)
{
	m_progress->setValue(percent);
	QString what=m_ctrl->isTranscoding()?"转码":"导出";
	emit statusMessage(QString("正在%1… %2%").arg(what).arg(percent));
}

void VideoModuleWidget::onExportFinished(bool ok,const QString& message,const QString& outPath)
{
	if(ok){
		m_progress->setValue(100);
		QMessageBox::information(this,"导出完成","已导出：\n"+outPath);
		emit artifactReady(outPath,"video");
	}else{
		m_progress->setValue(0);
		QMessageBox::warning(this,"导出失败",message);
	}
	refreshExportUi();
}

void VideoModuleWidget::onCaptureFinished(const QString& path)
{
	emit statusMessage("已保存当前帧："+path);
	emit artifactReady(path,"image");
}

void VideoModuleWidget::onCaptureFailed(const QString& reason)
{
	QMessageBox::warning(this,"截帧失败",reason);
	emit statusMessage(reason);
}

void VideoModuleWidget::onSliderPressed(){m_sliderDragging=true;}

void VideoModuleWidget::onSliderReleased()
{
	m_sliderDragging=false;
	m_ctrl->seek(m_slider->value());
}

void VideoModuleWidget::refreshSegmentState()
{
	m_lblIn->setText("起点："+VideoController::formatTime(m_ctrl->inPointMs()));
	m_lblOut->setText("终点："+VideoController::formatTime(m_ctrl->outPointMs()));
	if(m_ctrl->outPointMs()>m_ctrl->inPointMs())
		m_slider->setSegment(m_ctrl->inPointMs(),m_ctrl->outPointMs());
	else
		m_slider->clearSegment();
	refreshExportUi();
}

void VideoModuleWidget::refreshExportUi()
{
	bool loaded=m_ctrl->mediaInfo().valid();
	bool exporting=m_ctrl->state()==VideoController::State::Exporting;
	qint64 in=m_ctrl->inPointMs();
	qint64 out=m_ctrl->outPointMs();
	bool segInvalid=(in>0&&out>0&&out<=in);

	m_btnExport->setEnabled(loaded&&!exporting&&!segInvalid);
	m_btnCancel->setEnabled(exporting);
	m_btnSetIn->setEnabled(loaded&&!exporting);
	m_btnSetOut->setEnabled(loaded&&!exporting);
	m_cmbExportSpeed->setEnabled(!exporting);
	m_chkMute->setEnabled(!exporting);
	m_btnCapture->setEnabled(loaded);
	m_btnPlay->setEnabled(loaded);
	m_slider->setEnabled(loaded);

	if(segInvalid) m_lblHint->setText("⚠ 终点必须大于起点");
	else if(!loaded) m_lblHint->setText("请先打开视频文件");
	else m_lblHint->setText(QString());
}

double VideoModuleWidget::exportSpeedValue() const
{
	static const QList<double> speeds={1.0,0.5,1.5,2.0};
	int i=m_cmbExportSpeed->currentIndex();
	return (i>=0&&i<speeds.size())?speeds[i]:1.0;
}

QString VideoModuleWidget::defaultFrameName() const
{
	QString base=m_ctrl->mediaInfo().valid()?QFileInfo(m_ctrl->mediaInfo().fileName).completeBaseName():"frame";
	QString dir=m_ctrl->mediaInfo().valid()?QFileInfo(m_ctrl->mediaInfo().filePath).absolutePath():QString();
	QString name=base+"_帧_"+VideoController::formatTime(m_ctrl->positionMs()).replace(':','-')+".png";
	return dir.isEmpty()?name:dir+"/"+name;
}

QString VideoModuleWidget::defaultExportName() const
{
	QString base=m_ctrl->mediaInfo().valid()?QFileInfo(m_ctrl->mediaInfo().fileName).completeBaseName():"output";
	QString dir=m_ctrl->mediaInfo().valid()?QFileInfo(m_ctrl->mediaInfo().filePath).absolutePath():QString();
	QString speedTag=QString("x%1").arg(exportSpeedValue());
	QString name=base+"_片段_"+speedTag.replace('.','_')+".mp4";
	return dir.isEmpty()?name:dir+"/"+name;
}

ToolModule* createVideoModule(QWidget* parent)
{
	return new VideoModuleWidget(parent);
}
