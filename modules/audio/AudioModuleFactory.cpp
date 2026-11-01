#include "../../contracts/ModuleFactories.h"
#include "AudioModuleWidget.h"

ToolModule* createAudioModule(QWidget* parent)
{
	return new AudioModuleWidget(parent);
}
