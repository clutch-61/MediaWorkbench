#include "AudioController.h"
#include "WaveformAnalyzer.h"

#include "../../contracts/FFmpegHelper.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>

#include <cmath>

qint64 AudioExportParams::segmentDurationMs() const
{
	const qint64 end=(outMs>inMs)?outMs:durationMs;
	const qint64 d=end-inMs;
	return d>0?d:0;
}

AudioController::AudioController(QObject* parent)
	:QObject(parent)
	,m_analyzer(new WaveformAnalyzer(this))
{
	connect(m_analyzer,&WaveformAnalyzer::finished,this,
		[this](const QVector<float>& mins,const QVector<float>& maxs){
		m_wave.clear();
		m_wave.reserve(mins.size());
		for(int i=0;i<mins.size();++i)
			m_wave.append(WavePoint{mins.at(i),maxs.at(i)});
		emit waveReady();
	});

	connect(m_analyzer,&WaveformAnalyzer::failed,this,[this](const QString& reason){
		emit error(QStringLiteral("波形分析失败：")+reason);
	});
}

bool AudioController::load(const QString& path)
{
	if(path.isEmpty()||!QFileInfo::exists(path)){
		emit error(QStringLiteral("文件不存在"));
		return false;
	}
	if(!probe(path)) return false;

	m_wave.clear();
	m_inMs=0;
	m_outMs=m_clip.durationMs;
	m_gainDb=0.0;
	m_fadeIn=0.0;
	m_fadeOut=0.0;
	m_mono=false;

	emit loaded(m_clip.path,m_clip.durationMs);
	m_analyzer->start(path);
	return true;
}

void AudioController::cancelAnalysis()
{
	m_analyzer->cancel();
}

bool AudioController::probe(const QString& path)
{
	m_clip=AudioClipInfo();
	m_clip.path=path;

	QProcess p;
	p.start(FFmpegHelper::ffprobePath(),QStringList()
		<<QStringLiteral("-v")<<QStringLiteral("error")
		<<QStringLiteral("-print_format")<<QStringLiteral("json")
		<<QStringLiteral("-show_format")<<QStringLiteral("-show_streams")
		<<path);

	if(!p.waitForFinished(8000)){
		p.kill();
		emit error(QStringLiteral("ffprobe 启动失败或超时，请确认 FFmpeg 在 PATH 里"));
		return false;
	}
	if(p.exitCode()!=0){
		const QString err=QString::fromLocal8Bit(p.readAllStandardError()).trimmed();
		emit error(err.isEmpty()?QStringLiteral("无法读取该文件"):err);
		return false;
	}

	const QJsonDocument doc=QJsonDocument::fromJson(p.readAllStandardOutput());
	if(!doc.isObject()){
		emit error(QStringLiteral("ffprobe 输出无法解析"));
		return false;
	}

	const QJsonObject root=doc.object();
	const QJsonArray streams=root.value(QStringLiteral("streams")).toArray();

	double streamDurSec=0.0;
	bool foundAudio=false;
	for(int i=0;i<streams.size();++i){
		const QJsonObject s=streams.at(i).toObject();
		if(s.value(QStringLiteral("codec_type")).toString()!=QStringLiteral("audio"))
			continue;
		m_clip.hasAudio=true;
		m_clip.sampleRate=s.value(QStringLiteral("sample_rate")).toString().toInt();
		m_clip.channels=s.value(QStringLiteral("channels")).toInt();
		m_clip.codec=s.value(QStringLiteral("codec_name")).toString();
		streamDurSec=s.value(QStringLiteral("duration")).toString().toDouble();
		foundAudio=true;
		break;
	}
	if(!foundAudio){
		emit error(QStringLiteral("这个文件里没有音轨"));
		return false;
	}

	double durSec=root.value(QStringLiteral("format")).toObject()
					.value(QStringLiteral("duration")).toString().toDouble();
	if(durSec<=0.0) durSec=streamDurSec;
	if(durSec<=0.0){
		emit error(QStringLiteral("读不到文件时长"));
		return false;
	}

	m_clip.durationMs=qint64(durSec*1000.0+0.5);
	return true;
}

void AudioController::setRange(qint64 inMs,qint64 outMs)
{
	m_inMs=qMax<qint64>(0,inMs);
	m_outMs=qMin(outMs,m_clip.durationMs);
	if(m_outMs<=m_inMs) m_outMs=m_clip.durationMs;
}

void AudioController::setGainDb(double db){ m_gainDb=qBound(-12.0,db,12.0); }
void AudioController::setFade(double inSec,double outSec){ m_fadeIn=qMax(0.0,inSec); m_fadeOut=qMax(0.0,outSec); }
void AudioController::setMono(bool mono){ m_mono=mono; }

qint64 AudioController::segmentDurationMs() const
{
	return exportParams().segmentDurationMs();
}

AudioExportParams AudioController::exportParams() const
{
	AudioExportParams p;
	p.srcPath=m_clip.path;
	p.durationMs=m_clip.durationMs;
	p.inMs=m_inMs;
	p.outMs=m_outMs;
	p.gainDb=m_gainDb;
	p.fadeInSec=m_fadeIn;
	p.fadeOutSec=m_fadeOut;
	p.mono=m_mono;
	return p;
}

QString AudioController::validate(const QString& outPath) const
{
	return validateParams(exportParams(),outPath);
}

QString AudioController::validateParams(const AudioExportParams& p,const QString& outPath)
{
	if(p.srcPath.isEmpty()) return QStringLiteral("还没有加载音频");
	if(outPath.isEmpty()) return QStringLiteral("输出路径为空");

	const QFileInfo in(p.srcPath);
	const QFileInfo out(outPath);
	if(out.absoluteFilePath().compare(in.absoluteFilePath(),Qt::CaseInsensitive)==0)
		return QStringLiteral("输出文件不能覆盖源文件");
	if(!out.absoluteDir().exists())
		return QStringLiteral("输出目录不存在");

	if(p.durationMs<=0) return QStringLiteral("文件时长未知，无法导出");

	const qint64 seg=p.segmentDurationMs();
	if(seg<=0) return QStringLiteral("片段长度为 0");
	if(p.inMs<0||p.inMs>=p.durationMs) return QStringLiteral("起点超出音频范围");
	if(p.outMs>p.durationMs) return QStringLiteral("终点超出音频范围");

	if(p.gainDb<-12.0||p.gainDb>12.0)
		return QStringLiteral("增益必须在 -12 ~ +12 dB 之间");
	if(p.fadeInSec<0.0||p.fadeOutSec<0.0)
		return QStringLiteral("淡入/淡出不能为负数");
	if((p.fadeInSec+p.fadeOutSec)*1000.0>double(seg))
		return QStringLiteral("淡入淡出总长超过片段长度");

	return QString();
}

QStringList AudioController::buildFfmpegArgs(const QString& outPath) const
{
	return buildFfmpegArgs(exportParams(),outPath);
}

QStringList AudioController::buildExtractArgs(const QString& outPath) const
{
	AudioExportParams p=exportParams();
	p.inMs=0;
	p.outMs=p.durationMs;
	p.gainDb=0.0;
	p.fadeInSec=0.0;
	p.fadeOutSec=0.0;
	p.mono=false;
	return buildFfmpegArgs(p,outPath);
}

QStringList AudioController::buildFfmpegArgs(const AudioExportParams& p,const QString& outPath)
{
	const qint64 segMs=p.segmentDurationMs();
	const double startSec=double(p.inMs)/1000.0;
	const double segSec=double(segMs)/1000.0;

	QStringList a;
	a<<QStringLiteral("-nostdin")<<QStringLiteral("-y")
	 <<QStringLiteral("-v")<<QStringLiteral("error")
	 <<QStringLiteral("-nostats")
	 <<QStringLiteral("-progress")<<QStringLiteral("pipe:1");

	// -ss/-t 放在 -i 前面：输入定位，输出时间戳从 0 开始，淡入淡出才按 0 算
	a<<QStringLiteral("-ss")<<QString::number(startSec,'f',3);
	a<<QStringLiteral("-t")<<QString::number(segSec,'f',3);
	a<<QStringLiteral("-i")<<p.srcPath;
	a<<QStringLiteral("-map")<<QStringLiteral("0:a:0");

	const QString filter=buildFilter(p.gainDb,p.fadeInSec,p.fadeOutSec,segSec);
	if(!filter.isEmpty())
		a<<QStringLiteral("-af")<<filter;

	if(p.mono)
		a<<QStringLiteral("-ac")<<QStringLiteral("1");

	if(outPath.endsWith(QStringLiteral(".mp3"),Qt::CaseInsensitive))
		a<<QStringLiteral("-c:a")<<QStringLiteral("libmp3lame")<<QStringLiteral("-q:a")<<QStringLiteral("4");
	else
		a<<QStringLiteral("-c:a")<<QStringLiteral("pcm_s16le");

	a<<outPath;
	return a;
}

double AudioController::dbToLinear(double db)
{
	return std::pow(10.0,db/20.0);
}

QString AudioController::buildFilter(double gainDb,double fadeInSec,double fadeOutSec,double segSec)
{
	QStringList parts;

	if(std::fabs(gainDb)>0.001)
		parts<<QStringLiteral("volume=%1").arg(dbToLinear(gainDb),0,'f',4);

	if(fadeInSec>0.001)
		parts<<QStringLiteral("afade=t=in:st=0:d=%1").arg(fadeInSec,0,'f',3);

	if(fadeOutSec>0.001){
		double st=segSec-fadeOutSec;
		if(st<0.0) st=0.0;
		parts<<QStringLiteral("afade=t=out:st=%1:d=%2")
				.arg(st,0,'f',3)
				.arg(fadeOutSec,0,'f',3);
	}

	return parts.join(QLatin1Char(','));
}
