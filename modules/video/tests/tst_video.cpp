#include <QtTest>

#include "../VideoExportJob.h"
#include "../VideoController.h"

// 视频模块纯逻辑单元测试。
// 只覆盖不依赖真实媒体文件的函数：时间格式化、导出参数校验、FFmpeg 参数生成。
class TestVideo : public QObject
{
    Q_OBJECT

private slots:
    void testFormatTime();
    void testBuildArgs_speedKeepAudio();
    void testBuildArgs_speedMute();
    void testBuildArgs_noSpeedKeepAudio();
    void testBuildArgs_segmentInOut();
    void testExtractVideoCodec();
    void testIsQtFriendlyCodec();
};

void TestVideo::testFormatTime()
{
    QCOMPARE(VideoController::formatTime(0), QStringLiteral("00:00"));
    QCOMPARE(VideoController::formatTime(83000), QStringLiteral("01:23"));
    QCOMPARE(VideoController::formatTime(3600000 + 83000), QStringLiteral("01:01:23"));
    QCOMPARE(VideoController::formatTime(-5), QStringLiteral("00:00"));
}

void TestVideo::testBuildArgs_speedKeepAudio()
{
    VideoExportParams p;
    p.inputPath = QStringLiteral("in.mp4");
    p.outputPath = QStringLiteral("out.mp4");
    p.startMs = 0;
    p.endMs = 10000;
    p.speed = 2.0;
    p.mute = false;
    p.hasAudio = true;

    const QStringList a = VideoExportJob::buildArgs(p);
    const QString cmd = a.join(QLatin1Char(' '));
    QVERIFY(a.contains(QStringLiteral("-filter_complex")));
    QVERIFY(cmd.contains(QStringLiteral("setpts")));
    QVERIFY(cmd.contains(QStringLiteral("atempo")));
    QVERIFY(a.contains(QStringLiteral("libx264")));
    QVERIFY(a.contains(QStringLiteral("aac")));
    QVERIFY(!a.contains(QStringLiteral("-an")));
    QVERIFY(a.contains(QStringLiteral("-t"))); // 10 秒片段
    QVERIFY(a.contains(QStringLiteral("out.mp4")));
}

void TestVideo::testBuildArgs_speedMute()
{
    VideoExportParams p;
    p.inputPath = QStringLiteral("in.mp4");
    p.outputPath = QStringLiteral("out.mp4");
    p.speed = 2.0;
    p.mute = true;
    p.hasAudio = true;

    const QStringList a = VideoExportJob::buildArgs(p);
    const QString cmd = a.join(QLatin1Char(' '));
    QVERIFY(cmd.contains(QStringLiteral("setpts")));
    QVERIFY(!cmd.contains(QStringLiteral("atempo"))); // 静音不处理音轨
    QVERIFY(a.contains(QStringLiteral("-an")));
    QVERIFY(!a.contains(QStringLiteral("aac")));
}

void TestVideo::testBuildArgs_noSpeedKeepAudio()
{
    VideoExportParams p;
    p.inputPath = QStringLiteral("in.mp4");
    p.outputPath = QStringLiteral("out.mp4");
    p.speed = 1.0;
    p.mute = false;
    p.hasAudio = true;

    const QStringList a = VideoExportJob::buildArgs(p);
    QVERIFY(!a.contains(QStringLiteral("-filter_complex"))); // 原速无需滤镜
    QVERIFY(a.contains(QStringLiteral("aac")));
    QVERIFY(!a.contains(QStringLiteral("-an")));
    QVERIFY(a.contains(QStringLiteral("yuv420p"))); // 输出统一 4:2:0，保证后端可解码
}

void TestVideo::testBuildArgs_segmentInOut()
{
    VideoExportParams p;
    p.inputPath = QStringLiteral("in.mp4");
    p.outputPath = QStringLiteral("out.mp4");
    p.startMs = 5000;
    p.endMs = 15000;
    p.speed = 0.5;
    p.mute = true;

    const QStringList a = VideoExportJob::buildArgs(p);
    const QString cmd = a.join(QLatin1Char(' '));
    QVERIFY(a.contains(QStringLiteral("-ss")));
    QVERIFY(a.contains(QStringLiteral("5.000")));   // 起点 5 秒
    QVERIFY(a.contains(QStringLiteral("10.000")));  // 区间 10 秒
    QVERIFY(cmd.contains(QStringLiteral("setpts")));  // 慢速 0.5x
    QVERIFY(cmd.contains(QStringLiteral("0.5")));
}

void TestVideo::testExtractVideoCodec()
{
    QCOMPARE(VideoController::extractVideoCodec(
                 QStringLiteral("  Stream #0:0: Video: h264 (High) (avc1 / 0x31637661), yuv420p")),
             QStringLiteral("h264"));
    QCOMPARE(VideoController::extractVideoCodec(
                 QStringLiteral("  Stream #0:0: Video: hevc (Main) (hvc1 / 0x31637668), yuv420p")),
             QStringLiteral("hevc"));
    QCOMPARE(VideoController::extractVideoCodec(QStringLiteral("  Stream #0:1: Audio: aac")),
             QString());
}

void TestVideo::testIsQtFriendlyCodec()
{
    QVERIFY(VideoController::isQtFriendlyCodec(QStringLiteral("h264")));
    QVERIFY(VideoController::isQtFriendlyCodec(QStringLiteral("avc1")));
    QVERIFY(!VideoController::isQtFriendlyCodec(QStringLiteral("hevc")));
    QVERIFY(!VideoController::isQtFriendlyCodec(QStringLiteral("av1")));
    QVERIFY(!VideoController::isQtFriendlyCodec(QStringLiteral("vp9")));
}

QTEST_GUILESS_MAIN(TestVideo)
#include "tst_video.moc"
