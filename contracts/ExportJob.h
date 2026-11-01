#pragma once
#include <QObject>
#include <QString>

enum class JobState{
	Idle,
	Running,
	Succeeded,
	Failed,
	Cancelled
};

//导出任务基类 三个模块自己派生
class ExportJob:public QObject
{
	Q_OBJECT
public:
	explicit ExportJob(QObject* parent=nullptr):QObject(parent),m_state(JobState::Idle){}
	~ExportJob() override=default;

	JobState state() const{return m_state;}
	virtual void start()=0;
	virtual void cancel()=0;

signals:
	void progressChanged(int percent);
	void finished(bool ok,const QString& outPath,const QString& err);

protected:
	void setState(JobState s){m_state=s;}

private:
	JobState m_state;
};
