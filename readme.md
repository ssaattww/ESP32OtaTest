# 概要
- [Esp32Fota](https://github.com/chrisjoyce911/esp32FOTA)を用いてファームウェアを更新するデモ
- WiFi接続情報はNVS領域に保存しており、初回時はAPで起動したWebサーバでWiFiの接続を設定できます。
  - 次回以降は設定された接続情報に基づいて自動で接続します。
