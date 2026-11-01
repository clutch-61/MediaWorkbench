#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

class WaveformAnalyzer;

// 波形里一个窗口的最小/最大振幅
struct WavePoint{float mn;float mx;};

// ffprobe 探测出来的文件信息
struct AudioClipInfo{
	QString path;
	bool hasAudio=false;
	qint64 durationMs=0;
	int sampleRate=0;
	int channels=0;
	QString codec;
};

// 导出参数：界面/校验/生成 ffmpeg 参数都用这一份
struct AudioExportParams{
	QString srcPath;
	qint64 durationMs=0;
	qint64 inMs=0;
	qint64 outMs=0;         // <= inMs 表示到结尾
	double gainDb=0.0;      // -12 ~ +12
	double fadeInSec=0.0;
	double fadeOutSec=0.0;
	bool mono=false;

	qint64 segmentDurationMs() const;
};

// 音频逻辑：探测/波形/区间/增益/淡入淡出/声道/导出参数
class AudioController:public QObject
{
	Q_OBJECT
public:
	explicit AudioController(QObject* parent=nullptr);

	bool load(const QString& path);
	void cancelAnalysis();

	void setRange(qint64 inMs,qint64 outMs);
	void setGainDb(double db);
	void setFade(double inSec,double outSec);
	void setMono(bool mono);

	const AudioClipInfo& clip() const{ return m_clip; }
	QString filePath() const{ return m_clip.path; }
	qint64 durationMs() const{ return m_clip.durationMs; }
	qint64 inPoint() const{ return m_inMs; }
	qint64 outPoint() const{ return m_outMs; }
	qint64 segmentDurationMs() const;
	double gainDb() const{ return m_gainDb; }
	double fadeInSec() const{ return m_fadeIn; }
	double fadeOutSec() const{ return m_fadeOut; }
	bool mono() const{ return m_mono; }
	const QVector<WavePoint>& wave() const{ return m_wave; }

	AudioExportParams exportParams() const;

	// 校验：返回空串表示合法，否则是给人看的错误信息
	QString validate(const QString& outPath) const;
	static QString validateParams(const AudioExportParams& p,const QString& outPath);

	// 生成 ffmpeg 参数（区间/增益/淡入淡出/声道/编码）
	QStringList buildFfmpegArgs(const QString& outPath) const;
	// 从视频提取整条音轨（不施加任何处理）
	QStringList buildExtractArgs(const QString& outPath) const;
	static QStringList buildFfmpegArgs(const AudioExportParams& p,const QString& outPath);

	// 纯函数，方便单测
	static double dbToLinear(double db);
	static QString buildFilter(double gainDb,double fadeInSec,double fadeOutSec,double segSec);

signals:
	void loaded(const QString& path,qint64 durationMs);
	void waveReady();
	void error(const QString& msg);

private:
	bool probe(const QString& path);

	AudioClipInfo m_clip;
	QVector<WavePoint> m_wave;
	qint64 m_inMs=0;
	qint64 m_outMs=0;
	double m_gainDb=0.0;
	double m_fadeIn=0.0;
	double m_fadeOut=0.0;
	bool m_mono=false;
	WaveformAnalyzer* m_analyzer=nullptr;
};
