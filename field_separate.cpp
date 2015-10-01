/*********************************************************************
* 	フィールド分離 for AviUtl
* 								ver. 0.01
* [2003]
* 	11/12:	公開(0.01)
* 
*********************************************************************/
#include <windows.h>
#include <math.h>
#include "filter.h"


//----------------------------
//	FILTER_DLL構造体
//----------------------------
char filtername[] = "フィールド分離";
char filterinfo[] = "フィールド分離 ver 0.01 by MakKi";
#define track_N 1
#if track_N
TCHAR *track_name[]   = { "間隔", };	// トラックバーの名前
int   track_default[] = {     8 , };	// トラックバーの初期値
int   track_s[]       = {     0 , };	// トラックバーの下限値
int   track_e[]       = {    20 , };	// トラックバーの上限値
#endif

#define check_N 2
#if check_N
TCHAR *check_name[]   = { "間隔を上下の色で埋める", "分離表示", };	// チェックボックス
int   check_default[] = { 1, 0 };	// デフォルト
#endif

#define tMRGN     0
#define cCOLOR    0
#define cDISP     1


FILTER_DLL filter = {
	FILTER_FLAG_EX_INFORMATION,
	NULL,NULL,      	// 設定ウインドウのサイズ
	filtername,     	// フィルタの名前
	track_N,        	// トラックバーの数
#if track_N
	track_name,     	// トラックバーの名前郡
	track_default,  	// トラックバーの初期値郡
	track_s,track_e,	// トラックバーの数値の下限上限
#else
	NULL,NULL,NULL,NULL,
#endif
	check_N,      	// チェックボックスの数
#if check_N
	check_name,   	// チェックボックスの名前郡
	check_default,	// チェックボックスの初期値郡
#else
	NULL,NULL,
#endif
	func_proc,   	// フィルタ処理関数
	NULL,NULL,   	// 開始時,終了時に呼ばれる関数
	NULL,        	// 設定が変更されたときに呼ばれる関数
	func_WndProc,	// 設定ウィンドウプロシージャ
	NULL,NULL,   	// システムで使用
	NULL,NULL,   	// 拡張データ領域
	filterinfo,  	// フィルタ情報
	NULL,			// セーブ開始直前に呼ばれる関数
	NULL,			// セーブ終了時に呼ばれる関数
	NULL,NULL,NULL,	// システムで使用
	NULL,			// 拡張領域初期値
	NULL,
};

/*********************************************************************
*	DLL Export
*********************************************************************/
EXTERN_C FILTER_DLL __declspec(dllexport) * __stdcall GetFilterTable( void )
{
	return &filter;
}

/*====================================================================
*	フィルタ処理関数
*===================================================================*/
BOOL func_proc(FILTER *fp,FILTER_PROC_INFO *fpip)
{
	int i,j;
	PIXEL_YC *src;
	PIXEL_YC *dst;
	int pitch = fpip->max_w * 2;

	dst = fpip->ycp_temp;

	// トップフィールド
	src = fpip->ycp_edit;
	for(i=fpip->h-fpip->h/2; i;--i){
		memcpy(dst,src,fpip->w*sizeof(PIXEL_YC));
		src += pitch;
		dst += fpip->max_w;
	}

	// 間隔
	if(!fp->check[cCOLOR]){	// 黒で塗りつぶす
		for(j=fp->track[tMRGN];j;--j){
			memset(dst,0,fpip->w*sizeof(PIXEL_YC));
			dst += fpip->max_w;
		}
	}
	else {	// 上下の色で塗りつぶす
		src -= pitch;
		for(i= fp->track[tMRGN]-fp->track[tMRGN]/2;i;--i){
			memcpy(dst,src,fpip->w*sizeof(PIXEL_YC));
			dst += fpip->max_w;
		}
		src = fpip->ycp_edit + fpip->max_w;
		for(i=fp->track[tMRGN]/2;i;--i){
			memcpy(dst,src,fpip->w*sizeof(PIXEL_YC));
			dst += fpip->max_w;
		}
	}

	// ボトムフィールド
	src = fpip->ycp_edit + fpip->max_w;
	for(i=fpip->h/2;i;--i){
		memcpy(dst,src,fpip->w*sizeof(PIXEL_YC));
		src += pitch;
		dst += fpip->max_w;
	}

	fpip->h += fp->track[tMRGN];

	src = fpip->ycp_edit;
	fpip->ycp_edit = fpip->ycp_temp;
	fpip->ycp_temp = src;

	return TRUE;
}

/*====================================================================
*	設定ウィンドウプロシージャ
*===================================================================*/
BOOL func_WndProc( HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, void *editp, FILTER *fp )
{
	switch(message){
		case WM_KEYUP:	// メインウィンドウへ送る
		case WM_KEYDOWN:
		case WM_MOUSEWHEEL:
			SendMessage(GetWindow(hwnd, GW_OWNER), message, wparam, lparam);
			break;
	}

	return FALSE;
}



