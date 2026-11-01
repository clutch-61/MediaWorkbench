#include "ImageExportJob.h"
#include <QFileInfo>
#include <QDir>

ImageExportJob::ImageExportJob(QObject* parent):ExportJob(parent){}

void ImageExportJob::setSingle(const QImage& img,const QString& outPath)
{
	m_img=img;m_out=outPath;m_batch=false;
}

void ImageExportJob::setBatch(const QStringList& files,const QString& outDir)
{
	m_files=files;m_outDir=outDir;m_batch=true;
}

void ImageExportJob::start()
{
	setState(JobState::Running);
	m_cancel=false;
	if(!m_batch){
		if(m_img.isNull()||m_out.isEmpty()||!m_img.save(m_out)){
			setState(JobState::Failed);
			emit finished(false,QString(),"保存失败");
			return;
		}
		emit progressChanged(100);
		setState(JobState::Succeeded);
		emit finished(true,m_out,QString());
		return;
	}
	//批量留给界面里跑 这里先不写
	setState(JobState::Failed);
	emit finished(false,QString(),"用界面批量");
}

void ImageExportJob::cancel()
{
	m_cancel=true;
	setState(JobState::Cancelled);
}
