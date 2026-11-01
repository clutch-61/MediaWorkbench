#include "../../contracts/ModuleFactories.h"
#include "FrameModuleWidget.h"

ToolModule* createFrameModule(QWidget* parent)
{
	return new FrameModuleWidget(parent);
}
