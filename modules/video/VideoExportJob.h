#pragma once

#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>

// 导出参数：结构化描述一次视频片段 / 倍速 / 静音导出。
struct VideoExportParams
{
    QString inputPath;
    QString outputPath;
    qint64  startMs  = 0;     // 起点（毫秒）
    qint64  endMs    = 0;     // 终点（毫秒）；<= startMs 表示导出到结尾
    double  speed    = 1.0;   // 导出倍速（0.5 / 1.0 / 1.5 / 2.0）
    bool    mute     = false; // 是否静音导出
    bool    hasAudio = true;  // 源文件是否含音轨
};

// 负责一次导出的完整生命周期：Idle → Running → Succeeded / Failed / Cancelled。
// 通过 QProcess 异步调用 FFmpeg，绝不阻塞界面线程。
class VideoExportJob : public QObject
{
    Q_OBJECT

public:
    enum class State { Idle, Running, Succeeded, Failed, Cancelled };

    explicit VideoExportJob(QObject* parent = nullptr);
    ~VideoExportJob() override;

    void start(const VideoExportParams& params);
    void cancel();

    State state() const { return m_state; }
    bool isRunning() const { return m_state == State::Running; }

    // 纯函数：根据参数生成 FFmpeg 参数列表（便于单元测试，不依赖真实文件）。
    static QStringList buildArgs(const VideoExportParams& p);

    // 定位 FFmpeg：环境变量 → 配置文件 → PATH → 已知位置。
    static QString findFfmpeg();

signals:
    void progress(int percent);
    void finished(bool ok, const QString& message, const QString& outPath);
    void stateChanged();

private slots:
    void onReadyReadStdout();
    void onFinished(int exitCode, QProcess::ExitStatus status);
    void onErrorOccurred(QProcess::ProcessError error);

private:
    void setState(State s);
    void cleanupPartialOutput();

    QProcess m_process;
    VideoExportParams m_params;
    State m_state = State::Idle;
    double m_totalOutputSec = 1.0; // 输出总时长，用于进度百分比
    int m_lastPercent = -1;
};
