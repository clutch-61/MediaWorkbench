#pragma once
#include <QByteArray>
#include <QObject>
#include <QString>
#include <QVector>

class QProcess;

// 用 ffmpeg 把音轨解码成单声道 PCM，按窗口算 min/max 包络
// 不把整段音频读进内存，一边解码一边算
class WaveformAnalyzer:public QObject
{
	Q_OBJECT
public:
	explicit WaveformAnalyzer(QObject* parent=nullptr);
	~WaveformAnalyzer() override;

	void start(const QString& filePath);
	void cancel();
	bool isRunning() const;

	// 8kHz 单声道，100 个采样一个窗口（12.5ms），约 80 个窗口/秒
	static int sampleRate(){ return 8000; }
	static int windowSamples(){ return 100; }

signals:
	void finished(const QVector<float>& mins,const QVector<float>& maxs);
	void failed(const QString& reason);

private:
	void appendSamples(const QByteArray& data);
	void flushPartialWindow();

	QProcess* m_proc=nullptr;
	QByteArray m_carry;
	QVector<float> m_mins;
	QVector<float> m_maxs;
	float m_curMin=0.0f;
	float m_curMax=0.0f;
	int m_inWindow=0;
	bool m_cancelRequested=false;
};
