#include "AudioModuleWidget.h"

#include "AudioExportJob.h"
#include "WaveformWidget.h"

#include <QAudioOutput>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QMediaPlayer>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QScrollArea>
#include <QSlider>
#include <QSplitter>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

namespace {

QString formatMs(qint64 ms)
{
	if(ms<0) ms=0;
	const qint64 totalSec=ms/1000;
	return QStringLiteral("%1:%2").arg(totalSec/60).arg(totalSec%60,2,10,QLatin1Char('0'));
}

} // namespace

AudioModuleWidget::AudioModuleWidget(QWidget* parent):ToolModule(parent)
{
	m_ctrl=new AudioController(this);
	m_job=new AudioExportJob(this);
	buildUi();
}

void AudioModuleWidget::buildUi()
{
	auto* root=new QHBoxLayout(this);
	auto* split=new QSplitter(Qt::Horizontal,this);

	// ---- 左：波形 + 播放控制 ----
	auto* left=new QWidget(split);
	auto* lv=new QVBoxLayout(left);

	m_wave=new WaveformWidget(left);
	lv->addWidget(m_wave,1);

	auto* bar=new QHBoxLayout;
	m_btnPlayAll=new QPushButton(QStringLiteral("播放全部"),left);
	m_btnPlaySel=new QPushButton(QStringLiteral("播放选区"),left);
	m_btnPause=new QPushButton(QStringLiteral("暂停"),left);
	m_btnStop=new QPushButton(QStringLiteral("停止"),left);
	m_vol=new QSlider(Qt::Horizontal,left);
	m_vol->setRange(0,100);
	m_vol->setValue(80);
	m_vol->setMaximumWidth(150);
	m_vol->setToolTip(QStringLiteral("试听音量（只影响扬声器，不影响导出）"));
	m_btnMute=new QPushButton(QStringLiteral("静音"),left);
	m_btnMute->setCheckable(true);
	m_btnMute->setMaximumWidth(64);

	bar->addWidget(m_btnPlayAll);
	bar->addWidget(m_btnPlaySel);
	bar->addWidget(m_btnPause);
	bar->addWidget(m_btnStop);
	bar->addSpacing(8);
	bar->addWidget(new QLabel(QStringLiteral("试听音量"),left));
	bar->addWidget(m_vol);
	bar->addWidget(m_btnMute);
	bar->addStretch();
	lv->addLayout(bar);

	m_rangeInfo=new QLabel(QStringLiteral("区间：-"),left);
	m_rangeInfo->setToolTip(QStringLiteral("在波形上拖动可以标记处理区间，拖两端可微调"));
	lv->addWidget(m_rangeInfo);

	m_statusLabel=new QLabel(left);
	m_statusLabel->setWordWrap(true);
	lv->addWidget(m_statusLabel);

	// ---- 右：参数 + 导出（放进滚动区，窗口再矮也不会把内容挤掉）----
	auto* rightScroll=new QScrollArea(split);
	rightScroll->setWidgetResizable(true);
	rightScroll->setFrameShape(QFrame::NoFrame);
	rightScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	rightScroll->setMinimumWidth(330);
	auto* right=new QWidget;
	auto* rv=new QVBoxLayout(right);
	rightScroll->setWidget(right);

	m_info=new QLabel(QStringLiteral("未加载音频"),right);
	m_info->setWordWrap(true);
	m_btnOpen=new QPushButton(QStringLiteral("打开音频 / 视频"),right);

	m_gain=new QSlider(Qt::Horizontal,right);
	m_gain->setRange(-12,12);
	m_gain->setValue(0);
	m_gainInfo=new QLabel(QStringLiteral("导出增益：0 dB"),right);

	m_fadeIn=new QDoubleSpinBox(right);
	m_fadeIn->setRange(0.0,60.0);
	m_fadeIn->setSingleStep(0.1);
	m_fadeIn->setSuffix(QStringLiteral(" s"));
	m_fadeOut=new QDoubleSpinBox(right);
	m_fadeOut->setRange(0.0,60.0);
	m_fadeOut->setSingleStep(0.1);
	m_fadeOut->setSuffix(QStringLiteral(" s"));

	m_stereo=new QRadioButton(QStringLiteral("保留原声道"),right);
	m_mono=new QRadioButton(QStringLiteral("转为单声道"),right);
	m_stereo->setChecked(true);

	m_btnExport=new QPushButton(QStringLiteral("导出 WAV 音频"),right);
	m_btnExport->setObjectName(QStringLiteral("primaryButton"));
	m_btnExport->setToolTip(QStringLiteral("保存时可另选 MP3"));
	m_btnExtract=new QPushButton(QStringLiteral("提取整条音轨"),right);
	m_btnExtract->setToolTip(QStringLiteral("从视频里抽出音频，不施加任何处理"));
	m_btnCancel=new QPushButton(QStringLiteral("取消导出"),right);
	m_btnCancel->setEnabled(false);

	m_prog=new QProgressBar(right);
	m_prog->setRange(0,100);
	m_prog->setValue(0);

	rv->addWidget(m_info);
	rv->addWidget(m_btnOpen);
	rv->addSpacing(6);
	auto* gainTitle=new QLabel(QStringLiteral("导出增益（-12 ~ +12 dB）"),right);
	gainTitle->setWordWrap(true);
	rv->addWidget(gainTitle);
	rv->addWidget(m_gain);
	{
		auto* gainRow=new QHBoxLayout;
		gainRow->addWidget(m_gainInfo);
		gainRow->addStretch();
		m_btnGainReset=new QPushButton(QStringLiteral("复位"),right);
		m_btnGainReset->setMaximumWidth(70);
		m_btnGainReset->setToolTip(QStringLiteral("增益归零 0 dB"));
		gainRow->addWidget(m_btnGainReset);
		rv->addLayout(gainRow);
	}
	rv->addSpacing(6);
	rv->addWidget(new QLabel(QStringLiteral("淡入"),right));
	rv->addWidget(m_fadeIn);
	rv->addWidget(new QLabel(QStringLiteral("淡出"),right));
	rv->addWidget(m_fadeOut);
	rv->addSpacing(6);
	rv->addWidget(new QLabel(QStringLiteral("声道"),right));
	rv->addWidget(m_stereo);
	rv->addWidget(m_mono);
	rv->addSpacing(6);
	rv->addWidget(m_btnExport);
	rv->addWidget(m_btnExtract);
	rv->addWidget(m_btnCancel);
	rv->addWidget(m_prog);
	rv->addStretch();

	split->addWidget(left);
	split->addWidget(rightScroll);
	split->setStretchFactor(0,7);
	split->setStretchFactor(1,3);
	split->setSizes(QList<int>()<<780<<340);
	root->addWidget(split);

	// ---- 播放器 ----
	m_player=new QMediaPlayer(this);
	m_audio=new QAudioOutput(this);
	m_player->setAudioOutput(m_audio);
	m_audio->setVolume(0.8);

	m_tick=new QTimer(this);
	m_tick->setInterval(50);
	connect(m_tick,&QTimer::timeout,this,&AudioModuleWidget::onTick);

	// ---- 信号接线 ----
	connect(m_btnOpen,&QPushButton::clicked,this,&AudioModuleWidget::onOpen);
	connect(m_btnPlayAll,&QPushButton::clicked,this,&AudioModuleWidget::onPlayAll);
	connect(m_btnPlaySel,&QPushButton::clicked,this,&AudioModuleWidget::onPlaySelection);
	connect(m_btnPause,&QPushButton::clicked,this,&AudioModuleWidget::onPause);
	connect(m_btnStop,&QPushButton::clicked,this,&AudioModuleWidget::onStop);
	connect(m_btnMute,&QPushButton::toggled,this,[this](bool muted){
		m_audio->setMuted(muted);
		m_btnMute->setText(muted?QStringLiteral("取消静音"):QStringLiteral("静音"));
	});
	connect(m_btnExport,&QPushButton::clicked,this,&AudioModuleWidget::onExport);
	connect(m_btnExtract,&QPushButton::clicked,this,&AudioModuleWidget::onExtractTrack);
	connect(m_btnCancel,&QPushButton::clicked,this,&AudioModuleWidget::onCancelExport);

	connect(m_vol,&QSlider::valueChanged,this,[this](int v){
		m_audio->setVolume(double(v)/100.0);
	});

	connect(m_gain,&QSlider::valueChanged,this,[this](int v){
		m_ctrl->setGainDb(double(v));
		m_gainInfo->setText(QStringLiteral("导出增益：%1 dB").arg(v));
	});

	connect(m_btnGainReset,&QPushButton::clicked,this,[this](){
		m_gain->setValue(0);
	});

	connect(m_fadeIn,QOverload<double>::of(&QDoubleSpinBox::valueChanged),this,[this](double){
		m_ctrl->setFade(m_fadeIn->value(),m_fadeOut->value());
	});
	connect(m_fadeOut,QOverload<double>::of(&QDoubleSpinBox::valueChanged),this,[this](double){
		m_ctrl->setFade(m_fadeIn->value(),m_fadeOut->value());
	});

	connect(m_mono,&QRadioButton::toggled,this,[this](bool on){
		if(on) m_ctrl->setMono(true);
	});
	connect(m_stereo,&QRadioButton::toggled,this,[this](bool on){
		if(on) m_ctrl->setMono(false);
	});

	connect(m_wave,&WaveformWidget::rangeChanged,this,[this](qint64 inMs,qint64 outMs){
		m_ctrl->setRange(inMs,outMs);
		m_wave->setRange(m_ctrl->inPoint(),m_ctrl->outPoint(),m_ctrl->durationMs());
		updateRangeLabel();
	});

	connect(m_wave,&WaveformWidget::seekRequested,this,[this](qint64 ms){
		m_player->setPosition(ms);
		m_wave->setPlayPos(ms);
	});

	connect(m_ctrl,&AudioController::loaded,this,[this](const QString&,qint64 dur){
		m_wave->setRange(0,dur,dur);
		updateInfoLabel();
		updateRangeLabel();
		m_statusLabel->setStyleSheet(QStringLiteral("color:#2d7d46;"));
		m_statusLabel->setText(QStringLiteral("已加载，正在分析波形…"));
		updateEnabledState();
	});
	connect(m_ctrl,&AudioController::waveReady,this,[this](){
		m_wave->setWave(m_ctrl->wave());
		m_statusLabel->setStyleSheet(QStringLiteral("color:#2d7d46;"));
		m_statusLabel->setText(QStringLiteral("波形分析完成，共 %1 个窗口").arg(m_ctrl->wave().size()));
		emit statusMessage(QStringLiteral("波形分析完成"));
	});
	connect(m_ctrl,&AudioController::error,this,[this](const QString& msg){
		m_statusLabel->setStyleSheet(QStringLiteral("color:#c0392b;"));
		m_statusLabel->setText(msg);
		emit statusMessage(msg);
	});

	connect(m_job,&ExportJob::progressChanged,this,[this](int percent){
		m_prog->setValue(percent);
	});
	connect(m_job,&ExportJob::finished,this,[this](bool ok,const QString& out,const QString& err){
		setBusy(false);
		if(ok){
			m_prog->setValue(100);
			m_statusLabel->setStyleSheet(QStringLiteral("color:#2d7d46;"));
			m_statusLabel->setText(QStringLiteral("导出完成：")+out);
			emit statusMessage(QStringLiteral("导出完成：")+out);
			emit artifactReady(out,QStringLiteral("audio"));
		}else{
			m_prog->setValue(0);
			m_statusLabel->setStyleSheet(QStringLiteral("color:#c0392b;"));
			m_statusLabel->setText(QStringLiteral("导出失败：")+err);
			emit statusMessage(QStringLiteral("导出失败：")+err);
		}
	});

	updateEnabledState();
}

void AudioModuleWidget::openFile(const QString& filePath)
{
	if(m_job->state()==JobState::Running){
		emit statusMessage(QStringLiteral("正在导出，请先取消再换文件"));
		return;
	}

	m_player->stop();
	m_tick->stop();
	m_stopAtMs=-1;
	m_prog->setValue(0);
	m_wave->setWave(QVector<WavePoint>());
	m_wave->setPlayPos(0);

	if(!m_ctrl->load(filePath))
		return;

	m_player->setSource(QUrl::fromLocalFile(filePath));

	// 换文件后参数回到默认值
	m_gain->setValue(0);
	m_fadeIn->setValue(0.0);
	m_fadeOut->setValue(0.0);
	m_stereo->setChecked(true);
	m_ctrl->setMono(false);

	emit statusMessage(QStringLiteral("已加载：")+QFileInfo(filePath).fileName());
}

void AudioModuleWidget::setActive(bool active)
{
	if(!active){
		m_player->pause();
		m_tick->stop();
		m_stopAtMs=-1;
	}
}

void AudioModuleWidget::updateInfoLabel()
{
	const AudioClipInfo& c=m_ctrl->clip();
	if(c.path.isEmpty()){
		m_info->setText(QStringLiteral("未加载音频"));
		return;
	}
	m_info->setText(QStringLiteral("%1\n时长 %2    采样率 %3 Hz    %4 声道    %5")
		.arg(QFileInfo(c.path).fileName())
		.arg(formatMs(c.durationMs))
		.arg(c.sampleRate)
		.arg(c.channels)
		.arg(c.codec));
}

void AudioModuleWidget::updateRangeLabel()
{
	if(m_ctrl->durationMs()<=0){
		m_rangeInfo->setText(QStringLiteral("区间：-"));
		return;
	}
	m_rangeInfo->setText(QStringLiteral("区间：%1 → %2    共 %3")
		.arg(formatMs(m_ctrl->inPoint()))
		.arg(formatMs(m_ctrl->outPoint()))
		.arg(formatMs(m_ctrl->segmentDurationMs())));
}

void AudioModuleWidget::setBusy(bool busy)
{
	m_busy=busy;
	updateEnabledState();
}

// 统一的状态管理：没载入文件时处理和导出全部置灰，导出进行中只亮取消
void AudioModuleWidget::updateEnabledState()
{
	const bool has=m_ctrl->clip().hasAudio&&m_ctrl->durationMs()>0;
	const bool idle=!m_busy;

	m_btnOpen->setEnabled(idle);
	m_btnExport->setEnabled(has&&idle);
	m_btnExtract->setEnabled(has&&idle);
	m_btnCancel->setEnabled(m_busy);

	m_btnPlayAll->setEnabled(has);
	m_btnPlaySel->setEnabled(has&&m_ctrl->segmentDurationMs()>0);
	m_btnPause->setEnabled(has);
	m_btnStop->setEnabled(has);
	m_btnMute->setEnabled(has);
	m_vol->setEnabled(has);

	m_gain->setEnabled(has&&idle);
	m_btnGainReset->setEnabled(has&&idle);
	m_fadeIn->setEnabled(has&&idle);
	m_fadeOut->setEnabled(has&&idle);
	m_stereo->setEnabled(has&&idle);
	m_mono->setEnabled(has&&idle);
}

QString AudioModuleWidget::suggestOutputPath(const QString& suffix) const
{
	const QFileInfo fi(m_ctrl->filePath());
	return QDir(fi.absolutePath()).filePath(fi.completeBaseName()+suffix);
}

void AudioModuleWidget::startJob(const QStringList& args,const QString& outPath,qint64 totalMs)
{
	m_job->setArgs(args,outPath,totalMs);
	m_prog->setValue(0);
	setBusy(true);
	m_job->start();
}

void AudioModuleWidget::onOpen()
{
	const QString path=QFileDialog::getOpenFileName(
		this,QStringLiteral("打开音频或视频"),QString(),
		QStringLiteral("媒体文件 (*.wav *.mp3 *.m4a *.aac *.flac *.ogg *.mp4 *.mkv *.mov);;所有文件 (*.*)"));
	if(!path.isEmpty())
		openFile(path);
}

void AudioModuleWidget::onPlayAll()
{
	if(m_ctrl->durationMs()<=0){
		emit statusMessage(QStringLiteral("先打开一个音频或视频文件"));
		return;
	}
	m_player->setPosition(0);
	m_player->play();
	m_stopAtMs=-1;
	m_tick->start();
}

void AudioModuleWidget::onPlaySelection()
{
	if(m_ctrl->durationMs()<=0){
		emit statusMessage(QStringLiteral("先打开一个音频或视频文件"));
		return;
	}
	const qint64 in=m_ctrl->inPoint();
	const qint64 out=m_ctrl->outPoint()>in?m_ctrl->outPoint():m_ctrl->durationMs();
	if(out<=in){
		emit statusMessage(QStringLiteral("选区无效"));
		return;
	}
	m_player->setPosition(in);
	m_player->play();
	m_stopAtMs=out;
	m_tick->start();
}

void AudioModuleWidget::onPause()
{
	m_player->pause();
	m_tick->stop();
	m_stopAtMs=-1;
}

void AudioModuleWidget::onStop()
{
	m_player->stop();
	m_tick->stop();
	m_stopAtMs=-1;
	m_wave->setPlayPos(0);
}

void AudioModuleWidget::onTick()
{
	const qint64 pos=m_player->position();
	m_wave->setPlayPos(pos);

	if(m_stopAtMs>=0&&pos>=m_stopAtMs){
		m_player->pause();
		m_stopAtMs=-1;
		m_tick->stop();
	}
}

void AudioModuleWidget::onExport()
{
	if(m_ctrl->durationMs()<=0){
		emit statusMessage(QStringLiteral("先打开一个音频或视频文件"));
		return;
	}
	if(m_job->state()==JobState::Running) return;

	QString path=QFileDialog::getSaveFileName(
		this,QStringLiteral("导出音频"),suggestOutputPath(QStringLiteral("_processed.wav")),
		QStringLiteral("WAV (*.wav);;MP3 (*.mp3)"));
	if(path.isEmpty()) return;

	if(!path.endsWith(QStringLiteral(".wav"),Qt::CaseInsensitive)
		&&!path.endsWith(QStringLiteral(".mp3"),Qt::CaseInsensitive))
		path+=QStringLiteral(".wav");

	const QString err=m_ctrl->validate(path);
	if(!err.isEmpty()){
		emit statusMessage(err);
		QMessageBox::warning(this,QStringLiteral("参数不合法"),err);
		return;
	}

	if(QFileInfo::exists(path)){
		const auto ret=QMessageBox::question(this,QStringLiteral("确认覆盖"),
			QStringLiteral("文件已存在，覆盖吗？\n")+path);
		if(ret!=QMessageBox::Yes) return;
	}

	startJob(m_ctrl->buildFfmpegArgs(path),path,m_ctrl->segmentDurationMs());
	emit statusMessage(QStringLiteral("开始导出…"));
}

void AudioModuleWidget::onExtractTrack()
{
	if(m_ctrl->durationMs()<=0){
		emit statusMessage(QStringLiteral("先打开一个音频或视频文件"));
		return;
	}
	if(m_job->state()==JobState::Running) return;

	QString path=QFileDialog::getSaveFileName(
		this,QStringLiteral("提取音轨"),suggestOutputPath(QStringLiteral("_track.wav")),
		QStringLiteral("WAV (*.wav);;MP3 (*.mp3)"));
	if(path.isEmpty()) return;

	if(!path.endsWith(QStringLiteral(".wav"),Qt::CaseInsensitive)
		&&!path.endsWith(QStringLiteral(".mp3"),Qt::CaseInsensitive))
		path+=QStringLiteral(".wav");

	if(QFileInfo(path).absoluteFilePath().compare(
		   QFileInfo(m_ctrl->filePath()).absoluteFilePath(),Qt::CaseInsensitive)==0){
		emit statusMessage(QStringLiteral("输出文件不能覆盖源文件"));
		return;
	}
	if(!QFileInfo(path).absoluteDir().exists()){
		emit statusMessage(QStringLiteral("输出目录不存在"));
		return;
	}
	if(QFileInfo::exists(path)){
		const auto ret=QMessageBox::question(this,QStringLiteral("确认覆盖"),
			QStringLiteral("文件已存在，覆盖吗？\n")+path);
		if(ret!=QMessageBox::Yes) return;
	}

	startJob(m_ctrl->buildExtractArgs(path),path,m_ctrl->durationMs());
	emit statusMessage(QStringLiteral("开始提取音轨…"));
}

void AudioModuleWidget::onCancelExport()
{
	m_job->cancel();
}
