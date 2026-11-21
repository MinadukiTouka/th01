# 《東方靈異伝 ～ Highly Responsive to Prayers》Android 移植计划

> 状态：**调研稿 v0.1（只调研，未写移植代码）**
> 日期：2026-11-08
> 目标：`com.zun.th01` / `arm64-v8a` / `minSdk 26` / `targetSdk 36`
> 构建：CMake + NDK r27b + Gradle 9.8.2（AGP 9.4.1）
> 硬约束：不改动隔壁 `东方妖妖梦/东方永夜抄/东方风神录安卓移植`；公开仓库与公开 APK 不含原盘映像、原版字体、原版音乐。

---

## 0. 结论先行

1. **模板选 TH10（风神录）**：三套里只有它是 C++ + NDK + Gradle + SDL2 + GLES3 的 Android 原生工程（上游是 CMake 工程，Android 入口用 ndk-build）；TH08 是 Rust/wgpu，TH07 是 Termux 上的 SDL3 构建记录，二者都不能作为本项目的构建骨架。
2. **ReC98 的 TH01 已经 100% 还原**：官网进度页显示 `OP.EXE` / `REIIDEN.EXE` / `FUUIN.EXE` 的 reverse-engineered、finalized、position independence 都是 100%。**没有空的 stub**；剩下的汇编是 ZUN 手写代码和 master.lib 的 16 位汇编库，不是未还原的游戏逻辑。
3. **TH01 不是 PMD，是 MDRV2**：音乐文件是 `.MDT`（`MDRV98.COM`，MDRV2 ver 3.4F，INT F2h），不是 TH02 以后的 PMD `.M`。这点会影响阶段 3 的方案，必须早做决定。
4. **这不是“从零重写”，而是“游戏逻辑原样保留 + 重写 PC-98 运行时”**：TH01 的游戏代码约 25.6k 行（不含注释），其中约 19k 行是纯玩法逻辑；需要新写的是 16 位/PC-98 兼容层（平面 VRAM、EGC/GRCG、master.lib API、文本/字体、文件、输入、音频），估 6–10k 行 C++。
5. **最大的三个风险**：ReC98 没有 license（公开分发前必须解决）、MDRV2 音乐（没有现成转换器）、全 20 关 + 双路线的体量。建议先做“地狱路线 Stage 1”垂直切片。

---

## 1. 隔壁项目调研

### 1.1 三套工程的对比

| 维度 | TH07 妖妖梦 | TH08 永夜抄 | TH10 风神录 |
| --- | --- | --- | --- |
| 工程形态 | 只有一份移植报告 `.txt`，源码/构建不在工作区 | `th08-rs/`（Rust 净室重写）+ `docs/ scripts/ tools/` | `th10-port/`（C++ 上游 + Android 后端）+ `docs/ scripts/ tools/` |
| 语言 | C++（上游 some100/th07） | Rust | C++17 |
| 构建 | Termux：CMake + Ninja + Gradle | Cargo + Gradle | Gradle + ndk-build（TH10 因 CMake 崩溃改的） |
| 渲染 | SDL3 | wgpu（Vulkan） | SDL2 + GLES3 |
| 音频 | SDL3 | 上游 winit/音频 | SDL2 queued audio（AAudio/OpenSL） |
| 触屏 | 报告里没有工程可复用 | `touch.rs`，含惯用手/安全区/手势 | `AndroidMain.cpp` + Java 覆盖层，含惯用手 |
| 打包 | Termux 手工 | Gradle，`com.zun.th08` | Gradle，`com.zun.th10`，公开包/`-full` 包 |
| 适合当模板？ | 否 | 否（栈不同） | **是** |

### 1.2 为什么是 TH10

- 目录结构、`tools/env.ps1`、`scripts/build-apk.ps1`、`common.ps1`、签名/产物校验、公开包/自用包分离，都是现成的。
- 原生侧是 `SDL2 + GLES3`，TH01 只需要 2D 纹理 + 音频队列，正好是它的子集。
- Android 后端已经把“数据目录三档搜索、系统 CJK 字体、触控、生命周期、Back=Esc、横屏沉浸式”踩过一遍。
- 工具链完全复用 `C:\th08-toolchain`（JDK17 / Gradle 9.8.2 / AGP 9.4.1 / SDK36 / NDK r27b / CMake 3.31.6 / MinGit）。

TH10 的关键文件（后续照抄结构，不照抄内容）：

```
th10-port/
  CMakeLists.txt
  game/ platform/ portable/ src/        # 上游
  android-host/AndroidMain.cpp          # 我们这种“平台层”该放的位置
  android/                              # Gradle 工程
    settings.gradle.kts
    build.gradle.kts
    app/build.gradle.kts
    app/src/main/AndroidManifest.xml
    app/src/main/jni/Android.mk         # TH10 用 ndk-build
    app/src/main/java/com/zun/th10/{LauncherActivity,GameActivity}.java
```

TH10 的构建事实（来自 `docs/01`、`docs/03`、`docs/05`）：

| 项 | 值 |
| --- | --- |
| JDK | 17.0.21（`C:\th08-toolchain\jdk17`） |
| Gradle | 9.8.2 |
| AGP | 9.4.1 |
| SDK | platform 36 / build-tools 36.0.0 |
| NDK | 27.1.12297006（r27b） |
| CMake | 3.31.6（另有一份 SDK 自带 3.22.1） |
| SDL2 | 2.32.10（`tools/fetch-deps.ps1`） |
| FreeType | 2.15.0 |
| minSdk / targetSdk | 26 / 36 |
| ABI | 只发 `arm64-v8a` |
| 包名 | `com.zun.th10` |

TH10 的 `app/build.gradle.kts` 里有几件事值得直接沿用：

- `ndkVersion = "27.1.12297006"`、`compileSdk = 36`、`targetSdk = 36`。
- `signingConfigs` 从 `keystore.properties` 读，缺省回退到一次性 debug key。
- `packaging { jniLibs { useLegacyPackaging = false } }`，`.so` 不压缩。
- `androidResources { noCompress += listOf("dat", "ttc") }`。
- `sourceSets` 把 SDL2 的 `org.libsdl.app` Java 胶水直接指过去，不复制进仓库。
- 自定义 `BundledAssets` Task，按 `-Pth10.bundleFont` / `-Pth10.bundleGameData` 把数据拷进 `assets/`。

### 1.3 TH10 的触屏/惯用手做法（TH01 要沿用其“原则”，不沿用其布局）

- 启动器 `LauncherActivity` 里存 `SharedPreferences("port", "left_handed")`。
- 每次 `onCreate/onResume` 调 `nativeSetHandedness(0|1)`。
- 原生侧只改 `bomb_side`（+1 右 / -1 左）；Esc 覆盖层在 Java，盖在 SurfaceView 上，所以按 Esc 不会误触 Bomb。
- 安全区：TH08 报告 §7.6 记录了 `WindowInsets` + `gesture exclusion` 的处理；TH10 的 Java 覆盖层也受益于同一思路。
- **TH01 的布局不能照抄 TH10**：TH01 是打砖块，水平移动 + Z 射击 + X 打击，没有“拖动自机” analog 需求。应以“左右键 + Z/X + Bomb + Esc”的数字键布局为主。

### 1.4 构建环境的一个更正

TH10 的 `docs/03 §5.1` 说这台机器上“任何同时启用 C 与 CXX 的 CMake 工程都会在 `CMakeFindBinUtils` 崩溃（0xC0000409）”，因此改用 ndk-build。

我在本机做了最小复现：

```
CMake 3.31.6 + NDK r27b + Ninja
CMakeLists: project(smoke C CXX)
```

第一次失败的原因是 **PATH 里没有 Ninja**（报 `unable to find a build program corresponding to "Ninja"`），不是 0xC0000409。显式指定

```
-G Ninja
-DCMAKE_MAKE_PROGRAM=C:\th08-toolchain\mingw64\bin\ninja.exe
```

之后 **configure + build 都成功**（`libsmoke.so` 链接通过）。

因此本项目**优先按你的约束用 CMake + NDK + Gradle**；只有 Gradle 的 `externalNativeBuild.cmake` 集成再次出问题时，才回退到 TH10 的 ndk-build 方案，并在 `DECISIONS.md` 记录原因。

---

## 2. ReC98 / TH01 完成度

### 2.1 结论

- 仓库：<https://github.com/nmlgc/ReC98>
- 进度页：<https://rec98.nmlgc.net/>（截至 2026-01-19 的统计）
- 注：GitHub wiki 无法 clone/访问（`ReC98.wiki.git` 返回 `Repository not found`，网页返回 418/导航页）；格式信息来自 README、CONTRIBUTING、源码注释、官网/博客与 Pi/MDRV2 公开资料。
- 本次检出的提交：
  - `master`：`b6ba5b0a529edbb31efdf8c0e939263804f8ee47`
  - `debloated`：`28d59d31331e27fcbe61d1b65bd8c0cd618757d8`
- 官网数字：

| 指标 | TH01 | OP.EXE | REIIDEN.EXE | FUUIN.EXE |
| --- | --- | --- | --- | --- |
| Reverse-engineered | 100% | 100% | 100% | 100% |
| Finalized | 100% | 100% | 100% | 100% |
| Position independence | 100% | 100% | 100% | 100% |

“Finalized = 已反编译 或 已证明不可反编译”。所以**没有等待填的空 stub**；剩下的 `.asm` 是：

- ZUN 手写的、按原二进制逐字节匹配保留的汇编；
- master.lib（16 位 x86 汇编库）；
- PiLoad（`.GRP` 解码库）；
- 少量 C++ 文件里的内联 `asm`（13 处，5 个文件）。

### 2.2 代码规模

`th01/` 下：144 个 `.cpp`、96 个 `.hpp`、17 个 `.h`；去注释/空行后 **25,612 行**。

按子系统分（粗略，按文件路径归类）：

| 子系统 | 行数 | 说明 |
| --- | ---: | --- |
| `main/boss`、`main/bullet`、`main/player`、`main/stage`、`main/hud` | 16,490 | 纯玩法逻辑，主要复用 |
| `main` 其余 | 2,032 | 主循环、bonus、debug、particle、shape |
| `end` | 481 | 结局/Staff |
| `hardware` | 2,152 | 图形/文本/输入/EGC/GRCG 包装层，需改写 |
| `formats` | 1,300 | `.GRP/.GRZ/.GRF/.BOS/.PTN/.PF` 解析，逻辑可复用，底层 I/O 要换 |
| `hiscore` | 723 | 存档/排行榜，逻辑可复用 |
| `math` | 367 | 纯 C++，可复用 |
| 其他（`core`、`snd`、`zunsoft`、`shiftjis` 等） | 2,069 | 需要平台化 |

### 2.3 三个 EXE 已经被 `debloated` 合并成一个入口

这是本次调研最重要的发现之一。

`debloated` 分支新增了：

- `th01/pc98/entry.cpp` / `entry.hpp`：`main_setup`、`main_op`、`main_main`、`main_cutscene` 四个入口；`entrypoint_exec()` 在 DOS 下用 `execl()` 重启自身并传入口名。
- `th01/pc98/game_bat.cpp`：复刻 `GAME.BAT`（先 `mdrv98 -M7`，再 `zunsoft`，再 `op`，最后 `mdrv98 -R`）。
- `th01/pc98/blitters.cpp`：TH01 专用的快速 blitter 初始化。
- `resident.hpp` 去掉了大量 `__seg` 依赖，改成 `resident_get()` / `resident_get_or_create()`。
- `main_01.cpp`、`op_01.cpp` 等大量 `pascal` / 段声明被清理。

这意味着 Android 侧不需要真的 `exec` 三个 EXE：**把 `execl()` 换成进程内状态切换（或直接调用 `main_op` / `main_main` / `main_cutscene`）即可**。ReC98 README 也明确说：

> `debloated`: ... **Ports should start from that branch** ...

所以基线选 `debloated`，不是 `master`。

### 2.4 16 位残留的“硬点”

| 残留 | 位置/数量 | 处理 |
| --- | --- | --- |
| `platform.h` 直接 `#error` 拒绝非 Borland 编译器 | 根 `platform.h` | 重写为 Android/clang 版本 |
| `far` / `__seg` / `near` / `huge` | TH01 源码 47 处 `far`、21 处 `__seg`（不完全统计） | 宏为空；段指针改为“平面基址 + 偏移” |
| `pascal` / `__cdecl` / `__fastcall` / `__ds` | 85 处 `pascal` | 宏为空 / 去掉 |
| `int` 为 16 位 | 大量 `int` 用于坐标、计数 | 保留 32 位 `int`，但文件/共享结构体用固定宽度类型；评估 `-fshort-enums` |
| `MK_FP` / `FP_SEG` / `FP_OFF` | 8 / 13 处 | 段表 + 平面指针 |
| `peek/poke` / `inport/outport` / `int86` | 大量 | 软件 VRAM / 空 I/O / 回调 |
| 内联 `asm` | 13 处，5 个文件 | 逐处改 C++ |
| `#pragma option -...` | 36 处，31 个文件 | 删除/忽略；`#pragma startup` 改显式 init |
| `farmalloc/farfree`、`coreleft`、`filelength` | 少量 | 标准 C++ |
| `_ismbblead/_mbcjmstojis/_ismbbkana` | 文本层 | 自带 CP932→JIS 转换 |
| `bool16`（Borland 下 2 字节） | 结构体字段 | 改 `int16_t` |

**关键判断**：真正要重写的不是 25.6k 行游戏逻辑，而是它脚下的“PC-98 运行时”。游戏逻辑的 19k 行里，绝大多数只依赖：

- `screen_x_t` / `screen_y_t` / `pixel_t`（当作 32 位 int）；
- 平面 VRAM 的读写宏；
- `grcg_*` / `egc_*` / `graph_*` / `text_*` / `file_*` / `key_sense` / `irand`；
- 少量 `far` 指针和 `__seg`（集中在 `formats`、`core`、`hardware`）。

所以“去 16 位化”的工作量是**有限且可列举的**，不是不可控的全局改写。

---

## 3. 资源格式（已用本地原盘数据验证）

### 3.1 原盘文件清单

从本机 `E:\水無月桐花\游戏\(TH01-05).zip` 里的日版 TH01 HDI 提取（仅本地调研，不进入仓库）。HDI 是 Anex86 格式：4096 字节头 + FAT12 磁盘；簇大小 2048，数据区从 `0xFC00` 开始。

根目录关键文件：

| 文件 | 大小 | 用途 |
| --- | ---: | --- |
| `ST0.GRP` … `ST6.GRP` | 11–41 KB | 各关背景（Pi 格式） |
| `BOSS*.GRP` / `BOSS8.GRZ` | 0.4–296 KB | Boss 立绘/演出；`.GRZ` 是 16 图 RLE |
| `ED*.GRP` / `END*.GRP` | 5–32 KB | 结局图 |
| `OP_WIN.GRP` | 329 B | 标题/窗口小图 |
| `MASK.GRF` | 10.7 KB | HUD 用单平面 RLE |
| `TOUHOU.DAT` | 8.3 KB | BFNT 精灵（ZUN logo） |
| `東方靈異.伝` | 298,073 B | PF 归档，含 41 个文件（见下） |
| `MDRV98.COM` | 12,483 B | MDRV2 音乐驱动（TSR，INT F2h） |
| `*.MDT` | 0.5–5.5 KB | 19 个 BGM 文件（`MUSIC_FILES` 列 15 首，另有 `init/kami2/shugen/st6/syugen` 等；MDRV2 数据） |
| `ZIGOKU.MDE` | 762 B | 音效数据 |
| `REIIDEN.CFG` | 11 B | 设置 |

`東方靈異.伝`（PF 归档）里的 41 个条目（文件名 XOR `0xFF`）：

```
STAGE0.DAT … STAGE7.DAT          关卡对象数据
BOSS1.BOS … BOSS8_E2.BOS         Boss 精灵（5 平面，任意尺寸）
MIKO_AC.BOS / MIKO_AC2.BOS       自机精灵
BOSS5_GR.GRC … KUZI2.GRC         调色板/图形数据
BOSS3_M.PTN / MIKO.PTN / NUMB.PTN / STG.PTN / STG_B.PTN / TAMAYEN.PTN
```

也就是说：**换装/关卡/Boss 用的精灵都在这个 PF 归档里，不在根目录**。数据导入器要能识别这个文件名是 CP932 的归档。

### 3.2 `.GRP` = Pi 格式（“ZN”变体）

ReC98 的 `th01/formats/grp.h` 写得很清楚：

- 魔数 `"ZN"`（普通 Pi 是 `"Pi"`）；
- 640×400，4bpp（16 色）；
- 调色板是 PC-98 原生 4-bit 分量（0x0–0xF），不是 8-bit；
- 实际文件在 `"ZN"` 后有 `0x1A` 结束的注释区，然后才是 Pi 头；
- 头偏移（实测 `ST0.GRP`）：
  - `0x00` `"ZN"`
  - `0x02` `0x1A`（注释结束）
  - `0x03` 00（header start）
  - `0x04` mode
  - `0x05` aspect X
  - `0x06` aspect Y
  - `0x07` bitdepth = 4
  - `0x08` `"PC98"`（compressor model）
  - `0x0C` compressor data size = 0
  - `0x0E` width = `02 80` = 640（大端）
  - `0x10` height = `01 90` = 400（大端）
  - `0x12` palette：16 × RGB，每分量 1 字节（4-bit 值）
  - `0x42` 起是压缩流
- 解压算法：delta table（move-to-front）+ 序列重复；规范见 <https://mooncore.eu/bunny/txt/pi-pic.htm>。这是本项目阶段 1 的核心 decoder。

### 3.3 其他格式

| 扩展名 | 魔数/结构 | 说明 |
| --- | --- | --- |
| `.GRZ` | `"HGRZ"` + image_count + 16 个 int32 offset + total_size | 16 张 RLE 图；`th01/formats/grz.cpp` 有完整解码逻辑 |
| `.GRF` | `"HGRF"` | 单平面 RLE（HUD 的 `MASK.GRF`）；`formats/grf.hpp` 有说明 |
| `.BOS` | `"BOSS"` + header + 5 平面 | 5 平面 = B/R/G/E/alpha；`formats/bos.hpp` 有结构 |
| `.PTN` | `"HPTN"` + image_count + palette + 32×32×4 平面 | alpha 由颜色 15 推导（四平面 AND 取反）；`formats/ptn.cpp` |
| `.PF` | 64 × 32 字节头 | 文件名为逐字节 XOR `0xFF`；压缩类型 `0x95 0x95`（“封”）是 RLE，`0x88 0xF3` 等是原样；`formats/pf.cpp` |
| `.BFNT` | `"BFNT"` | `TOUHOU.DAT`，ZUN logo 的精灵库；`super_entry_bfnt` 读取 |
| `.MDT` | `02 03 0D 00 01 00` + 偏移表 + Shift-JIS 标题 + 命令流 | **MDRV2 音乐数据**，不是 PMD |

### 3.4 音乐：MDRV2，不是 PMD

- `MDRV98.COM` 的字符串明确写着：

```
MDRV2 ver 3.4F for  PC-98 Series  94/01/30
Copyright (c) 1989-94 Regulus / NARIMI.A  Rel.2.31
```

- `GAME.BAT` 的内容：

```bat
mdrv98 -M7
zunsoft
op %1 %2 %3 %4
mdrv98 -R
```

- TH01 的 `th01/snd/mdrv2.cpp` 走 `INT 0xF2`，功能号 `MSET/MPLAY/MSTOP/MFADE_*`；BGM 用 `.MDT`，SE 用 `.MDE`。
- TH02 以后才是 PMD（`INT 0x60`，`.M` 文件，`ONGCHK.COM`）。**不要把 TH02 的 PMD 方案套到 TH01 上。**
- 目前没有找到现成的 MDRV2 → MIDI/WAV/OGG 转换器；日文资料（Kerupani129 的总结）也说 `.MDT` 的转换方法“谜”，最实际的是 **PC-98 模拟器播放 + 录音**。
- MDRV2 的官方包在 Vector：<http://www.vector.co.jp/soft/dos/art/se018677.html>（`mdrv9231.lzh`，1994）。里面有 `MCOMP`（MML 编译器）和文档，但不是解码库。
- `MDRV98.COM` 是第三方（Regulus / NARIMI.A）的 DOS TSR，**不能进公开仓库/公开 APK**。

---

## 4. 去 16 位化评估

### 4.1 必须重写 / 替换

| # | 部分 | 具体内容 | 预估 |
| --- | --- | --- | --- |
| 1 | `platform.h` / `x86real.h` | Borland 类型、`far/__seg/near`、`MK_FP/FP_SEG/FP_OFF`、`peek/poke`、`inport/outport`、`int86`、`getvect/setvect`、`__emit__`、`_AX...`、`interrupt` | 中 |
| 2 | master.lib API | 约 60 个实际用到的函数：`file_*`、`key_*`、`graph_*`、`palette_*`、`text_*`、`grcg_*`、`egc_*`、`super_*`、`resdata/respal`、`irand/iatan2/isqrt`、`vsync_*`、`mem_*` | 大 |
| 3 | PC-98 视频 | 2 页 × 4 平面 × 640×400；GRCG 的 tile/RMW/TCR；EGC 的 copy/shift；GDC 的显示/滚动；TRAM 80×25 + 属性 | 大 |
| 4 | PiLoad / 图像解码 | `.GRP`（Pi）、`.GRZ`、`.GRF`、`.BOS`、`.PTN`、`.BFNT` | 中 |
| 5 | BIOS/DOS | `int 18h`（GDC）、`int 21h`（文件）、`int 1Ah`（时间）、键盘 BIOS、`spawn/exec` | 中 |
| 6 | 文本/字体 | KCG 字形读取（端口 A1/A3/A5/A9）、`_mbcjmstojis`、TRAM 属性、粗体/阴影/反色/闪烁 | 中 |
| 7 | 音频 | MDRV2 `.MDT/.MDE` → PCM/OGG；SDL2 音频队列；音量/暂停/焦点 | 大（见阶段 3） |
| 8 | 进程/常驻 | `resident_t __seg*`、Pi loader、三 EXE 切换 → 单进程 + 全局状态 | 小 |
| 9 | 编译器差异 | `#pragma option`、`#pragma startup`、`_asm`、`__fastcall`、`__ds`、16 位 `int`、`bool16`、enum 宽度、指针宽度 | 中 |
| 10 | 构建 | Borland + Tup → CMake + NDK + Gradle | 小 |

### 4.2 可以原样搬（加类型/宏适配）

- `th01/main/player/*`：自机移动、滑铲、连段、Bomb、死亡/复活；
- `th01/main/bullet/*`：弹幕、激光、导弹；
- `th01/main/boss/*`：全部 Boss 的弹幕/演出逻辑（约 30 万字节源码，最大块）；
- `th01/main/stage/*`：关卡对象、卡片翻转、道具、计时；
- `th01/main/hud/*`、`bonus.cpp`、`particle.cpp`、`shape.cpp`；
- `th01/end/*`、`hiscore/*`、`math/*`、`shiftjis/*`；
- `th01/formats/*` 的上层解析逻辑（把 `file_*` / `arc_file_*` 换成宿主 API 即可）。

### 4.3 16 位 `int` 的具体处理策略

1. **不改全局 `int` 宽度**。Android arm64 上 `int` 是 32 位；坐标/计数用 32 位不会破坏玩法，反而避免 16 位溢出。
2. **所有落盘/跨二进制结构体用固定宽度**：
   - `resident_t`：`int8_t/uint8_t/uint16_t/uint32_t`，按原顺序，`#pragma pack(1)` 或 `static_assert` 校验 `sizeof`；
   - `ptn_header_t`、`bos_header_t`、`pf_header_t`、`grz_header_t`、`pi_header_t`：固定宽度 + `static_assert`；
   - 分数/设置文件：固定宽度。
3. **enum 宽度**：ReC98 用 `_FORCE_INT16 = 0x7FFF` 把关键 enum 强制成 16 位。编译时评估 `-fshort-enums`（让 enum 取 2 字节），并用 `static_assert(sizeof(...))` 锁住；如果某些 enum 变成 1 字节，就改成 `int16_t` 字段。
4. **指针宽度**：只在内存结构里出现（`bos_image_t`、`ptn_t*`、`PiHeader`），不落盘；落盘结构里不要放指针。
5. **`bool16`**：Borland 下是 2 字节的 `int`；Android 上改成 `int16_t`，不要用 `int`。
6. **`long`**：Borland 和 Android 都是 32 位，可直接用；但仍建议落盘用 `int32_t`。

### 4.4 内联汇编清单（13 处）

| 文件 | 行 | 内容 | 处理 |
| --- | --- | --- | --- |
| `th01/zunsoft.cpp` | 216 | 读 PC-9821 机器标志 | 删除/返回固定值 |
| `th01/hardware/grppsafx.cpp` | 62 | 通过 KCG 端口取字形 | 改字体 provider |
| `th01/snd/mdrv2.cpp` | 75/84/87/94/155 | 调 MDRV2 TSR / 读中断向量 | 改音频后端 |
| `th01/main/boss/b20j.cpp` | 835 | 热路径寄存器技巧 | 改普通 C++ |
| `th01/main/bullet/pellet.cpp` | 420/430/433/436/442 | `REP MOVSW` 画弹 | 改 `memcpy`/循环 |

加上 `th01/hardware/vram_planes[bss].asm`、`player_is_hit[bss].asm` 这类数据/标志定义，都是小文件。

### 4.5 master.lib 的实际使用面

TH01 的三个 `.asm` 入口显式 include 了 **51 个 master.lib 模块**（含 data/bss），例如：

```
graph_400line / graph_clear / graph_show / graph_start / graph_scrollup
palette_init / palette_show
file_ropen / file_read / file_seek / file_close / file_create / file_write / file_exist
key_start / key_sense / keybios / keytable / keystart
egc / grcg_setcolor / grcg_pset
resdata / respal_exist / respal_free
random / isqrt / iatan2 / sin8 / atan8
text_clear / text_fillca
```

`bin/masters.lib` 里还会被链接器拉进其他函数（`super_*`、`grcg_circlefill`、`graph_pi_load_pack` 等）。所以不要按“230 个 asm 文件”估工作量；**按实际符号表/链接错误逐个补**，预计 60–80 个入口。

---

## 5. 目标架构

```
┌────────────────────────────────────────────────────────────┐
│ Android (Java)                                             │
│  LauncherActivity: 数据检查/导入/惯用手/开始               │
│  GameActivity: SDLActivity 子类, Back→Esc, 覆盖层按钮       │
└──────────────────────────┬─────────────────────────────────┘
                           │ JNI
┌──────────────────────────▼─────────────────────────────────┐
│ libmain.so (SDL2)                                          │
│  android-host/AndroidMain.cpp                              │
│   - SDL window + SDL_Renderer/GLES texture                 │
│   - 56.423 Hz 固定步进 + 显示刷新节流                      │
│   - 触摸/按键注入、生命周期、音频焦点                       │
│  port/pc98/                                                │
│   - PlanarVram (2 pages × 4 planes × 640×400)              │
│   - Palette, GRCG, EGC, TRAM, GDC 软件模拟                 │
│   - FileHost, TimeHost, InputHost, Resident                │
│  port/master/                                              │
│   - master.lib API 的 C++ 实现（只实现用到的）              │
│  port/data/                                                │
│   - Pi/GRZ/GRF/BOS/PTN/PF/BFNT 解码                        │
│  port/audio/                                               │
│   - OGG (stb_vorbis) / PCM / 可选 MDRV2 player             │
│  th01/ (ReC98 debloated, 游戏逻辑基本不改)                  │
└────────────────────────────────────────────────────────────┘
```

### 5.1 渲染

```
游戏写 4 个平面 (B/R/G/E)
        │
        ▼
PlanarVram::to_rgba8888()
  用 256 项 LUT 把每个 plane 字节展开成 8 像素
  4 个平面 → 16 色索引 → palette[index]
        │
        ▼
SDL_UpdateTexture(640×400 RGBA8888)
        │
        ▼
SDL_RenderCopy + 4:3 letterbox
```

- 颜色映射：PC-98 4-bit 分量 `v` → 8-bit `(v << 4) | v`（0→0，15→255）。
- 透明色 15：只在精灵 blit 时作为 alpha mask，最终画面仍按调色板画。
- 4:3 letterbox：640×400 拉伸到 4:3（PC-98 像素不是方形）；左右留黑边给虚拟键。
- 阶段 1 可以先全屏重转（256k 像素/帧，arm64 足够）；阶段 4 再加脏矩形。
- 纹理过滤用 nearest；可选整数倍缩放。

### 5.2 输入

原版键位（`main_01.cpp` 的 `input_sense`）：

| 键 | 作用 |
| --- | --- |
| ← → | 左右移动 |
| ↑ ↓ | 菜单 |
| Z | 射击 / 确认 |
| X | 打击 / 取消 |
| Esc | 暂停 |
| Return | 确认（菜单） |
| Z 或 X 双击 | Bomb（`BOMB_DOUBLETAP_WINDOW`） |

触屏方案：

- 左侧（或右手模式的左侧）：`←` `→` 两个大按钮；菜单时再加 `↑` `↓`。
- 右侧：`Z`（射击）、`X`（打击）、`Bomb`、`Esc`。
- `Bomb` 不模拟双击，直接在平台输入层置 `input_bomb = true`（更可靠；游戏逻辑本来就有这个变量）。
- 惯用手：左/右面板互换；Esc 放优势手对侧顶部；持久化到 `port.cfg` / SharedPreferences。
- 安全区：Java 侧读 `WindowInsets`，把安全矩形传给 native；所有按钮放在安全区内；贴边按钮申请 gesture exclusion。
- Back 键 → Esc；Home → 暂停/后台。
- 触屏按钮用 Java 覆盖层（透明按钮）实现，按下/抬起调 JNI `nativeKeyDown/Up`；这样不用改 SDL 的触摸坐标映射，也天然在 SurfaceView 之上，不会误触游戏区。

### 5.3 音频

先明确：**TH01 是 MDRV2，不是 PMD**。用户已决定：**所有 BGM/SE 都内置离线 OGG，本地只做一个完整 APK，不区分公开/`-full`**。

| 方案 | 做法 | 是否采用 | 工作量 | 风险 |
| --- | --- | --- | --- | --- |
| A. 离线 OGG | 在 PC-98 模拟器里播放 `.MDT`/`.MDE`，录 48 kHz WAV → OGG；App 用 `stb_vorbis` 解码 + SDL 音频队列 | **采用** | 1–2 周（含录音/循环点） | 录音质量/循环点 |
| B. 原生 MDRV2 播放器 | 解析 `.MDT` 命令流 + 实现 MDRV2 驱动 + YM2608(fmgen) | 后置备选（若 OGG 效果不可接受） | 4–8 周+ | 格式/驱动逆向，高 |
| C. 先静音 | 只做 SE 或完全静音 | 否决 | 0 | 体验差 |

执行顺序：

- 先把音频后端（SDL2 队列 + `stb_vorbis`）做出来，用占位 OGG 验证不崩；
- 再离线生成 19 个 `.MDT` + `ZIGOKU.MDE` 的 OGG，放进 APK 的 `assets/audio/`；
- BGM 循环点用固定 loop start/end（录制时录 2 遍，取稳定段）；SE 做池。
- 公开仓库最后再剥离 OGG；本地 APK 直接内置。

### 5.4 字体/文本

- 不内置原版 PC-98 字体 ROM。
- 首选 Android 系统 CJK 字体（`/system/fonts/NotoSansCJK-Regular.ttc` 等），用 FreeType 加载；TTC 用 `FT_New_Face(path, -1)` 选 JP face（TH10 已验证这条路）。
- 备选：内置一份 OFL/公共领域授权的 16×16 点阵字体（如 Shinonome/Misaki），保证无系统 CJK 的设备也能显示。
- 文本层：
  - 模拟 TRAM（80×25，JIS 码 + 属性）；
  - 用 CP932→JIS 转换表把游戏里的 Shift-JIS 字符串转成 JIS 码；
  - 用 FreeType 把字形栅格化成 16×16/8×16 单色位图，再套粗体/阴影/反色/下划线/闪烁；
  - 最终合成到 RGBA 帧（或先画到一张文本层，再与图形层合成）。
- ReC98 源码本身是 **CP932 编码**；NDK clang 能编译（只会报 `-Winvalid-source-encoding` 警告），加 `-Wno-invalid-source-encoding` 即可。**不要**把源文件整体转 UTF-8，否则字符串字面量的字节会变。

### 5.5 数据目录与导入

沿用 TH10 的三档搜索，但 TH01 需要处理 CP932 文件名：

```
1. /storage/emulated/0/Download/th01        （需要“所有文件访问”）
2. /sdcard/Android/data/com.zun.th01/files/th01   （adb push，无需权限）
3. /data/data/com.zun.th01/files/th01       （私有）
```

建议在启动器里做一次“导入/规范化”：

- 扫描用户选定的 TH01 目录；
- 按 8.3 名（CP932 → ASCII 显示）和文件头魔数识别：
  - `ST?.GRP`、`BOSS*.GRP`、`ED*.GRP`、`OP_WIN.GRP`、`CLEAR3.GRP`、`REIIDEN2/3.GRP`、`GAME_O.GRP`；
  - `BOSS8.GRZ`、`MASK.GRF`、`TOUHOU.DAT`；
  - PF 归档（`東方靈異.伝`，按 `0x95 0x95` 头识别）；
  - `*.MDT`、`ZIGOKU.MDE`（如果走音频路线）。
- 复制到应用私有/外置 files 目录，统一改成 ASCII 名（`th01.pf`、`st0.grp`…），并写 `manifest.json`；
- 缺失文件在启动器里明确列出，不“黑屏然后退出”。
- 原 EXE / `MDRV98.COM` 不需要复制（除非做 B 方案）。
- **本地完整 APK 可以直接内置这些资源**（用户 2026-11-08 决定）；上 GitHub 前再剥离成“无数据包”。

### 5.6 构建

- 工具链：复用 `C:\th08-toolchain`。
- 构建路径：`C:\th08-toolchain\build-target-th01`（CMake build dir），Gradle 工程在 ASCII junction `C:\th01ws` 下执行，避免中文路径。
- CMake：`-G Ninja -DCMAKE_MAKE_PROGRAM=...\ninja.exe`；NDK toolchain file；`ANDROID_ABI=arm64-v8a`；`ANDROID_PLATFORM=android-26`；`ANDROID_STL=c++_static`。
- Gradle：`externalNativeBuild.cmake`；`ndkVersion`、`compileSdk`、`minSdk`、`targetSdk` 同 TH10；`abiFilters += "arm64-v8a"`。
- C++ 选项：`-std=c++17 -fno-exceptions -fno-rtti -fno-strict-aliasing -ffp-contract=off -Wno-invalid-source-encoding`；是否加 `-fshort-enums` 由阶段 0 的 `static_assert` 测试决定。
- 产物：`dist/th01-<ver>-android-apk.apk`（本地完整包，内置数据 + OGG；上 GitHub 前再单独做无数据包）。

---

## 6. 阶段计划

> 用户 2026-11-08 已决定：**本地直接做完整 APK（双路线全 20 关，内置数据 + OGG），不区分公开/`-full`，不再按阶段交付**。
> 下面的“阶段”因此只作为**内部里程碑/检查点**，不是交付门槛；每个里程碑仍应能编译出可运行 APK，便于回归。
> 说明：你的原建议是 1 图形 → 2 输入 → 3 音频 → 4 存档 → 5 打包。实际移植中“游戏核心”是最大的一块，所以把它显式拆成阶段 4；阶段 3 的音频可以先独立做后端，再在阶段 4 接进游戏。

### 阶段 0：基线（2–3 天）

**任务**

- 建工作区骨架（`docs/ tools/ scripts/ th01-port/ third_party/ game-data/ secrets/ dist/`）。
- `tools/env.ps1`：复用 `C:\th08-toolchain`，建 `C:\th01ws` junction。
- 把 ReC98 `debloated`（`28d59d3`）作为 `th01-port/upstream` 的 git submodule/fork；记录提交。
- 写最小的 `platform.h` / `CMakeLists.txt` / `android/` Gradle 工程；空 `AndroidMain.cpp`。
- `LauncherActivity`：数据检查 + 惯用手按钮 + 开始游戏（先显示“数据缺失”）。
- 构建并安装空 APK。

**可验证产出**

- `dist/th01-0.0.1-android-apk.apk` 能安装、启动、显示启动器；
- `aapt2 dump badging` 显示 `package com.zun.th01`、`arm64-v8a`、`minSdk 26`；
- APK 内没有 `assets/*.grp`、`*.mdt`、`*.dat`；
- CMake + Gradle 集成通过（否则记录并回退 ndk-build）。

**风险**：Gradle 的 CMake 集成；中文路径；CP932 源文件。

---

### 阶段 1：图形（1–2 周）

**任务**

- `PlanarVram`：2 页 × 4 平面 × 640×400（每平面 32,000 字节），页切换、清屏、拷贝。
- `Palette`：16 色 × 4-bit RGB；4-bit→8-bit 映射；fade/tone 的接口先留空。
- Pi 解码器：`.GRP`（含 `"ZN"` 头、delta table、重复命令），输出 640×400 索引图；再转 4 平面。
- `.GRZ` / `.GRF` 解码器（阶段 1 可以先只做 `.GRP`；`.GRZ` 用于 Boss 8，后续补）。
- `to_rgba8888()`：LUT 展开 + 调色板；`SDL_UpdateTexture` + 4:3 letterbox。
- 数据导入器的最小版：识别 `ST0.GRP` / `OP_WIN.GRP`，复制到 `files/th01`。
- 帧循环：先 60 Hz 显示，逻辑步进留到阶段 2；阶段 1 只要求“显示一帧/轮播几张图”。

**可验证产出**

- `dist/th01-0.1.0-android-apk.apk` 启动后显示 **真实的 TH01 图像**（`ST0.GRP` 或 `OP_WIN.GRP`），4:3、无花屏、60 fps；
- 同一 `.GRP` 在桌面端解码成 PNG，与 NP2 截图做像素对比（允许调色板/缩放差异）；
- 连续切换 5 张 `.GRP` 不崩溃。

**风险**：Pi 解码边界（重复长度越界、首行重复）；调色板 4-bit 缩放；CP932 文件名。

---

### 阶段 2：输入与启动流程（2–4 周）

**任务**

- 移植 `platform.h` / `x86real.h` 的可移植版本：`far/__seg` 宏、段表、`peek/poke`、`inport/outport` 空实现、`int86` 回调。
- 实现 master.lib 的第一批：`file_*`、`key_sense`、`graph_*`、`palette_*`、`text_*`、`vsync_*`、`irand`、`mem_*`。
- 移植 `zunsoft.cpp`（logo）和 `op_01.cpp`（标题/菜单）的最小闭包；`main_setup`/`main_op` 入口。
- 文本层：TRAM + 字体 provider + CP932→JIS；先支持标题/菜单的固定文本。
- 触屏：Java 覆盖层（←/→/↑/↓/Z/X/Bomb/Esc），JNI 注入按键；惯用手；安全区。
- 帧节奏：**56.423 Hz** 固定步进（PC-98 原生帧率），显示刷新只负责呈现。
- Back→Esc；暂停/恢复。

**可验证产出**

- `dist/th01-0.2.0-android-apk.apk`：启动 → ZUNSOFT logo → 标题 → 菜单 → 路线选择；
- 触屏能完成菜单导航；左右手布局正确；Back 能暂停/返回；
- 文本可读（允许字形与 PC-98 有差异），无方块/乱码；
- 连续运行 10 分钟不崩溃。

**风险**：文本/字体效果；OP 菜单的阻塞式 `frame_delay`；56.423 Hz 与 60 Hz 显示的节流。

---

### 阶段 3：音频（1–2 周，方案 A）

**任务**

- SDL2 音频设备（48 kHz / stereo / S16），音频队列；暂停/恢复/焦点。
- `stb_vorbis`（或 libvorbis）解码 OGG；BGM 循环点；SE 池。
- 离线渲染流水线（桌面，需用户自备原盘）：
  - NP2/Anex86 播放 `.MDT`，录 WAV；
  - 用 `ffmpeg` 转 OGG（保留循环点元数据/或用固定 loop start/end）；
  - 对 `ZIGOKU.MDE` 的 SE 逐个录/切。
- **完整 APK 直接内置 `assets/audio/` 的 OGG**（用户已决定）；不再有公开包静音分支。
- 可选 spike：评估 MDRV2 原生播放器（只做可行性，不进首版）。

**可验证产出**

- 完整 APK：标题/关卡 BGM、射击/打击/爆炸 SE 正常；循环无明显接缝；暂停/恢复正确；
- 音频焦点丢失/恢复不炸音；
- 缺 OGG 时降级为静音并打日志，不崩溃。

**风险**：MDRV2 录音质量/循环；版权（OGG 不能公开）；模拟器音频设置差异。

---

### 阶段 4：游戏核心（6–10 周）

**任务（按垂直切片）**

1. **4a：地狱路线 Stage 1 可玩**
   - PF 归档读取（`STAGE0.DAT`、`MIKO_AC.BOS`、`STG.PTN` 等）；
   - `.BOS` / `.PTN` 解码 + 精灵实体；
   - GRCG/EGC 软件实现（fill/line/circle/box/copy/shift）；
   - 自机、阴阳玉、弹幕、Stage 对象、HUD、计分、死亡/复活；
   - 与 NP2 逐帧对比关键画面。
2. **4b：地狱路线全 20 关 + Boss**
   - 各 Boss 的 `.BOS`/`.GRZ`/弹幕；
   - 关卡切换、BGM 切换、结局。
3. **4c：封印（Makai）路线**
   - `FUUIN` 入口、路线差异、Boss 差异。
4. **4d：性能/稳定性**
   - 脏矩形、LUT 优化、内存池；1 小时 soak。

**可验证产出**

- `dist/th01-0.4.0-android-apk.apk`：Stage 1 从开局到 Boss 可玩；
- 后续 APK：全 20 关、双路线、结局可通；
- 60 fps（显示）/ 56.423 Hz（逻辑）；无崩溃、无明显卡顿；
- 关键画面与 NP2 截图对比（允许字形/音质差异）。

**风险**：EGC/GRCG 的像素级差异；Boss 逻辑的隐藏依赖；体量（这是最大阶段）。

---

### 阶段 5：存档与设置（1 周）

**任务**

- `REIIDEN.CFG`（11 字节）与分数/排行文件的读写；固定宽度结构体。
- 存档目录：`<external-files>/th01-save/`；数据目录只读。
- 设置项：惯用手、缩放模式（4:3/整数/拉伸）、音量、BGM/SE 开关、语言（如做中文）。
- 设置持久化（`port.cfg` 或 SharedPreferences）；首次启动迁移。
- 异常处理：损坏存档、只读目录、空间不足。

**可验证产出**

- 设置/分数/继续次数在重启、升级安装后保留；
- 原版 `.CFG` 可读（可选兼容）；损坏文件不崩溃；
- 数据目录只读时仍能存档。

**风险**：原版格式细节；Android 分区权限。

---

### 阶段 6：打包与发布（1 周）

**任务**

- 图标（原创：阴阳玉/鸟居，不使用 ZUN 原画）；自适应图标。
- 横屏 `sensorLandscape`、沉浸式、cutout、保持亮屏。
- Back→Esc、Home 暂停、音频焦点、生命周期。
- 单个完整 APK（内置数据 + OGG）；签名；`versionCode`；SHA256。
- 本地阶段不做“无数据包”；上 GitHub 前再单独剥离。
- 两台设备验证 + 1 小时 soak。

**可验证产出**

- `dist/th01-1.0.0-android-apk.apk` 可安装、启动、横屏、沉浸式、Back 正常；
- 所有本地资源（数据 + OGG）在 APK 内，能离线玩；
- 两台设备（OnePlus PJE110 / 荣耀 JDY-AN00，或用户现有设备）通过基本回归。

**风险**：图标版权；不同厂商的后台/手势策略。

---

## 7. 验证方法

| 方法 | 用途 |
| --- | --- |
| 桌面 golden frame | 用同一 Pi/GRZ/PF 解码器输出 PNG，和 NP2 截图对比 |
| NP2/Anex86 参考 | 原版运行、关键帧截图、音频录制（本地，仅自用） |
| 单元测试 | Pi/GRZ/GRF/BOS/PTN/PF 解码器；固定输入的 CRC/尺寸断言 |
| `adb logcat` | tag `th01`；启动、数据、GL、音频、生命周期 |
| `adb shell screencap` | 每个阶段的截图存档 |
| `dumpsys gfxinfo` / `simpleperf` | 帧时间、CPU |
| 触屏布局测试 | 左右手、横竖屏、安全区、按钮命中；参考 TH10 `test-touch.ps1` |
| soak | Stage 1 循环 1 小时；内存/句柄不增长 |

---

## 8. 风险登记

| # | 风险 | 概率 | 影响 | 缓解 |
| --- | --- | --- | --- | --- |
| R1 | **ReC98 没有 license**，公开分发 derivative 有法律风险 | 高 | 高 | 先私有；公开仓库只放我们的代码 + submodule 指向上游；发布前联系 nmlgc 确认许可 |
| R2 | **MDRV2 音乐**没有现成转换器，OGG 录音质量/循环点不确定 | 高 | 高 | 先按离线 OGG 做（用户已定）；用 NP2 多录几版，循环点做 A/B 对比；效果不可接受再评估原生 MDRV2 播放器 |
| R3 | EGC/GRCG 像素级差异导致画面不对 | 中 | 高 | 软件实现 + NP2 golden frame 对比；按阶段 4a/4b 逐步补 |
| R4 | 字体/文本效果与 PC-98 不一致 | 中 | 中 | FreeType + 系统 CJK；接受差异；提供点阵字体备选 |
| R5 | 16 位 `int`/enum/结构体布局差异 | 中 | 中 | 固定宽度类型 + `static_assert` + `-fshort-enums` 评估 |
| R6 | 性能：平面→RGBA 每帧全转 | 低 | 中 | LUT + 脏矩形；先全转，后优化 |
| R7 | 数据文件名 CP932 / 缺文件 | 中 | 低 | 启动器导入/规范化 + manifest + 明确报错 |
| R8 | CMake/Gradle 集成失败 | 低 | 低 | 已验证 CMake+Ninja；回退 ndk-build |
| R9 | 全 20 关 + 双路线体量 | 高 | 高 | 垂直切片；先地狱路线 Stage 1；按阶段交付 |
| R10 | 上游 ReC98 更新导致冲突 | 低 | 低 | pin commit；submodule；补丁分层 |

---

## 9. 目录与命令（建议）

```
东方灵异传安卓移植/
  PLAN.md
  DECISIONS.md
  README.md
  docs/
    01-调研-TH10模板.md
    02-调研-ReC98与TH01.md
    03-环境与工具链.md
    04-构建与产物.md
    05-Android后端.md
    06-验证记录.md
  tools/
    env.ps1
    fetch-deps.ps1
  scripts/
    common.ps1
    build-apk.ps1
    install-verify.ps1
    make-keystore.ps1
  th01-port/
    upstream/            # ReC98 debloated 子模块（pin 28d59d3）
    port/                # pc98 / master / data / audio 兼容层
    android-host/        # AndroidMain.cpp
    android/             # Gradle 工程（com.zun.th01）
  third_party/           # SDL2 2.32.10, FreeType 2.15.0（gitignore）
  game-data/th01/        # 用户自备数据（gitignore）
  secrets/               # keystore（gitignore）
  dist/                  # APK（gitignore）
```

命令（与 TH10 同构）：

```powershell
Set-ExecutionPolicy -Scope Process Bypass -Force
Set-Location "E:\水無月桐花\工程文件\东方灵异传安卓移植"
. .\tools\env.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File tools\fetch-deps.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\make-keystore.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\build-apk.ps1 -VersionName 0.1.0
```

CMake 直构（调试用）：

```powershell
cmake -S th01-port -B C:\th08-toolchain\build-target-th01 -G Ninja `
  -DCMAKE_MAKE_PROGRAM=C:\th08-toolchain\mingw64\bin\ninja.exe `
  -DCMAKE_TOOLCHAIN_FILE="$env:ANDROID_NDK_HOME\build\cmake\android.toolchain.cmake" `
  -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-26
cmake --build C:\th08-toolchain\build-target-th01
```

装机/推数据：

```powershell
adb install -r dist\th01-0.1.0-android-apk.apk
adb push <TH01原盘目录> /sdcard/Android/data/com.zun.th01/files/th01/
```

---

## 10. 用户已确认的决定（2026-11-08）

1. **先本地做，不上 GitHub**；ReC98 许可/公开分发问题后置。
2. **所有 BGM/SE 都内置离线 OGG**；不区分公开包/`-full` 包。
3. **一上来双路线、全 20 关**。
4. **直接做完整 APK**。
5. 触屏布局先参考隔壁 TH10/TH08；`lh-*` 没有找到，先按“左右面板 + Z/X/Bomb/Esc + 惯用手”实现。

仍然待你确认的只有：

- **`lh-*` 的路径/链接**：如果你想起在哪，请给一下；否则我按上面的布局做，实机手感不对再调。
- **语言**：默认保持日文原版；如果要中文翻译，请单独说（会影响文本层/字体/阶段 2）。
- **数据**：默认用本地 `(TH01-05).zip` 的日版 HDI 做开发测试，并把资源内置进本地 APK；上 GitHub 前再剥离。

---

## 11. 参考

- ReC98 仓库：<https://github.com/nmlgc/ReC98>
- ReC98 进度：<https://rec98.nmlgc.net/>
- ReC98 `debloated` 分支：<https://github.com/nmlgc/ReC98/tree/debloated>
- Pi 图像格式规范：<https://mooncore.eu/bunny/txt/pi-pic.htm>
- MDRV2 System（Vector）：<http://www.vector.co.jp/soft/dos/art/se018677.html>
- TH10 本机文档：`..\东方风神录安卓移植\docs\01-环境与工具链.md`、`03-构建与产物.md`、`05-Android后端实现.md`
- TH08 本机文档：`..\东方永夜抄安卓移植\docs\03-验证记录.md`（惯用手/安全区）
- TH07 移植报告：`..\东方妖妖梦安卓移植\东方妖妖梦移植报告.txt`
