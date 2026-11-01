#pragma once
#include "../../contracts/ToolModule.h"
#include "FrameController.h"

class QLabel;
class QListWidget;
class QPushButton;
class QProgressBar;
class QCheckBox;

class FrameModuleWidget:public ToolModule
{
	Q_OBJECT
public:
	explicit FrameModuleWidget(QWidget* parent=nullptr);
	QString moduleId() const override{return "frame";}
	void openFile(const QString& filePath) override;
	void setActive(bool active) override;

private slots:
	void onOpenImage();
	void onOpenVideoFrame();
	void onAddFilter();
	void onExport();
	void onBatch();
	void onCancel();
	void refreshPreview();

private:
	void buildUi();

	FrameController* m_ctrl=nullptr;
	QLabel* m_view=nullptr;
	QListWidget* m_list=nullptr;
	QProgressBar* m_prog=nullptr;
	QPushButton* m_btnExport=nullptr;
	QPushButton* m_btnBatch=nullptr;
	QPushButton* m_btnCancel=nullptr;
	QCheckBox* m_showOrig=nullptr;
	bool m_cancel=false;
};
