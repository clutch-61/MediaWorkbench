#include "AudioExportJob.h"

#include "../../contracts/FFmpegHelper.h"

#include <QFile>
#include <QFileInfo>
#include <QProcess>

AudioExportJob::AudioExportJob(QObject* parent):ExportJob(parent)
{
}

AudioExportJob::~AudioExportJob()
{
	if(m_proc&&m_proc->state()!=QProcess::NotRunning){
		m_proc->kill();
		m_proc->waitForFinished(1000);
	}
}

void AudioExportJob::setArgs(const QStringList& ffmpegArgs,const QString& outPath,qint64 totalMs)
{
	m_args=ffmpegArgs;
	m_out=outPath;
	m_totalMs=totalMs;
	m_err.clear();
}

void AudioExportJob::start()
{
	if(m_args.isEmpty()||m_out.isEmpty()){
		m_err=QStringLiteral("导出参数为空");
		setState(JobState::Failed);
		emit finished(false,QString(),m_err);
		return;
	}

	if(m_proc){
		m_proc->deleteLater();
		m_proc=nullptr;
	}

	m_cancelRequested=false;
	m_err.clear();

	m_proc=new QProcess(this);
	m_proc->setProcessChannelMode(QProcess::SeparateChannels);

	connect(m_proc,&QProcess::readyReadStandardOutput,this,&AudioExportJob::handleStdout);

	connect(m_proc,QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),this,
		[this](int exitCode,QProcess::ExitStatus status){
		if(m_cancelRequested){
			cleanupPartialOutput();
			setState(JobState::Cancelled);
			emit finished(false,QString(),QStringLiteral("已取消"));
			return;
		}

		if(status==QProcess::CrashExit||exitCode!=0){
			m_err=QString::fromLocal8Bit(m_proc->readAllStandardError()).trimmed();
			if(m_err.isEmpty()) m_err=QStringLiteral("导出进程异常退出");
			cleanupPartialOutput();
			setState(JobState::Failed);
			emit finished(false,QString(),m_err);
			return;
		}

		if(!QFileInfo::exists(m_out)||QFileInfo(m_out).size()<=0){
			m_err=QStringLiteral("没有生成有效的输出文件");
			setState(JobState::Failed);
			emit finished(false,QString(),m_err);
			return;
		}

		emit progressChanged(100);
		setState(JobState::Succeeded);
		emit finished(true,m_out,QString());
	});

	setState(JobState::Running);
	emit progressChanged(0);
	m_proc->start(FFmpegHelper::ffmpegPath(),m_args);

	if(!m_proc->waitForStarted(3000)){
		m_err=QStringLiteral("ffmpeg 启动失败，请确认它在 PATH 里");
		setState(JobState::Failed);
		emit finished(false,QString(),m_err);
	}
}

void AudioExportJob::handleStdout()
{
	const QByteArray data=m_proc->readAllStandardOutput();
	const QList<QByteArray> lines=data.split('\n');
	const QByteArray key("out_time_us=");

	for(const QByteArray& raw:lines){
		const QByteArray line=raw.trimmed();
		if(!line.startsWith(key)) continue;

		bool ok=false;
		const qint64 us=line.mid(key.size()).toLongLong(&ok);
		if(!ok||m_totalMs<=0) continue;

		double pct=100.0*double(us)/(double(m_totalMs)*1000.0);
		int value=int(pct);
		if(value<0) value=0;
		if(value>100) value=100;
		emit progressChanged(value);
	}
}

void AudioExportJob::cancel()
{
	if(state()!=JobState::Running||!m_proc) return;
	m_cancelRequested=true;
	m_proc->kill();       // 文件清理放在 finished 里，进程退了句柄才释放
}

void AudioExportJob::cleanupPartialOutput()
{
	if(!m_out.isEmpty()&&QFile::exists(m_out))
		QFile::remove(m_out);
}
