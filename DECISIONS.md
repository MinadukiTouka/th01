# DECISIONS

记录本项目的技术选择。状态：

- **已验证**：本机实际跑过/读过代码/解过文件，有证据。
- **已确认**：根据用户需求或既有约束确定。
- **待确认**：需要用户回答或后续 spike 才能定。
- **已否决**：考虑过但排除。

| ID | 决策 | 状态 | 理由 / 证据 |
| --- | --- | --- | --- |
| D-001 | 以 **TH10 风神录** 作为工程模板，不照抄 TH07/TH08 | 已验证 | 三套里只有 TH10 是 C++ + CMake/NDK + Gradle + SDL2 + GLES3；TH08 是 Rust/wgpu，TH07 只有 Termux 构建报告 |
| D-002 | 代码基线用 **ReC98 `debloated`**，commit `28d59d31331e27fcbe61d1b65bd8c0cd618757d8` | 已验证 | ReC98 README 明确写 “Ports should start from that branch”；该分支把 OP/REIIDEN/FUUIN 合并成多入口单二进制，清理了大量 `pascal`/段声明 |
| D-003 | TH01 采用 **单进程多入口**（`main_setup/main_op/main_main/main_cutscene`），不 `exec` 原 EXE | 已验证 | `debloated/th01/pc98/entry.cpp` 已有 `entrypoint_exec()`，Android 侧换成进程内切换即可 |
| D-004 | 构建用 **CMake + Ninja + NDK r27b + Gradle 9.8.2/AGP 9.4.1** | 已验证 | 本机最小 C+CXX CMake 工程 configure + build 成功；TH10 文档里的 0xC0000409 未复现，首次失败只是 PATH 缺 Ninja |
| D-005 | 若 Gradle 的 `externalNativeBuild.cmake` 集成失败，回退 TH10 的 ndk-build | 待确认 | 预留 fallback；必须在阶段 0 实测后记录 |
| D-006 | 显示管线：**4 平面 VRAM → RGBA8888 → SDL 纹理 → 4:3 letterbox** | 已确认 | 用户要求 “.GRP → RGBA8888 → GLES 纹理”；TH01 原生 640×400/16 色；SDL2 纹理在 Android 上走 GLES |
| D-007 | 帧率：逻辑按 **56.423 Hz**（PC-98 原生），显示按设备刷新率呈现 | 已验证 | ReC98 `CONTRIBUTING.md` 明确 “native 56.423 FPS rate”；TH10 的 60 Hz 不能直接套 |
| D-008 | 触屏：**虚拟键 + 惯用手镜像**，不用 TH08 的 analog 拖动 | 已确认 | TH01 是打砖块，原版只有左右 + Z/X；TH08 的 analog 拖动是为 STG 自机做的 |
| D-009 | Bomb 用**独立虚拟键直接置 `input_bomb`**，不模拟 Z/X 双击 | 待确认 | 原版靠 Z/X 双击；直接置变量更可靠，但等于改输入语义；先按独立 Bomb 键做，实机手感不对再改 |
| D-010 | 音频：**TH01 是 MDRV2（`.MDT`/`MDRV98.COM`），不是 PMD** | 已验证 | `MDRV98.COM` 字符串 “MDRV2 ver 3.4F”；`GAME.BAT` 先 `mdrv98 -M7`；`th01/snd/mdrv2.cpp` 走 INT F2h |
| D-011 | 音频：**全部内置离线 OGG**（BGM + SE），不区分公开/自用包 | 已确认（用户决定） | 用户 2026-11-08 明确“都内置离线 OGG”；MDRV2 没有现成转换器，OGG 需另行离线生成 |
| D-012 | 字体：**系统 CJK + FreeType**，不内置 PC-98 原版字体 | 已确认 | 法律约束；TH10 已验证系统 NotoSansCJK + FreeType TTC 可行；可备选 OFL 点阵字体 |
| D-013 | 数据：**本地开发期直接把原盘资源放进 APK/工作区**；公开仓库/公开 APK 再剥离 | 已确认（用户决定） | 用户“先在本地搞，github 最后”；本地不区分 full/公开包 |
| D-014 | 包名 `com.zun.th01`，`arm64-v8a`，`minSdk 26`，`targetSdk 36` | 已确认 | 用户指定包名；ABI/minSdk 沿用 TH10 |
| D-015 | 不改动隔壁三个移植工程 | 已确认 | 用户硬约束；本次调研只读 |
| D-016 | **先不上 GitHub**；ReC98 许可/公开分发问题后置 | 已确认（用户决定） | 用户“先在本地搞，github 是最后的事情，先不管” |
| D-017 | 源文件保持 **CP932** 编码，不整体转 UTF-8 | 已验证 | NDK clang 能编译 CP932 字符串字面量（仅 `-Winvalid-source-encoding` 警告）；整体转 UTF-8 会改变游戏内文本字节 |
| D-018 | 首版范围：**双路线、全 20 关**；不分阶段交付 | 已确认（用户决定） | 用户 2026-11-08 明确“一上来双路线全 20 关”“直接做完整 APK” |
| D-019 | 只做一个 **完整 APK**（内置本地数据 + OGG），不再做公开/`-full` 双包 | 已确认（用户决定） | 用户“不用区分 full 与公开包”“直接做完整 apk” |
| D-020 | 触屏布局先参考 TH10/TH08，并检查隔壁是否有 `lh-*` 遗留；找不到再问用户 | 已确认 | 用户在 `E:\水無月桐花` / `C:\Users\sophia` 浅层没有 `lh-*`；先按 TH10/TH08 原则做 |
| D-021 | PC-98 运行时：**1 MiB `dos_mem` + 2 页 × 4 平面 VRAM + 段表** | 已验证 | `port/pc98_runtime.cpp`；`MK_FP/peek/poke` 走段表，VRAM 平面前后页切换 |
| D-022 | Pi `.GRP` 解码器按 **delta table + RLE** 自写，不依赖 PiLoad | 已验证 | `port/pi.cpp`；`ST0.GRP` 已还原出红鸟居背景；delta 码前缀修正为 `1x/00x/010xx/011xxx` |
| D-023 | 文本：**FreeType + 系统 CJK 字体 + JIS→Unicode 表** | 已验证（Android 编译）/待真机 | `port/font.cpp`、`port/jis_table.cpp`；桌面 host 仍是空白 fallback |
| D-024 | Android 触屏：**Java 覆盖层**（← → Z X B Esc），native 只收 key group 位 | 已验证（编译） | `GameActivity.java`；惯用手从 `SharedPreferences("port","left_handed")` 读 |
| D-025 | APK 内置 `assets/th01/`，启动器解包到 `getExternalFilesDir/th01` | 已验证 | `LauncherActivity.java`；native 用 `SDL_AndroidGetExternalStoragePath()` |
| D-026 | 音频：**16 首离线 OGG + stb_vorbis 流式播放** | 已验证（桌面宿主解码） | `port/audio_ogg.cpp`；`bgm/NN.ogg` 内置；`.MDT` 文件名映射到曲目号；SE 暂静音 |
| D-027 | 桌面 host 用于无设备验证：跑 240 帧后 dump `frame.ppm` | 已验证 | `host/host_main.cpp`；Stage 1 截图 `docs/screenshots/stage1-desktop.png` |
| D-028 | Android 构建：**CMake + SDL2 2.32.10 + FreeType 2.15.0 + Gradle** | 已验证 | `android/app/build.gradle.kts`、`th01-port/CMakeLists.txt`；APK 4.67 MB |

## 关键事实（供后续查）

- ReC98 master HEAD：`b6ba5b0a529edbb31efdf8c0e939263804f8ee47`
- ReC98 debloated HEAD：`28d59d31331e27fcbe61d1b65bd8c0cd618757d8`
- TH01 代码量：25,612 行（去注释/空行）；144 `.cpp` / 96 `.hpp` / 17 `.h`
- TH01 的 master.lib 显式 include：51 个模块（含 data/bss）
- TH01 内联 asm：13 处 / 5 个文件
- `.GRP`：Pi “ZN” 变体，640×400，4bpp，palette 在 `0x12`，压缩流从 `0x42`
- `東方靈異.伝`：298,073 字节的 PF 归档，41 个条目（STAGE0-7.DAT、BOSS*.BOS、MIKO*.BOS、*.GRC、*.PTN）
- `MDRV98.COM`：12,483 字节，MDRV2 ver 3.4F，INT F2h
- 本机 CMake smoke test：`project(smoke C CXX)` + `-G Ninja` + NDK r27b → configure/build 通过
