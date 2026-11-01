#pragma once
#include <QString>

//找ffmpeg / 抽一帧
class FFmpegHelper
{
public:
	static QString ffmpegPath();
	static QString ffprobePath();
	static bool available();
	//同步抽帧，成功返回true
	static bool extractFrame(const QString& video,double sec,const QString& outPath);
};
