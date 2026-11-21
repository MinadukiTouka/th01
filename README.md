# 东方灵异传（TH01 / Highly Responsive to Prayers）Android 移植

把《東方靈異伝 ～ Highly Responsive to Prayers》移植到 Android 的个人项目。

- 包名：`com.zun.th01`
- ABI：`arm64-v8a`
- minSdk：26，targetSdk：36
- 构建：CMake + NDK r27b + Gradle 9.8.2 / AGP 9.4.1
- 代码基线：[ReC98](https://github.com/nmlgc/ReC98) 的 TH01 反编译代码
- 工程模板：`东方风神录安卓移植`（TH10，C++ + SDL2）

## 当前状态

- [x] ReC98 TH01 可移植源码接入，1 MiB 实模式内存、PC-98 4-plane VRAM、GRCG/EGC 模拟
- [x] 两条路线、全部 20 关，原始 Boss 顺序
- [x] SDL2 Android 后端，640×400 画面缩放
- [x] 触屏操作：左右滑块、B / Z / X / Esc，移动只响应左侧控件
- [x] 16 首 BGM 离线 OGG；17 个 SE 使用 MDRV98 原始效果录制
- [x] 暂停菜单、路线选择、存档/名字输入、上下选择
- [x] 标题画面 `START / CONTINUE / OPTION / QUIT`
- [x] 续关界面：不会自动确认，保留当前路线和关卡
- [x] 名称/分数文本 Shift-JIS 修正，中文/日文全角字形正常
- [x] Android 上关闭 Z/X 双击 Bomb，只保留 B 键
- [x] `1.3.0` APK 构建通过（`versionCode 10031`）

## 目录

| 路径 | 内容 |
| --- | --- |
| `th01-port/rec98/` | ReC98 TH01 源码与少量兼容修正 |
| `th01-port/port/` | 可移植运行时：PC-98 硬件、字体、音频、输入 |
| `th01-port/android-host/` | Android SDL2 入口、JNI、画面呈现与音频回调 |
| `android/` | Gradle 工程、Java 启动器/GameActivity、CMake 配置 |
| `docs/` | 调研、技术路线、进度记录、音频来源、截图 |
| `PLAN.md` / `DECISIONS.md` | 阶段计划与技术决策 |
| `tools/` | 本地工具链环境脚本 |
| `scripts/` | 构建、安装、无线 ADB 辅助脚本 |

## 下载

当前版本：[`v1.3.0`](https://github.com/MinadukiTouka/th01/releases/tag/v1.3.0)（`th01-1.3.0-android-apk.apk`，`versionCode 10031`）。

## 构建

本仓库**不包含**原版游戏数据、字体、BGM/SE，也不包含 APK 和签名文件。构建前需要：

1. 自备正版 TH01 数据，放入 `android/app/src/main/assets/th01/`；
2. 准备 BGM/SE 资源（见 `docs/04-音频来源.md`）；
3. 准备 JDK 17、Android SDK、NDK r27b 和 Gradle。

```powershell
$env:JAVA_HOME = "C:\th08-toolchain\jdk17"
$env:ANDROID_HOME = "C:\th08-toolchain\android-sdk"
$env:ANDROID_NDK_HOME = "$env:ANDROID_HOME\ndk\27.1.12297006"
Set-Location android
gradle assembleRelease
```

签名配置在 `android/app/build.gradle.kts` 中读取本地 `secrets/`；不要把 keystore 提交到仓库。

## 版本

当前内部版本：`1.3.0` / `10031`。历史构建记录见 [`docs/03-进度记录.md`](docs/03-进度记录.md)。

## 法律声明

本项目为非官方、非盈利的技术研究项目，与上海爱丽丝幻乐团及 ZUN 无任何关联。
《東方靈異伝 ～ Highly Responsive to Prayers》的著作权归上海爱丽丝幻乐团及 ZUN 所有。

本仓库不包含原版游戏数据、PC-98 原盘映像、原版字体或原版音乐。使用者必须合法持有原作，
仅在个人、非商业环境中使用。

ReC98 仓库目前没有 LICENSE 文件；在许可问题解决前，本仓库只应作为个人研究使用，
不应公开分发由 ReC98 代码编译出的二进制。
