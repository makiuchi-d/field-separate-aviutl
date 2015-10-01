/*********************************************************************
* 	フィールド結合 for AviUtl
* 								ver. 0.01
* [2003]
* 	11/12:	公開(0.01)
* 
*********************************************************************/
#include <windows.h>
#include <math.h>
#include "filter.h"


//----------------------------
//	グローバル変数
//----------------------------
static FILTER* sepfp;
const  char* FIELDSEP_NAME     = "フィールド分離";
const  char* NOT_FOUND_FIELDSP = "フィールド分離フィルタが見つかりません";

//----------------------------
//	FILTER_DLL構造体
//----------------------------
char filtername[] = "フィールド結合";
char filterinfo[] = "フィールド結合 ver 0.01 by MakKi";
#define track_N 0
#if track_N
TCHAR *track_name[]   = {  };	// トラックバーの名前
int   track_default[] = {  };	// トラックバーの初期値
int   track_s[]       = {  };	// トラックバーの下限値
int   track_e[]       = {  };	// トラックバーの上限値
#endif

#define check_N 0
#if check_N
TCHAR *check_name[]   = {  };	// チェックボックス
int   check_default[] = {  };	// デフォルト
#endif

#define tMRGN     0
#define cCOLOR    0
#define cDISP     1


FILTER_DLL filter = {
	FILTER_FLAG_ALWAYS_ACTIVE |	// フィルタを常にアクティブにします
	FILTER_FLAG_NO_CONFIG |	// 設定ウィンドウを表示しないようにします
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
	func_init,NULL,	// 開始時,終了時に呼ばれる関数
	NULL,        	// 設定が変更されたときに呼ばれる関数
	NULL,			// 設定ウィンドウプロシージャ
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
	if(!sepfp) return FALSE;
	if(!fp->exfunc->is_filter_active(sepfp)) return TRUE;
	if(sepfp->check[cDISP])                  return TRUE;

	int h     = fpip->h-sepfp->track[tMRGN];

	PIXEL_YC* top    = fpip->ycp_edit;
	PIXEL_YC* bottom = fpip->ycp_edit + fpip->max_w * (fpip->h - h/2);

	PIXEL_YC* dst = fpip->ycp_temp;

	for(int i=h/2;i;--i){
		memcpy(dst,top,fpip->w*sizeof(PIXEL_YC));
		dst += fpip->max_w;
		memcpy(dst,bottom,fpip->w*sizeof(PIXEL_YC));
		dst += fpip->max_w;
		top    += fpip->max_w;
		bottom += fpip->max_w;
	}

	fpip->h = h;

	top = fpip->ycp_edit;
	fpip->ycp_edit = fpip->ycp_temp;
	fpip->ycp_temp = top;

	return TRUE;
}

/*====================================================================
*	開始時初期化
*===================================================================*/
BOOL func_init( FILTER *fp )
{
	sepfp = NULL;
	for(int n=0;(sepfp=(FILTER*)fp->exfunc->get_filterp(n))!=NULL;n++){
		if(lstrcmp(sepfp->name,FIELDSEP_NAME)==0)
			return TRUE;
	}

	MessageBox(fp->hwnd,NOT_FOUND_FIELDSP,filtername,MB_OK | MB_ICONERROR);

	return FALSE;
}



