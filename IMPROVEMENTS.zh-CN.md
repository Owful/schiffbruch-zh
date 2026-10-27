# 构建与验证说明

日期：2026-10-26。基于“交互修正版”源码归档继续开发，未合并离岛版人物、剧情或键盘移动系统。

## 源码构建

这是 Windows 原生 C++ 项目，使用 C++17、CMake 3.21 及以上、SFML 2.6.2。Windows SDK 提供 GDI 与窗口相关声明；运行时显示通过 SFML/OpenGL 完成，不依赖旧 DirectDraw 驱动。

在 Visual Studio 2022 的 x64 开发环境中，可以使用：

```powershell
cmake -S . -B build -A x64 -DBUILD_SHARED_LIBS=ON -DSFML_BUILD_NETWORK=OFF -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
cmake --install build --config Release --prefix install
```

首次配置需要获取 SFML；离线构建可设置 FETCHCONTENT_SOURCE_DIR_SFML 指向本地 SFML 2.6.2 源码。采用 CMake 4 时，CMAKE_POLICY_VERSION_MINIMUM=3.5 用于兼容 SFML 2.6.2 的构建文件。

工作区另提供 build-local.ps1，可复用上级 .tools 中已准备的 LLVM-MinGW 18.1.8、CMake、Ninja 与 SFML 源码。源码压缩包不重复附带大型编译工具。其他编译器构建时，应同时准备其对应的 C++ 运行库。

## 中文资源

localization.zh-CN.json 保存 141 条可读中文原文。src/Schiffbruch.rc 的字符串表使用宽字符串与 Unicode 转义，避免资源编译器以本地西文代码页误读 UTF-8。

tests/localization_expected.inc 为中文期望值。localization_test 实际加载编译后的 Schiffbruch.exe，逐条比较 Windows 字符串资源，而不是只检查源文件。修改翻译时应同步原文、资源表与期望值。

界面使用 Microsoft YaHei UI 与 Windows 字体回退；原有位图字体不再负责显示中文。透明文字层先填充正文背景，避免抗锯齿混入原来的粉色色键。

## 自动验证

十组检查覆盖：

1. expedition_rules：天气、水分、火源、作物、信号和扩展存档规则。
2. pixel_format：不同像素宽度、行跨度、颜色读写与调色。
3. expedition_integration：物资、消耗品、救援与进度存取联动。
4. frame_surface：裁剪、透明色、缩放、重叠复制和 GDI / 像素内存共享。
5. localization：最终可执行文件内全部 141 条中文文本的逐字核对。
6. experience：邻格自动交互、到达后执行、取消与目标复核、工具限制、透明叶片边缘容差、危险预警、一次生命保护及雨层裁剪。
7. pace_view：行动耗时与饥渴缩放、半分钟累计、PAC1尾部、缩放前后坐标互逆、鼠标锚点及界面区域保持。
8. crafting：全部配方产物与扣料、前置工具、动作解锁、已有工具防重复、探索物品不可凭空制造，以及制作补给不误用已有补给。
9. realtime：固定走时、停表、日长、微秒余量存取、入夜工作收尾、短话、实际寻路入口、不可达/远处庇护所与剩余工程材料。
10. audio_rules：请求音量与分类音量叠加、距离衰减、淡入淡出和动画触发去重。实际29个素材解码及混音器检查纳入原生--revision-check；音频分析结果见audio-validation.json，生成说明见docs/声音设计.md。

--revision-check 在隔离运行目录生成四张原生岛屿，实际调用World时间、资源、制作及存档接口。它还在100%、140%、180%倍率下，经过ModernUI的点击入口验证浆果和淡水目标。放置一份交互修正版旧存档为legacy-save.dat时，会验证旧档读取和再次保存。该命令会写测试存档，应只在隔离目录使用。

实机确认了邻格自动靠近浆果丛并进食、雨天显示和生命危急暂停。提示布局按实际中文字形测量后分配空间。原有菜单、背包、制作、设置及中文资源检查仍保留。

## 性能测量

程序支持内部 --benchmark 参数，隐藏窗口、关闭限帧，以完全揭示的地图绘制行动、建造、背包、制作、阵雨、风暴、暂停和中文对话八类场景，每类预热后测量 100 帧，输出 render-benchmark.json。该参数会创建测试岛屿并写入运行目录的存档，因此应在独立测试目录使用。

它测量当前机器的绘制耗时，不代表所有电脑的实际游玩帧率，也不覆盖完整通关。数据随发布验证记录附带。

## 主要实现

- FrameSurface：共享内存绘图表面，直接处理精灵拷贝、色键和裁剪。
- NativeDisplay：单次帧纹理更新、颜色转换、淡入淡出和比例适配。
- ModernUI：中文菜单、字体缓存、按钮、快捷键、模态对话与状态展示。
- Localization：Unicode 字符串资源读取与 UTF-8 转换。
- Expedition / ExpeditionRules：新增生存机制与兼容的扩展存档尾部。
- Interaction：精灵拾取、邻格目标选择、严格寻路、到达复核与无消耗紧急停止。
- 交互修正：通用互动只自动选择饮食目标；砍伐使用独立意图。水源支持地块与精灵两种点击判断，近距离对象按实际点击距离选择。独立进食、饮水命令不会互相替换。
- Survival / SurvivalRules：持续状态建议、低生命和入夜预警、日间最后一次保护与原因说明。
- WeatherEffects：按深度变化的透明雨层、风偏、水花与界面 / 迷雾排除。
- GameClock / ClockRules：现实0.4秒推进一个游戏分钟，白天4分48秒；菜单、阅读、预警、失焦、夜间停表。旧Action::AddTime调用不再推进时钟；真实时钟驱动作物、天气、救援与基础饥渴。超过250毫秒的单帧阻塞不补算整段卡顿。
- PaceRules：保留负向饥渴50%和缩放设置，旧PAC1尾部兼容。CLK1保存不足一分钟的微秒余量；CRF1保存尚未扣料的制作任务。
- Crafting：显式配方、前置工具、扣料、动作解锁和探索所得说明共用一份定义。制作水壶、绷带调用专用制作接口，避免使用快捷命令产生意外消费。
- 制作任务播放原作敲打、绑扎精灵，按3–5秒持续更新；暂停会停进度。完成时重新核对配方并扣料，取消无损，保存可恢复。
- 补给使用复用同一异步任务调度与CRF1尾部：饮水1.2秒、装水1.6秒、包扎1.9秒。动作期间不扣补给，完成前再次核对条件；可暂停、取消和保存恢复。移除场景内的制作进度框，保留底部停止入口。
- Nightfall：先等待有限的当前工作结束，再显示随机短话并严格寻路到附近完整帐篷/树屋；抵达后进入原作过夜动作。缺料取消、不可达、远处目标和无庇护所均有退路，不穿障碍传送。
- Construction：暂停地块的Rohstoff保存剩余用料，统一显示持有/还需/缺口；点击半成品优先恢复施工，不进入未成熟食物逻辑。
- Direct：开关快捷键只消费SFML按下事件，失焦/恢复清空队列，避免系统全局按键在返回窗口时误切网格。水面动画始终开启，A键不再关闭水流。
- SceneView：仅缩放世界视口，界面维持固定坐标；绘制、鼠标点击、悬停与拖动使用同一裁切变换，放大围绕鼠标位置。制作列表内滚轮只浏览条目。

内部体验预览可以使用 --preview-rain 和 --preview-danger（仅在独立测试目录运行）。SCHIFFBRUCH_QA_PREVIEW 构建选项仅供自动进入预览与 F2 注入风险状态；发布配置为 OFF。普通游戏没有这些测试状态或测试存档。

原作角色、地形与物件素材仍被保留；本次画面改进主要涉及显示管线、配色与全部操作界面。
