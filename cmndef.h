/*--------------------------------------------------------------------
*	共通マクロ
*-------------------------------------------------------------------*/
#ifndef ___CMNDEF_H
#define ___CMNDEF_H

#define ERR_SHORT_OF_MAXWIDTH  "最大画像サイズの幅が足りません\n\n"\
                                   "システムの設定を変更した後"\
                                   "AviUtlを再起動してください。"
#define ERR_SHORT_OF_MAXHEIGHT "最大画像サイズの高さが足りません\n\n"\
                                   "システムの設定を変更した後"\
                                   "AviUtlを再起動してください。"
#define ERR_SHORT_OF_MAXFRAME  "最大フレーム数が足りません\n\n"\
                                   "システムの設定を変更した後"\
                                   "AviUtlを再起動してください。"


#define FIELDSEPAR_NAME   "フィールド分離"
#define FIELDSEPAR_INFO   "フィールド分離 ver 0.02 by MakKi"
#define FIELDWEAVE_NAME   "フィールド結合"
#define FIELDWEAVE_INFO   "フィールド結合 ver 0.02 by MakKi"


// 設定パラメータ
#define tMRGN     0
#define tMODE     1
#define cCOLOR    0
#define cSWAP     1
#define cDISP     2

#define MODE_V    0
#define MODE_H    1
#define MODE_T    2
#define MODE_T2   3


#endif
