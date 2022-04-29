# nekotoiruka/contributed/hello-aquestalk

ESP32向けの超軽量音声合成ライブラリ、AquesTalk ESP32をModdableから使ってみたサンプルです。

Moddable with [AquesTalk ESP32](https://www.a-quest.com/products/aquestalk_esp32.html), a tiny text-to-speech engine for ESP32. 
<br />
```JavaScript
import TTS from 'tts-aquestalk'

//TTS.speak(aquesText, speed, volume)
// aquesText : https://www.a-quest.com/demo/index.html
//             音声記号列 for AquesTalk pico
// speed : 50-300 (default 100)
// volume : 0-256
TTS.speak("yukku'ri/_shiteitte'ne. ", 80, 64);
```


---
# Acknowledgements

[こちらのissue](https://github.com/Moddable-OpenSource/moddable/issues/487)を参考にさせていただきました。<br />
meganetaaan様、ありがとうございます！

I appreciate the [following issue](https://github.com/Moddable-OpenSource/moddable/issues/487) as a reference.




---
# Requirement

* AquesTalk-ESP32([aquestalk-esp32_0221.zip](https://www.a-quest.com/download.html#a-etc))


---
# Installation

[こちらのコミット](https://github.com/nekotoiruka/moddable/commit/62165a3d50a924c21fccebeead2860cf19456b4f)も合わせてご確認ください。

1. aquestalk-esp32_0221.zipを展開します。
    * 以下のファイルを $(MODDABLE)/contributed/hello-aquestalk フォルダにコピーします。
        * aquestalk-esp32/src/aquestalk.h（ライブラリ定義ヘッダ）
        * aquestalk-esp32/src/esp32/libaquestalk.a（スタティックライブラリ実体）

2. libaquestalk.aはそのままではModdablenに取り込めないため、同一フォルダ内に展開しておきます。
    * tools/mcconfig/make.esp32.mk 45行目以降に定義されているファイルが含まれていることを確認します。


---
# Usage

いつものように動かします。

mac / m5stack core2

```bash
cd /path/to/moddable/contributed/hello-aquestalk
UPLOAD_PORT=/dev/cu.wchusbserialXXXXXXXXXXXX mcconfig -d -m -p esp32/m5stack_core2
```
---
# Unresolved Issues...

* aquestalk.hの59行目をコメントアウトしています（型定義？のエラーが発生するため）
* [ライセンスキー](https://store.a-quest.com/items/10524168)をjavascript側から初期化時に渡したかったけどうまく実装できてません・・・