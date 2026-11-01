# MediaWorkbench · 面向对象程序设计大作业

简易音视频处理工具箱（课程大作业）。

按模块联合开发：

| 模块代号 | 模块 | 目录 |
|------|------|------|
| A | 视频工作台 | `modules/video` |
| B | 音频工作台 | `modules/audio` |
| C | 图像帧工作台 | `modules/frame` |

- 公共契约：`contracts/`（`ToolModule` 多态接口等）
- 主程序：`app/` → `media_workbench`
- 独立调试：`video_demo` / `audio_demo` / `frame_demo`

> 视频 / 音频 / 图像模块分别完善后合并联调。

## 直接运行（本仓库不包含编译产物）

编译结果默认不上传 GitHub。在原开发电脑上可运行：

```
build_vs\Debug\media_workbench.exe
```

详见仓库内 `运行环境说明.txt`。

## 文档

- `软件设计说明书.txt` — 按课件文档规范撰写
- `运行环境说明.txt` — 运行依赖与入口

## 技术栈

Windows · C++17 · Qt 6 Widgets · CMake · FFmpeg（QProcess 调用）
