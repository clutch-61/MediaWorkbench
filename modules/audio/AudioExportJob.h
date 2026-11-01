#pragma once
#include "../../contracts/ExportJob.h"

#include <QStringList>

class QProcess;

// 音频导出：QProcess 跑 ffmpeg，解析 -progress 的 out_time_us 出百分比
class AudioExportJob:public ExportJob
{
	Q_OBJECT
public:
	explicit AudioExportJob(QObject* parent=nullptr);
	~AudioExportJob() override;

	void setArgs(const QStringList& ffmpegArgs,const QString& outPath,qint64 totalMs);
	void start() override;
	void cancel() override;

	QString outputPath() const{ return m_out; }
	QString lastError() const{ return m_err; }

private:
	void handleStdout();
	void cleanupPartialOutput();

	QProcess* m_proc=nullptr;
	QStringList m_args;
	QString m_out;
	QString m_err;
	qint64 m_totalMs=0;
	bool m_cancelRequested=false;
};
