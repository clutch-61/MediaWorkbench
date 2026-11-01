# MediaWorkbench · 面向对象程序设计大作业

简易音视频处理工具箱（课程大作业）。

三人按模块联合开发：

| 同学 | 模块 | 目录 |
|------|------|------|
| A | 视频工作台 | \modules/video\ |
| B | 音频工作台 | \modules/audio\ |
| C | 图像帧工作台 | \modules/frame\ |

- 公共契约：\contracts/\（\ToolModule\ 多态接口等）
- 主程序：\pp/\ → \media_workbench- 独立调试：\ideo_demo\ / \udio_demo\ / \rame_demo
> 三人按视频 / 音频 / 图像模块分别完善后合并联调。

## 直接运行（本仓库若带本地构建）

若本地有 \uild_vs/Debug/\（默认不上传到 GitHub）：

\build_vs\\Debug\\media_workbench.exe
\
详见 \运行环境说明.txt\。

## 文档

- \软件设计说明书.txt\ — 按课件文档规范撰写
- \运行环境说明.txt\ — 运行依赖与入口

## 技术栈

Windows · C++17 · Qt 6 Widgets · CMake · FFmpeg（QProcess 调用）
