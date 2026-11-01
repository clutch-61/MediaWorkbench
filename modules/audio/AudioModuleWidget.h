#pragma once
#include "../../contracts/ToolModule.h"
#include "AudioController.h"

class WaveformWidget;
class AudioExportJob;
class QLabel;
class QPushButton;
class QSlider;
class QDoubleSpinBox;
class QRadioButton;
class QProgressBar;
class QMediaPlayer;
class QAudioOutput;
class QTimer;

class AudioModuleWidget:public ToolModule
{
	Q_OBJECT
public:
	explicit AudioModuleWidget(QWidget* parent=nullptr);

	QString moduleId() const override{ return "audio"; }
	void openFile(const QString& filePath) override;
	void setActive(bool active) override;

private slots:
	void onOpen();
	void onPlayAll();
	void onPlaySelection();
	void onPause();
	void onStop();
	void onExport();
	void onExtractTrack();
	void onCancelExport();
	void onTick();

private:
	void buildUi();
	void updateRangeLabel();
	void updateInfoLabel();
	void updateEnabledState();
	void setBusy(bool busy);
	void startJob(const QStringList& args,const QString& outPath,qint64 totalMs);
	QString suggestOutputPath(const QString& suffix) const;

	AudioController* m_ctrl=nullptr;
	AudioExportJob* m_job=nullptr;
	WaveformWidget* m_wave=nullptr;
	QMediaPlayer* m_player=nullptr;
	QAudioOutput* m_audio=nullptr;
	QTimer* m_tick=nullptr;

	QLabel* m_info=nullptr;
	QLabel* m_rangeInfo=nullptr;
	QLabel* m_statusLabel=nullptr;
	QLabel* m_gainInfo=nullptr;
	QSlider* m_vol=nullptr;
	QSlider* m_gain=nullptr;
	QDoubleSpinBox* m_fadeIn=nullptr;
	QDoubleSpinBox* m_fadeOut=nullptr;
	QRadioButton* m_stereo=nullptr;
	QRadioButton* m_mono=nullptr;
	QProgressBar* m_prog=nullptr;
	QPushButton* m_btnOpen=nullptr;
	QPushButton* m_btnExport=nullptr;
	QPushButton* m_btnExtract=nullptr;
	QPushButton* m_btnCancel=nullptr;
	QPushButton* m_btnMute=nullptr;
	QPushButton* m_btnGainReset=nullptr;
	QPushButton* m_btnPlayAll=nullptr;
	QPushButton* m_btnPlaySel=nullptr;
	QPushButton* m_btnPause=nullptr;
	QPushButton* m_btnStop=nullptr;

	bool m_busy=false;
	qint64 m_stopAtMs=-1;   // 选区试听到这个位置停，-1 表示不限
};
