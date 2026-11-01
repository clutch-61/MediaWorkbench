#include "WaveformAnalyzer.h"

#include "../../contracts/FFmpegHelper.h"

#include <QProcess>

#include <algorithm>
#include <cstring>

WaveformAnalyzer::WaveformAnalyzer(QObject* parent):QObject(parent)
{
}

WaveformAnalyzer::~WaveformAnalyzer()
{
	cancel();
}

bool WaveformAnalyzer::isRunning() const
{
	return m_proc&&m_proc->state()!=QProcess::NotRunning;
}

void WaveformAnalyzer::start(const QString& filePath)
{
	cancel();

	m_mins.clear();
	m_maxs.clear();
	m_carry.clear();
	m_inWindow=0;
	m_cancelRequested=false;

	if(m_proc){
		m_proc->deleteLater();
		m_proc=nullptr;
	}

	m_proc=new QProcess(this);
	m_proc->setProcessChannelMode(QProcess::SeparateChannels);

	connect(m_proc,&QProcess::readyReadStandardOutput,this,[this](){
		appendSamples(m_proc->readAllStandardOutput());
	});

	connect(m_proc,QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),this,
		[this](int exitCode,QProcess::ExitStatus status){
		if(m_cancelRequested) return;

		if(status==QProcess::CrashExit){
			emit failed(QStringLiteral("解码进程异常退出"));
			return;
		}
		if(exitCode!=0){
			const QString err=QString::fromLocal8Bit(m_proc->readAllStandardError()).trimmed();
			emit failed(err.isEmpty()?QStringLiteral("解码失败（文件可能没有音轨）"):err);
			return;
		}

		flushPartialWindow();
		if(m_mins.isEmpty()){
			emit failed(QStringLiteral("没有解出音频数据（文件可能没有音轨）"));
			return;
		}
		emit finished(m_mins,m_maxs);
	});

	const QStringList args{
		QStringLiteral("-nostdin"),
		QStringLiteral("-v"),QStringLiteral("error"),
		QStringLiteral("-i"),filePath,
		QStringLiteral("-map"),QStringLiteral("0:a:0"),
		QStringLiteral("-ac"),QStringLiteral("1"),
		QStringLiteral("-ar"),QString::number(sampleRate()),
		QStringLiteral("-f"),QStringLiteral("f32le"),
		QStringLiteral("pipe:1")
	};
	m_proc->start(FFmpegHelper::ffmpegPath(),args);
}

void WaveformAnalyzer::appendSamples(const QByteArray& data)
{
	m_carry.append(data);

	const int complete=m_carry.size()/4;
	for(int i=0;i<complete;++i){
		float v=0.0f;
		std::memcpy(&v,m_carry.constData()+i*4,sizeof(float));

		if(m_inWindow==0){
			m_curMin=v;
			m_curMax=v;
		}else{
			m_curMin=std::min(m_curMin,v);
			m_curMax=std::max(m_curMax,v);
		}

		if(++m_inWindow>=windowSamples()){
			m_mins.append(m_curMin);
			m_maxs.append(m_curMax);
			m_inWindow=0;
		}
	}

	// 不足一个 float 的残字节留到下次，避免跨块数据错位
	m_carry.remove(0,complete*4);
}

void WaveformAnalyzer::flushPartialWindow()
{
	if(m_inWindow>0){
		m_mins.append(m_curMin);
		m_maxs.append(m_curMax);
		m_inWindow=0;
	}
}

void WaveformAnalyzer::cancel()
{
	if(!m_proc) return;

	m_cancelRequested=true;
	if(m_proc->state()!=QProcess::NotRunning){
		m_proc->kill();
		m_proc->waitForFinished(1000);
	}
}
