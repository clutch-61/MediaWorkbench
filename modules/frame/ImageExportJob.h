#pragma once
#include "../../contracts/ExportJob.h"
#include <QImage>
#include <QStringList>

class ImageExportJob:public ExportJob
{
	Q_OBJECT
public:
	explicit ImageExportJob(QObject* parent=nullptr);
	void setSingle(const QImage& img,const QString& outPath);
	void setBatch(const QStringList& files,const QString& outDir);
	void start() override;
	void cancel() override;

private:
	QImage m_img;
	QString m_out;
	QStringList m_files;
	QString m_outDir;
	bool m_batch=false;
	bool m_cancel=false;
};
