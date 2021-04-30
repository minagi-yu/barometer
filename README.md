# 気圧計キーホルダー

気圧と気温と、気圧から求めた高さを測れるキーホルダーです。

![気圧計キーホルダーの画像](https://repository-images.githubusercontent.com/363173965/40a20480-aa1f-11eb-9bff-a84e372922dd)

## ハードウェア

- マイコン：Microchip AVR microcontroller ATtiny402
- 気圧センサー：Infineon Technologies DPS368
- 液晶：秋月電子通商 AQM0802A
- ケース：タカチ PS-65
- 電池：CR2032
- その他：スイッチ・基板・コンデンサ・抵抗など

## 使い方

1. スイッチを押すと電源が入ります
2. 測定値表示画面の時は、1行目に現在の気圧、2行目に現在の気温が表示されます。
3. スイッチ短押しで、測定値表示と相対値表示が切り替わります。
4. 相対値表示画面の時は、1行目にリセット時からの差圧、2行目に気圧から計算した標高差が表示されます。
5. 相対値表示のときに長押しすると、現在の気圧でリセットされます。
6. 操作せずに30秒たったら電源が切れます。

## コンパイルの仕方

1. AVR toolchainをダウンロードして、AVRの開発環境を作ります。
2. [Microchip Packs Repository](https://packs.download.microchip.com/)からATtiny用のPackをダウンロードします。
3. MakefileのATPACKをダウンロードしたPackのパスへ書き換えます。
4. makeします
5. objディレクトリにelfファイルやhexファイルが出来上がります。
6. UPDIに対応した書き込み器でマイコンに書き込みます。
