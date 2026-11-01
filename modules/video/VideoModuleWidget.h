#pragma once
#include "contracts/ToolModule.h"

class VideoController;
struct VideoMediaInfo;
class SegmentSlider;
class QVideoWidget;
class QPushButton;
class QLabel;
class QComboBox;
class QSlider;
class QCheckBox;
class QProgressBar;
class QSplitter;

//视频工作台 布局在cpp里手写（和B/C一致）
class VideoModuleWidget:public ToolModule
{
	Q_OBJECT
public:
	explicit VideoModuleWidget(QWidget* parent=nullptr);
	~VideoModuleWidget() override;

	QString moduleId() const override{return QStringLiteral("video");}
	void openFile(const QString& filePath) override;
	void setActive(bool active) override;

private slots:
	void onOpenClicked();
	void onPlayPause();
	void onCaptureFrame();
	void onSetIn();
	void onSetOut();
	void onExportClicked();
	void onCancelExport();
	void onSpeedChanged(int index);
	void onExportSpeedChanged(int index);
	void onVolumeChanged(int value);
	void onMediaLoaded(const VideoMediaInfo& info);
	void onMediaLoadFailed(const QString& reason);
	void onMediaNeedsTranscode(const QString& reason);
	void onPlaybackTranscodeReady(const QString& path);
	void onPlaybackTranscodeFailed(const QString& reason);
	void onPositionChanged(qint64 ms);
	void onDurationChanged(qint64 ms);
	void onPlaybackStateChanged();
	void onExportProgress(int percent);
	void onExportFinished(bool ok,const QString& message,const QString& outPath);
	void onCaptureFinished(const QString& path);
	void onCaptureFailed(const QString& reason);
	void onSliderPressed();
	void onSliderReleased();

private:
	void buildUi();
	void wireSignals();
	void refreshSegmentState();
	void refreshExportUi();
	double exportSpeedValue() const;
	QString defaultFrameName() const;
	QString defaultExportName() const;

	VideoController* m_ctrl=nullptr;
	bool m_sliderDragging=false;

	QVideoWidget* m_video=nullptr;
	SegmentSlider* m_slider=nullptr;
	QPushButton* m_btnPlay=nullptr;
	QPushButton* m_btnCapture=nullptr;
	QPushButton* m_btnOpen=nullptr;
	QPushButton* m_btnSetIn=nullptr;
	QPushButton* m_btnSetOut=nullptr;
	QPushButton* m_btnExport=nullptr;
	QPushButton* m_btnCancel=nullptr;
	QLabel* m_lblCur=nullptr;
	QLabel* m_lblDur=nullptr;
	QLabel* m_lblFileName=nullptr;
	QLabel* m_lblResolution=nullptr;
	QLabel* m_lblDuration=nullptr;
	QLabel* m_lblIn=nullptr;
	QLabel* m_lblOut=nullptr;
	QLabel* m_lblHint=nullptr;
	QComboBox* m_cmbSpeed=nullptr;
	QComboBox* m_cmbExportSpeed=nullptr;
	QSlider* m_volSlider=nullptr;
	QCheckBox* m_chkMute=nullptr;
	QProgressBar* m_progress=nullptr;
};
