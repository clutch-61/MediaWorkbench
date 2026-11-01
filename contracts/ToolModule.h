#pragma once
#include <QWidget>
#include <QString>

//模块统一接口
class ToolModule:public QWidget
{
	Q_OBJECT
public:
	explicit ToolModule(QWidget* parent=nullptr):QWidget(parent){}
	~ToolModule() override=default;

	virtual QString moduleId() const=0;
	virtual void openFile(const QString& filePath)=0;
	virtual void setActive(bool active)=0;

signals:
	void statusMessage(const QString& message);
	void artifactReady(const QString& filePath,const QString& kind);
};
