#include "VideoController.h"

#include <QAudioOutput>
#include <QDateTime>
#include <QFileInfo>
#include <QMediaMetaData>
#include <QMediaPlayer>
#include <QRegularExpression>
#include <QTimer>
#include <QUrl>
#include <QVideoWidget>

VideoController::VideoController(QObject* parent)
	: QObject(parent)
{
	m_exportJob = new VideoExportJob(this);
	connect(m_exportJob, &VideoExportJob::progress, this, &VideoController::onExportProgress);
	connect(m_exportJob, &VideoExportJob::finished, this, &VideoController::onExportFinished);

	m_loadCheckTimer = new QTimer(this);
	m_loadCheckTimer->setSingleShot(true);
	connect(m_loadCheckTimer, &QTimer::timeout, this, &VideoController::onLoadCheckTimeout);

	m_probeProcess = new QProcess(this);
	connect(m_probeProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
			this, &VideoController::onProbeFinished);

	m_captureProcess = new QProcess(this);
	connect(m_captureProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
			this, &VideoController::onCaptureProcessFinished);
}

VideoController::~VideoController() = default;

void VideoController::setVideoWidget(QVideoWidget* widget)
{
	m_videoWidget = widget;

	m_player = new QMediaPlayer(this);
	m_audio = new QAudioOutput(this);
	m_player->setAudioOutput(m_audio);
	m_player->setVideoOutput(widget);

	connect(m_player, &QMediaPlayer::playbackStateChanged, this, &VideoController::onPlayerStateChanged);
	connect(m_player, &QMediaPlayer::positionChanged, this, &VideoController::onPlayerPosition);
	connect(m_player, &QMediaPlayer::durationChanged, this, &VideoController::onPlayerDuration);
	connect(m_player, &QMediaPlayer::mediaStatusChanged, this, &VideoController::onPlayerMediaStatus);
	connect(m_player, &QMediaPlayer::errorOccurred, this, &VideoController::onPlayerError);

	m_audio->setVolume(0.8);
}

void VideoController::openFile(const QString& filePath)
{
	if (filePath.isEmpty() || !QFileInfo::exists(filePath)) {
		emit mediaLoadFailed(QStringLiteral("文件不存在或路径为空：%1").arg(filePath));
		return;
	}

	if (m_exportJob->isRunning())
		m_exportJob->cancel();

	m_loadCheckTimer->stop();
	resetSegment();
	m_media = VideoMediaInfo();
	m_media.filePath = filePath;
	m_media.fileName = QFileInfo(filePath).fileName();

	m_pendingPlay = true;
	setState(State::Loaded);
	emit statusMessage(QStringLiteral("正在加载：%1").arg(m_media.fileName));

	startCodecProbe(filePath);
}

void VideoController::play()
{
	if (!m_player || m_media.filePath.isEmpty())
		return;
	m_player->play();
}

void VideoController::pause()
{
	if (m_player)
		m_player->pause();
}

void VideoController::playPause()
{
	if (!m_player || m_media.filePath.isEmpty())
		return;
	if (m_player->playbackState() == QMediaPlayer::PlayingState)
		m_player->pause();
	else
		m_player->play();
}

void VideoController::seek(qint64 ms)
{
	if (!m_player)
		return;
	m_player->setPosition(ms);
	m_positionMs = ms;
	emit positionChanged(ms);
}

void VideoController::setPlaybackRate(qreal rate)
{
	m_rate = rate;
	if (m_player)
		m_player->setPlaybackRate(rate);
	emit playbackRateChanged(rate);
}

void VideoController::setVolume(int volume)
{
	if (m_audio)
		m_audio->setVolume(qBound(0, volume, 100) / 100.0);
}

void VideoController::setInPoint(qint64 ms)
{
	const qint64 max = m_media.durationMs > 0 ? m_media.durationMs : ms;
	m_inMs = qBound<qint64>(0, ms, max);
}

void VideoController::setOutPoint(qint64 ms)
{
	const qint64 max = m_media.durationMs > 0 ? m_media.durationMs : ms;
	m_outMs = qBound<qint64>(0, ms, max);
}

bool VideoController::hasValidSegment() const
{
	return m_media.durationMs > 0 && m_outMs > m_inMs;
}

void VideoController::resetSegment()
{
	m_inMs = 0;
	m_outMs = 0;
}

void VideoController::captureFrame(const QString& savePath)
{
	if (savePath.isEmpty())
		return;
	if (m_media.filePath.isEmpty() || !m_player) {
		emit captureFailed(QStringLiteral("请先打开视频再截取帧"));
		return;
	}
	if (m_captureProcess->state() != QProcess::NotRunning) {
		emit captureFailed(QStringLiteral("已有截帧任务进行中，请稍后再试"));
		return;
	}

	const QString ffmpeg = VideoExportJob::findFfmpeg();
	if (ffmpeg.isEmpty()) {
		emit captureFailed(QStringLiteral("未找到 FFmpeg，无法截取当前帧"));
		return;
	}

	const double sec = m_positionMs / 1000.0;
	m_capturePath = savePath;
	m_captureProcess->start(ffmpeg, QStringList()
		<< QStringLiteral("-ss") << QString::number(sec, 'f', 3)
		<< QStringLiteral("-i") << m_media.filePath
		<< QStringLiteral("-frames:v") << QStringLiteral("1")
		<< QStringLiteral("-q:v") << QStringLiteral("2")
		<< QStringLiteral("-y")
		<< savePath);
}

void VideoController::startExport(const VideoExportParams& params)
{
	if (params.inputPath.isEmpty() || params.outputPath.isEmpty()) {
		emit exportFinished(false, QStringLiteral("导出参数不完整"), QString());
		return;
	}
	if (params.outputPath == params.inputPath) {
		emit exportFinished(false, QStringLiteral("输出文件不能覆盖源文件"), QString());
		return;
	}
	if (m_exportJob->isRunning()) {
		emit exportFinished(false, QStringLiteral("已有导出任务进行中"), QString());
		return;
	}

	setState(State::Exporting);
	emit exportStateChanged();
	m_exportJob->start(params);
}

void VideoController::cancelExport()
{
	if (m_exportJob->isRunning())
		m_exportJob->cancel();
}

QString VideoController::formatTime(qint64 ms)
{
	if (ms < 0)
		ms = 0;
	const qint64 totalSec = ms / 1000;
	const qint64 h = totalSec / 3600;
	const qint64 m = (totalSec % 3600) / 60;
	const qint64 s = totalSec % 60;

	if (h > 0)
		return QStringLiteral("%1:%2:%3")
			.arg(h, 2, 10, QLatin1Char('0'))
			.arg(m, 2, 10, QLatin1Char('0'))
			.arg(s, 2, 10, QLatin1Char('0'));
	return QStringLiteral("%1:%2")
		.arg(m, 2, 10, QLatin1Char('0'))
		.arg(s, 2, 10, QLatin1Char('0'));
}

void VideoController::onPlayerStateChanged(QMediaPlayer::PlaybackState state)
{
	if (state == QMediaPlayer::PlayingState)
		setState(State::Playing);
	else if (state == QMediaPlayer::PausedState)
		setState(State::Paused);
	emit playbackStateChanged();
}

void VideoController::onPlayerPosition(qint64 ms)
{
	m_positionMs = ms;
	emit positionChanged(ms);
}

void VideoController::onPlayerDuration(qint64 ms)
{
	m_media.durationMs = ms;
	if (ms > 0)
		m_loadCheckTimer->stop();
	emit durationChanged(ms);
}

void VideoController::onPlayerMediaStatus(QMediaPlayer::MediaStatus status)
{
	switch (status) {
	case QMediaPlayer::LoadedMedia:
		updateMediaInfo();
		if (m_media.durationMs <= 0)
			m_loadCheckTimer->start(1000);
		if (m_pendingPlay) {
			m_pendingPlay = false;
			m_player->play();
		}
		break;
	case QMediaPlayer::InvalidMedia:
		emit mediaLoadFailed(QStringLiteral("无法解码该文件（格式不受支持或文件损坏）：%1")
							 .arg(m_media.fileName));
		setState(State::Empty);
		break;
	default:
		break;
	}
}

void VideoController::onPlayerError(QMediaPlayer::Error error, const QString& errorString)
{
	Q_UNUSED(error);
	emit mediaLoadFailed(QStringLiteral("播放出错：%1").arg(errorString));
}

void VideoController::onExportProgress(int percent)
{
	emit exportProgress(percent);
}

void VideoController::onExportFinished(bool ok, const QString& message, const QString& outPath)
{
	if (m_isTranscoding) {
		m_isTranscoding = false;
		if (m_state == State::Exporting)
			setState(State::Loaded);
		emit exportStateChanged();
		if (ok) {
			emit statusMessage(QStringLiteral("转码完成：%1").arg(QFileInfo(outPath).fileName()));
			emit playbackTranscodeReady(outPath);
		} else {
			emit statusMessage(QStringLiteral("转码失败：%1").arg(message));
			emit playbackTranscodeFailed(message);
		}
		return;
	}

	if (m_state == State::Exporting)
		setState(State::Loaded);
	emit exportStateChanged();
	emit exportFinished(ok, message, outPath);

	if (ok)
		emit statusMessage(QStringLiteral("导出完成：%1").arg(QFileInfo(outPath).fileName()));
	else
		emit statusMessage(QStringLiteral("导出失败：%1").arg(message));
}

void VideoController::onLoadCheckTimeout()
{
	if (m_media.valid() && m_media.durationMs <= 0 && m_player && m_player->duration() <= 0)
		emit mediaNeedsTranscode(
			QStringLiteral("视频「%1」无法解码，可能使用了 HEVC/AV1 等播放器不支持的编码。")
				.arg(m_media.fileName));
}

void VideoController::transcodeForPlayback(const QString& srcPath)
{
	if (srcPath.isEmpty() || !QFileInfo::exists(srcPath)) {
		emit playbackTranscodeFailed(QStringLiteral("转码源文件不存在"));
		return;
	}
	if (m_exportJob->isRunning()) {
		emit playbackTranscodeFailed(QStringLiteral("已有任务进行中，请稍后再试"));
		return;
	}

	const QFileInfo fi(srcPath);
	const QString outPath = fi.absolutePath() + QLatin1Char('/') + fi.completeBaseName()
		+ QStringLiteral("_h264兼容_")
		+ QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss"))
		+ QStringLiteral(".mp4");

	VideoExportParams p;
	p.inputPath = srcPath;
	p.outputPath = outPath;
	p.startMs = 0;
	p.endMs = 0;
	p.speed = 1.0;
	p.mute = false;
	p.hasAudio = true;

	m_isTranscoding = true;
	setState(State::Exporting);
	emit exportStateChanged();
	emit statusMessage(QStringLiteral("正在用 FFmpeg 转码为 H.264 MP4…"));
	m_exportJob->start(p);
}

void VideoController::startCodecProbe(const QString& path)
{
	const QString ffmpeg = VideoExportJob::findFfmpeg();
	if (ffmpeg.isEmpty()) {
		loadWithPlayer();
		return;
	}

	m_probePath = path;
	if (m_probeProcess->state() != QProcess::NotRunning) {
		m_probeProcess->kill();
		m_probeProcess->waitForFinished(500);
	}
	m_probeProcess->start(ffmpeg,
		QStringList() << QStringLiteral("-hide_banner") << QStringLiteral("-i") << path);
}

void VideoController::onProbeFinished(int exitCode, QProcess::ExitStatus status)
{
	Q_UNUSED(exitCode);
	Q_UNUSED(status);

	if (m_media.filePath.isEmpty() || m_media.filePath != m_probePath)
		return;

	const QString text = QString::fromUtf8(
		m_probeProcess->readAllStandardError() + m_probeProcess->readAllStandardOutput());
	const QString codec = extractVideoCodec(text);
	const QSize res = extractResolution(text);
	if (!res.isEmpty())
		m_media.resolution = res;
	if (text.contains(QStringLiteral("Audio:")))
		m_media.hasAudio = true;

	if (codec.isEmpty() || isQtFriendlyCodec(codec)) {
		loadWithPlayer();
	} else {
		emit mediaNeedsTranscode(
			QStringLiteral("视频编码为「%1」，Qt 播放器无法直接解码，需先转码为 H.264。").arg(codec));
	}
}

void VideoController::onCaptureProcessFinished(int exitCode, QProcess::ExitStatus status)
{
	if (m_capturePath.isEmpty())
		return;

	const QString path = m_capturePath;
	m_capturePath.clear();

	if (status == QProcess::NormalExit && exitCode == 0 && QFileInfo::exists(path)) {
		emit captureFinished(path);
	} else {
		QString err = QString::fromLocal8Bit(m_captureProcess->readAllStandardError()).trimmed();
		if (err.size() > 300)
			err = err.right(300);
		emit captureFailed(err.isEmpty() ? QStringLiteral("FFmpeg 截帧失败") : err);
	}
}

void VideoController::loadWithPlayer()
{
	if (!m_player || m_media.filePath.isEmpty())
		return;
	m_player->setSource(QUrl::fromLocalFile(m_media.filePath));
}

QString VideoController::extractVideoCodec(const QString& text)
{
	const QStringList lines = text.split(QLatin1Char('\n'));
	for (const QString& line : lines) {
		if (!line.contains(QStringLiteral("Video:")))
			continue;
		const int start = line.indexOf(QStringLiteral("Video:")) + 6;
		QString rest = line.mid(start).trimmed();
		const int end = rest.indexOf(QLatin1Char(' '));
		if (end > 0)
			rest = rest.left(end);
		return rest.toLower();
	}
	return QString();
}

QSize VideoController::extractResolution(const QString& text)
{
	// ffmpeg -i 常见输出: Stream #0:0: Video: h264 ..., 1920x1080, ...
	static const QRegularExpression re(QStringLiteral("(\\d{2,5})x(\\d{2,5})"));
	const QStringList lines = text.split(QLatin1Char('\n'));
	for (const QString& line : lines) {
		if (!line.contains(QStringLiteral("Video:")))
			continue;
		const QRegularExpressionMatch m = re.match(line);
		if (m.hasMatch())
			return QSize(m.captured(1).toInt(), m.captured(2).toInt());
	}
	return QSize();
}

bool VideoController::isQtFriendlyCodec(const QString& codec)
{
	return codec.contains(QStringLiteral("h264"))
		|| codec.contains(QStringLiteral("avc"));
}

void VideoController::setState(State s)
{
	if (m_state == s)
		return;
	m_state = s;
}

void VideoController::updateMediaInfo()
{
	if (!m_player)
		return;
	m_media.durationMs = m_player->duration();
	// hasAudio 优先用 ffmpeg 探测结果，这里不强制覆盖
	emit mediaLoaded(m_media);
	emit durationChanged(m_media.durationMs);
}
