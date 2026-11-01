#include <QtTest>

#include "../AudioController.h"

// 只测纯函数和参数校验，不依赖 ffmpeg / 真实文件
class TestAudio:public QObject
{
	Q_OBJECT
private slots:
	void dbToLinear();
	void buildFilter_empty();
	void buildFilter_gainAndFade();
	void segmentDuration();
	void validate_ok();
	void validate_fadeTooLong();
	void validate_gainOutOfRange();
	void validate_overwriteSource();
};

void TestAudio::dbToLinear()
{
	QVERIFY(qAbs(AudioController::dbToLinear(0.0)-1.0)<1e-9);
	QVERIFY(qAbs(AudioController::dbToLinear(6.0)-1.995262)<1e-5);
	QVERIFY(qAbs(AudioController::dbToLinear(-12.0)-0.251189)<1e-5);
}

void TestAudio::buildFilter_empty()
{
	QCOMPARE(AudioController::buildFilter(0.0,0.0,0.0,10.0),QString());
}

void TestAudio::buildFilter_gainAndFade()
{
	const QString f=AudioController::buildFilter(6.0,1.0,2.0,10.0);
	QCOMPARE(f,QStringLiteral("volume=1.9953,afade=t=in:st=0:d=1.000,afade=t=out:st=8.000:d=2.000"));
}

void TestAudio::segmentDuration()
{
	AudioExportParams p;
	p.durationMs=10000;

	p.inMs=1000;
	p.outMs=5000;
	QCOMPARE(p.segmentDurationMs(),qint64(4000));

	// outMs 落在 inMs 之前表示"到结尾"
	p.inMs=1000;
	p.outMs=0;
	QCOMPARE(p.segmentDurationMs(),qint64(9000));

	// 起点本身超界时不做修正，交给校验去报
	p.inMs=10000;
	p.outMs=0;
	QCOMPARE(p.segmentDurationMs(),qint64(0));
}

void TestAudio::validate_ok()
{
	const QString dir=QDir::tempPath();
	AudioExportParams p;
	p.srcPath=dir+QStringLiteral("/src.wav");
	p.durationMs=10000;
	p.inMs=1000;
	p.outMs=6000;
	p.gainDb=6.0;
	p.fadeInSec=0.5;
	p.fadeOutSec=1.0;
	p.mono=true;

	QCOMPARE(AudioController::validateParams(p,dir+QStringLiteral("/out.wav")),QString());
}

void TestAudio::validate_fadeTooLong()
{
	const QString dir=QDir::tempPath();
	AudioExportParams p;
	p.srcPath=dir+QStringLiteral("/src.wav");
	p.durationMs=10000;
	p.inMs=0;
	p.outMs=2000;      // 片段 2 秒
	p.fadeInSec=1.5;
	p.fadeOutSec=1.5;  // 合计 3 秒 > 2 秒

	QVERIFY(!AudioController::validateParams(p,dir+QStringLiteral("/out.wav")).isEmpty());
}

void TestAudio::validate_gainOutOfRange()
{
	const QString dir=QDir::tempPath();
	AudioExportParams p;
	p.srcPath=dir+QStringLiteral("/src.wav");
	p.durationMs=10000;
	p.inMs=0;
	p.outMs=5000;
	p.gainDb=13.0;

	QVERIFY(!AudioController::validateParams(p,dir+QStringLiteral("/out.wav")).isEmpty());
}

void TestAudio::validate_overwriteSource()
{
	const QString path=QDir::tempPath()+QStringLiteral("/src.wav");
	AudioExportParams p;
	p.srcPath=path;
	p.durationMs=10000;
	p.inMs=0;
	p.outMs=5000;

	QVERIFY(!AudioController::validateParams(p,path).isEmpty());
}

QTEST_MAIN(TestAudio)
#include "tst_audio.moc"
