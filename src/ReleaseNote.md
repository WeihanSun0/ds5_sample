# TL10向け　DS5 upsampling
## Version 1.0
* 日付 22/7/12
* 変更点
  * API 
    * 関数名変更：run_pc() -> run()
    * パラメータ変更：set_upsampling_parameters()のパラメータをSpotとFloodそれぞれ２つに設定
    * filter_by_conf()消し
  * 機能
    * spotのみの対応
    * spotとfloodの合わせの対応
    * 処理時間の短縮（flood単体約 10 ms、spot+flood約 24 ms 測定環境AMD 3700X, Windows 10)
    * NaNの出力
    * 信頼度：仮に0か1の出力、次回変更。

* 使用
  * sample.cppをご参照ください。
## Version 1.1
* 日付 22/8/5
* 変更点
  * 機能
    * flood,spot,flood+spotのサポート
    * 高速化 (Intel i5 1.6GHzで20~40FPS)
    * 信頼度（Confidence）の出力（0~1）
    * flood upsampling結果のみNaN対応
  * API 
    * get_default_upsampling_parametersの追加
    * set_upsampling_parametersの追加
    * get_default_preprocessing_parametersの追加
    * set_preprocessing_paramtersの追加
    * パラメータの追加。ユーザに設定させるパラメータを上記関数で設定
    * get_flood_depthMap,get_spot_depthMapの追加（depthmap 出力用）
    * 

* 使用方法
  * sample.cppをご参照ください。

## Verion 1.1.1
* 日付 22/8/8
* 変更点
  * DSViewerチームの指摘により、修正
    * Upsampling classのメンバー変数m_xxx_のフォーマットに修正
    * ヘッダファイルのget_default_upsampling_parametersとset_upsampling_parametersにコメントの追加
    * パラメータを構造体にまとめ、設定と取得の際に使用することに修正
    * cannyの閾値をthresh1とthresh2に統一