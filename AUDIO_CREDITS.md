# 声音来源与许可

从 `v0.1.0-preview.5` 起，`bin/sounds` 中的29个音频文件全部重新制作，未采样旧版游戏音乐或音效。以下录音素材经剪辑、滤波、混合、音量调整与循环处理；原始下载链接和SHA-256记录于 `audio-manifest.json`。

| 来源与作者 | 本版使用方式 | 原页面许可 |
| --- | --- | --- |
| [RPG Audio — Kenney](https://kenney.nl/assets/rpg-audio) | 木材敲击、布料、皮革、脚步等动作声的基础素材 | CC0；[随包原声明](licenses/Kenney-RPG-Audio-CC0.txt) |
| [Beach Ocean Waves — jasinski，经qubodup整理](https://opengameart.org/content/beach-ocean-waves) | 潮浪、游泳、落水与船体撞水 | CC0 |
| [Park ambiences — Thimras](https://opengameart.org/content/park-ambiences) | 树林鸟鸣、流水，以及装水与饮水中的水声层 | CC0 |
| [Rain (loopable) — Ylmir](https://opengameart.org/content/rain-loopable) | 连续雨声 | CC0 |
| 本项目新增的程序合成与编排 | 菜单主题、结尾变奏、提示音、火焰细节、夜鸟、呼吸、风与低沉雷声等 | 以CC0 1.0共享 |

上述CC0素材与本项目新增的合成音频可按 [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/) 使用，[完整文本](licenses/Audio-CC0-1.0.txt)随包提供。该说明仅涉及当前音频资产，不改变原作代码、图像或第三方运行库的许可。

菜单与结尾音乐为通过 `scripts/build_audio.py` 编排、合成的疏朗拨弦主题；它们不是原作音乐的翻录，也不是现场乐器录音。自然环境声使用上述录音，风、火焰与部分动作细节由程序合成叠加。

旧版音乐作者Heiko Plate的历史署名仍保留在 `README.upstream.md`，当前音频包不包含该旧版音乐。
