# 第三方组件与源码

本游戏在原作素材和代码之外，随 Windows 包提供以下可独立替换的运行库。它们各自适用其原有许可；仓库根目录的 CC BY 4.0 声明不覆盖这些第三方库。

| 组件 | 用途与许可 | 许可与源码 |
| --- | --- | --- |
| SFML 2.6.2 | 图形、窗口、音频；SFML 使用 zlib/png 许可 | [随包许可](licenses/SFML-license.md)、[上游源码](https://github.com/SFML/SFML/tree/2.6.2) |
| OpenAL Soft 1.18.1 | `openal32.dll`，由 SFML 音频使用；GNU Library GPL 2.0 或后续版本 | [完整许可](licenses/OpenAL-Soft-LGPL-2.0.txt)、[官方标签](https://github.com/kcat/openal-soft/tree/openal-soft-1.18.1)、同一 Release 中的 `OpenAL-Soft-1.18.1-source.zip` |
| LLVM MinGW 运行库 | `libc++.dll` 等；Apache 2.0 with LLVM Exceptions | [随包许可](licenses/LLVM-runtime-LICENSE.txt)、[LLVM 项目](https://github.com/llvm/llvm-project) |

`openal32.dll` 为独立 DLL，可由使用者替换。随包的 OpenAL Soft 源码档案取自官方 `openal-soft-1.18.1` 标签，包含构建文件与原始 `COPYING`。本项目未修改该 DLL 的源码。

原作图像及声音来自 [eXpl0it3r/Schiffbruch](https://github.com/eXpl0it3r/Schiffbruch)，文件保持原样。原项目将游戏署名给 Dirk Plate、C++ 移植署名给 Lukas Dürrenberger，并在说明中将音乐署名给 Heiko Plate；[原说明](README.upstream.md)随包保留。本版修改的范围见 [更新记录](CHANGELOG.md)。
