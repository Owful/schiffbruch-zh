# 声音来源与许可

当前 `bin/sounds` 包含29个音频。`v0.1.0-preview.6` 的斧击音效由原作 `faellen.wav` 轻度清理而来，保留原节奏和重音，沿用原项目的CC BY 4.0；其他成品采用下列CC0录音或本项目新增合成音频。录音经剪辑、滤波、混合、音量调整与循环处理；原始下载链接、文件级许可和SHA-256记录于 `audio-manifest.json`。

| 来源与作者 | 本版使用方式 | 原页面许可 |
| --- | --- | --- |
| [RPG Audio — Kenney](https://kenney.nl/assets/rpg-audio) | 木材敲击、布料、皮革、脚步等动作声的基础素材 | CC0；[随包原声明](licenses/Kenney-RPG-Audio-CC0.txt) |
| [Beach Ocean Waves — jasinski，经qubodup整理](https://opengameart.org/content/beach-ocean-waves) | 潮浪、游泳、落水与船体撞水 | CC0 |
| [Park ambiences — Thimras](https://opengameart.org/content/park-ambiences) | 树林鸟鸣、流水，以及装水与饮水中的水声层 | CC0 |
| [Rain (loopable) — Ylmir](https://opengameart.org/content/rain-loopable) | 连续雨声 | CC0 |
| [7 Eating Crunches — Tito，经StarNinjas发布](https://opengameart.org/content/7-eating-crunches) | 轻度咀嚼录音，剪短并减弱高频 | CC0 |
| [Liquid Bottle Drink Set — qubodup](https://opengameart.org/content/liquid-bottle-drink-set) | 两次简短、较轻的真实吞咽 | CC0 |
| [Schiffbruch 原作斧击 — Dirk Plate / Lukas Dürrenberger](https://github.com/eXpl0it3r/Schiffbruch/blob/master/bin/sounds/faellen.wav) | 保留约0.615秒原节奏、固定音高、清理与加强木质重击 | CC BY 4.0；[完整许可](LICENSE) |
| 本项目新增的程序合成与编排 | 菜单主题、结尾变奏、提示音、火焰细节、夜鸟、呼吸、风与低沉雷声等 | 以CC0 1.0共享 |

上述标为CC0的素材与本项目新增的合成音频可按 [CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/) 使用，[完整文本](licenses/Audio-CC0-1.0.txt)随包提供。斧击成品适用CC BY 4.0。该说明不改变原作代码、图像或第三方运行库的许可。

菜单与结尾音乐为通过 `scripts/build_audio.py` 编排、合成的疏朗拨弦主题；它们不是原作音乐的翻录，也不是现场乐器录音。自然环境声使用上述录音，风、火焰与部分动作细节由程序合成叠加。

旧版音乐作者Heiko Plate的历史署名仍保留在 `README.upstream.md`，当前音频包不包含该旧版音乐。
