#include "FFmpegHelper.h"
#include <QProcess>
#include <QStandardPaths>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QCoreApplication>

static QString findTool(const QString& name)
{
	QString env=qEnvironmentVariable("FFMPEG_PATH");
	if(!env.isEmpty()){
		QFileInfo fi(env);
		if(fi.isDir()){
			QString p=fi.absoluteFilePath()+"/"+name+".exe";
			if(QFileInfo::exists(p)) return p;
		}else if(fi.exists()) return fi.absoluteFilePath();
	}
	QString cfg=QCoreApplication::applicationDirPath()+"/ffmpeg_path.txt";
	QFile f(cfg);
	if(f.open(QIODevice::ReadOnly|QIODevice::Text)){
		QString line=QString::fromUtf8(f.readAll()).trimmed();
		if(!line.isEmpty()&&QFileInfo::exists(line)){
			if(name=="ffmpeg") return line;
			QString side=QFileInfo(line).absolutePath()+"/"+name+".exe";
			if(QFileInfo::exists(side)) return side;
		}
	}
	QString inApp=QCoreApplication::applicationDirPath()+"/"+name+".exe";
	if(QFileInfo::exists(inApp)) return inApp;
	QString wh=QStandardPaths::findExecutable(name);
	if(!wh.isEmpty()) return wh;
	if(name=="ffmpeg"){
		QString bili=QDir::homePath()+"/AppData/Roaming/bilibili/ffmpeg/ffmpeg.exe";
		if(QFileInfo::exists(bili)) return bili;
	}
	return QString();
}

QString FFmpegHelper::ffmpegPath(){return findTool("ffmpeg");}
QString FFmpegHelper::ffprobePath(){return findTool("ffprobe");}

bool FFmpegHelper::available()
{
	QString p=ffmpegPath();
	if(p.isEmpty()) return false;
	QProcess proc;
	proc.start(p,QStringList()<<"-version");
	if(!proc.waitForFinished(3000)) return false;
	return proc.exitCode()==0;
}

bool FFmpegHelper::extractFrame(const QString& video,double sec,const QString& outPath)
{
	QString ff=ffmpegPath();
	if(ff.isEmpty()||video.isEmpty()||outPath.isEmpty()) return false;
	QProcess p;
	p.start(ff,QStringList()
		<<"-y"<<"-ss"<<QString::number(sec,'f',3)
		<<"-i"<<video
		<<"-frames:v"<<"1"<<"-q:v"<<"2"
		<<outPath);
	if(!p.waitForFinished(20000)||p.exitCode()!=0) return false;
	return QFileInfo::exists(outPath);
}
