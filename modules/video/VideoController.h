#pragma once

#include <QObject>
#include <QMediaPlayer>
#include <QProcess>
#include <QSize>

#include "VideoExportJob.h"

class QVideoWidget;
class QAudioOutput;
class QTimer;

// 模块内部共享的媒体信息。
struct VideoMediaInfo
{
	QString filePath;
	QString fileName;
	qint64  durationMs = 0;
	QSize   resolution;
	bool    hasAudio = false;

	bool valid() const { return !filePath.isEmpty(); }
};

// 协调播放、截帧与导出的状态与流程；不负责具体界面控件。
class VideoController : public QObject
{
	Q_OBJECT

public:
	enum class State { Empty, Loaded, Playing, Paused, Exporting };

	explicit VideoController(QObject* parent = nullptr);
	~VideoController() override;

	void setVideoWidget(QVideoWidget* widget);

	void openFile(const QString& filePath);

	void play();
	void pause();
	void playPause();
	void seek(qint64 ms);
	void setPlaybackRate(qreal rate);
	void setVolume(int volume);

	void setInPoint(qint64 ms);
	void setOutPoint(qint64 ms);
	bool hasValidSegment() const;
	void resetSegment();

	void captureFrame(const QString& savePath);

	void startExport(const VideoExportParams& params);
	void cancelExport();

	void transcodeForPlayback(const QString& srcPath);
	bool isTranscoding() const { return m_isTranscoding; }

	State state() const { return m_state; }
	const VideoMediaInfo& mediaInfo() const { return m_media; }
	qint64 positionMs() const { return m_positionMs; }
	qint64 durationMs() const { return m_media.durationMs; }
	qreal  playbackRate() const { return m_rate; }
	qint64 inPointMs() const { return m_inMs; }
	qint64 outPointMs() const { return m_outMs; }

	static QString formatTime(qint64 ms);
	static QString extractVideoCodec(const QString& text);
	static bool isQtFriendlyCodec(const QString& codec);
	static QSize extractResolution(const QString& text);

signals:
	void mediaLoaded(const VideoMediaInfo& info);
	void mediaLoadFailed(const QString& reason);
	void positionChanged(qint64 ms);
	void durationChanged(qint64 ms);
	void playbackStateChanged();
	void playbackRateChanged(qreal rate);
	void statusMessage(const QString& message);
	void captureFinished(const QString& path);
	void captureFailed(const QString& reason);
	void exportProgress(int percent);
	void exportStateChanged();
	void exportFinished(bool ok, const QString& message, const QString& outPath);

	void mediaNeedsTranscode(const QString& reason);
	void playbackTranscodeReady(const QString& path);
	void playbackTranscodeFailed(const QString& reason);

private slots:
	void onPlayerStateChanged(QMediaPlayer::PlaybackState state);
	void onPlayerPosition(qint64 ms);
	void onPlayerDuration(qint64 ms);
	void onPlayerMediaStatus(QMediaPlayer::MediaStatus status);
	void onPlayerError(QMediaPlayer::Error error, const QString& errorString);
	void onExportProgress(int percent);
	void onExportFinished(bool ok, const QString& message, const QString& outPath);
	void onLoadCheckTimeout();
	void onProbeFinished(int exitCode, QProcess::ExitStatus status);
	void onCaptureProcessFinished(int exitCode, QProcess::ExitStatus status);

private:
	void setState(State s);
	void updateMediaInfo();
	void startCodecProbe(const QString& path);
	void loadWithPlayer();

	QMediaPlayer*  m_player = nullptr;
	QAudioOutput*  m_audio = nullptr;
	QVideoWidget*  m_videoWidget = nullptr;
	VideoExportJob* m_exportJob = nullptr;

	State m_state = State::Empty;
	VideoMediaInfo m_media;
	qint64 m_positionMs = 0;
	qreal  m_rate = 1.0;
	qint64 m_inMs = 0;
	qint64 m_outMs = 0;
	bool   m_pendingPlay = false;
	QString m_capturePath;

	QTimer* m_loadCheckTimer = nullptr;
	bool    m_isTranscoding = false;
	QProcess* m_probeProcess = nullptr;
	QString  m_probePath;
	QProcess* m_captureProcess = nullptr;
};
