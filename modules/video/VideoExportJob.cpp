#include "VideoExportJob.h"
#include "contracts/FFmpegHelper.h"

#include <QFile>
#include <QFileInfo>

VideoExportJob::VideoExportJob(QObject* parent)
    : QObject(parent)
{
    m_process.setProcessChannelMode(QProcess::SeparateChannels);

    connect(&m_process, &QProcess::readyReadStandardOutput,
            this, &VideoExportJob::onReadyReadStdout);
    connect(&m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &VideoExportJob::onFinished);
    connect(&m_process, &QProcess::errorOccurred,
            this, &VideoExportJob::onErrorOccurred);
}

VideoExportJob::~VideoExportJob()
{
    if (m_state == State::Running) {
        m_process.kill();
        m_process.waitForFinished(2000);
        cleanupPartialOutput();
    }
}

QString VideoExportJob::findFfmpeg()
{
	//统一走公共查找，避免和 B/C 各写一套
	return FFmpegHelper::ffmpegPath();
}

QStringList VideoExportJob::buildArgs(const VideoExportParams& p)
{
    QStringList a;

    a << QStringLiteral("-y"); // 覆盖输出（调用前已做存在性确认）

    if (p.startMs > 0)
        a << QStringLiteral("-ss") << QString::number(p.startMs / 1000.0, 'f', 3);

    // 时长限制必须放在 -i 之前（限制的是「输入片段」）。若放在 -i 之后则限制
    // 「输出时长」：变速(setpts)后输出时长 ≠ 输入片段时长，慢放(0.5x)会被
    // 截成只有一半，看起来就像没变速。
    const qint64 durMs = (p.endMs > p.startMs) ? (p.endMs - p.startMs) : 0;
    if (durMs > 0)
        a << QStringLiteral("-t") << QString::number(durMs / 1000.0, 'f', 3);

    a << QStringLiteral("-i") << p.inputPath;

    const bool needAudio = !p.mute && p.hasAudio;

    if (p.speed != 1.0) {
        // 视频用 setpts 变速，音频用 atempo 变速，二者一起变，保证音画同步。
        const QString tempo = QString::number(p.speed);
        if (needAudio) {
            a << QStringLiteral("-filter_complex")
              << QStringLiteral("[0:v]setpts=PTS/%1[v];[0:a]atempo=%1[a]").arg(tempo);
            a << QStringLiteral("-map") << QStringLiteral("[v]")
              << QStringLiteral("-map") << QStringLiteral("[a]");
        } else {
            a << QStringLiteral("-filter_complex")
              << QStringLiteral("[0:v]setpts=PTS/%1[v]").arg(tempo);
            a << QStringLiteral("-map") << QStringLiteral("[v]");
            a << QStringLiteral("-an");
        }
    } else {
        if (!needAudio)
            a << QStringLiteral("-an");
    }

    a << QStringLiteral("-c:v") << QStringLiteral("libx264")
      << QStringLiteral("-preset") << QStringLiteral("fast")
      << QStringLiteral("-crf") << QStringLiteral("23")
      // 强制 4:2:0：若源是 4:4:4/4:2:2（如部分 HEVC），不指定会输出
      // High 4:4:4 H.264，WMF 后端无法解码；yuv420p 兼容性最好。
      << QStringLiteral("-pix_fmt") << QStringLiteral("yuv420p");
    if (needAudio)
        a << QStringLiteral("-c:a") << QStringLiteral("aac");

    // 机器可读进度输出到 stdout
    a << QStringLiteral("-progress") << QStringLiteral("pipe:1")
      << QStringLiteral("-nostats");

    a << p.outputPath;
    return a;
}

void VideoExportJob::start(const VideoExportParams& params)
{
    if (m_state == State::Running)
        return;

    m_params = params;
    m_lastPercent = -1;

    const QString ffmpeg = findFfmpeg();
    if (ffmpeg.isEmpty()) {
        setState(State::Failed);
        emit finished(false,
            QStringLiteral("未找到 FFmpeg。请设置环境变量 FFMPEG_PATH，或在程序目录放置 ffmpeg_path.txt。"),
            QString());
        return;
    }

    // 输出总时长 = 源区间时长 / 倍速，用于进度百分比
    const qint64 durMs = (params.endMs > params.startMs) ? (params.endMs - params.startMs) : 0;
    m_totalOutputSec = (durMs > 0 ? durMs : 1) / 1000.0 / params.speed;
    if (m_totalOutputSec <= 0)
        m_totalOutputSec = 1.0;

    // 删除可能存在的残留输出，避免旧文件被误认为成功结果
    QFile::remove(m_params.outputPath);

    setState(State::Running);
    emit progress(0);
    m_process.start(ffmpeg, buildArgs(params));
}

void VideoExportJob::cancel()
{
    if (m_state != State::Running)
        return;
    setState(State::Cancelled);
    m_process.kill();
    m_process.waitForFinished(2000);
    cleanupPartialOutput();
    emit finished(false, QStringLiteral("已取消导出"), m_params.outputPath);
}

void VideoExportJob::onReadyReadStdout()
{
    // -progress pipe:1 输出形如 "out_time_us=1234567" 的 key=value 行
    while (m_process.canReadLine()) {
        const QByteArray line = m_process.readLine().trimmed();
        if (!line.startsWith("out_time_us="))
            continue;
        const int eq = line.indexOf('=');
        bool ok = false;
        const qlonglong us = line.mid(eq + 1).toLongLong(&ok);
        if (!ok)
            continue;
        const double sec = us / 1000000.0;
        const int pct = qBound(0, int(sec / m_totalOutputSec * 100.0), 100);
        if (pct != m_lastPercent) {
            m_lastPercent = pct;
            emit progress(pct);
        }
    }
}

void VideoExportJob::onFinished(int exitCode, QProcess::ExitStatus status)
{
    if (m_state != State::Running)
        return; // 取消路径已处理

    if (status == QProcess::CrashExit || exitCode != 0) {
        QString err = QString::fromLocal8Bit(m_process.readAllStandardError()).trimmed();
        if (err.size() > 300)
            err = err.right(300);
        cleanupPartialOutput();
        setState(State::Failed);
        emit finished(false, err.isEmpty() ? QStringLiteral("FFmpeg 执行失败") : err,
                      m_params.outputPath);
        return;
    }

    if (QFileInfo::exists(m_params.outputPath)) {
        setState(State::Succeeded);
        emit finished(true, QStringLiteral("导出成功"), m_params.outputPath);
    } else {
        setState(State::Failed);
        emit finished(false, QStringLiteral("FFmpeg 结束但未生成输出文件"), m_params.outputPath);
    }
}

void VideoExportJob::onErrorOccurred(QProcess::ProcessError error)
{
    if (m_state != State::Running)
        return;

    QString reason;
    switch (error) {
    case QProcess::FailedToStart:
        reason = QStringLiteral("无法启动 FFmpeg（路径错误或缺少依赖）");
        break;
    case QProcess::Crashed:
        reason = QStringLiteral("FFmpeg 崩溃");
        break;
    default:
        reason = QStringLiteral("FFmpeg 运行错误");
        break;
    }
    cleanupPartialOutput();
    setState(State::Failed);
    emit finished(false, reason, m_params.outputPath);
}

void VideoExportJob::setState(State s)
{
    if (m_state == s)
        return;
    m_state = s;
    emit stateChanged();
}

void VideoExportJob::cleanupPartialOutput()
{
    if (!m_params.outputPath.isEmpty() && QFileInfo::exists(m_params.outputPath))
        QFile::remove(m_params.outputPath);
}
