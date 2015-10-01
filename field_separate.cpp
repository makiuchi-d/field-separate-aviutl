/*********************************************************************
* 	フィールド分離 for AviUtl
* 								ver. 0.02
* [2003]
* 	11/12:	公開(0.01)
* 	12/30:	MODE2(水平方向分離)を追加
* [2004]
* 	01/03:	MODE3(時間軸方向分離)を追加
* 	08/18:	大幅な手直し開始
* 			MODE4(時間軸方向分離 type2)を追加(未実装)
* 			トップとボトムの配置を入れ替えれるように(未実装)
* 	09/08:	手直し完了。あとは追加機能の実装。
* 	09/16:	ようやく全機能実装完了。
* 			でもコピペだらけの見苦しいソースに…(0.02)
* 
*********************************************************************/
/* TODO:
* 
* 	分離状態では縦が半分になるためフィルタの効果が強まってしまう
* 		→分離時に縦を倍にする（オプション）
* 
*/
#include <windows.h>
#include <math.h>
#include "filter.h"
#include "cmndef.h"


static BOOL separate_vertical_tff(FILTER *fp,FILTER_PROC_INFO *fpip);
static BOOL separate_vertical_bff(FILTER *fp,FILTER_PROC_INFO *fpip);
static BOOL separate_horizon_tff(FILTER *fp,FILTER_PROC_INFO *fpip);
static BOOL separate_horizon_bff(FILTER *fp,FILTER_PROC_INFO *fpip);
static BOOL separate_time_tff(FILTER *fp,FILTER_PROC_INFO *fpip);
static BOOL separate_time_bff(FILTER *fp,FILTER_PROC_INFO *fpip);
static BOOL separate_time2_tff(FILTER *fp,FILTER_PROC_INFO *fpip);
static BOOL separate_time2_bff(FILTER *fp,FILTER_PROC_INFO *fpip);

//----------------------------
//	グローバル変数
//----------------------------
const int   CACHE_FRAME       = 9;	// キャッシュするフレーム数
const char* NOT_FOUND_FIELDWV = "フィールド結合フィルタが見つかりません";

static BOOL (* const proc[4][2])(FILTER*,FILTER_PROC_INFO*) = {
	{ separate_vertical_tff , separate_vertical_bff },
	{ separate_horizon_tff  , separate_horizon_bff  },
	{ separate_time_tff     , separate_time_bff     },
	{ separate_time2_tff    , separate_time2_bff    }
};

typedef BOOL (*FUNCPROC)(FILTER*,FILTER_PROC_INFO*);
const FUNCPROC func = proc[0][0];

//----------------------------
//	FILTER_DLL構造体
//----------------------------
char filtername[] = FIELDSEPAR_NAME;
char filterinfo[] = FIELDSEPAR_INFO;
#define track_N 2
#if track_N
TCHAR *track_name[]   = { "間隔", "Mode" };	// トラックバーの名前
int   track_default[] = {     8 ,     0  };	// トラックバーの初期値
int   track_s[]       = {     0 ,     0  };	// トラックバーの下限値
int   track_e[]       = {    20 ,     3  };	// トラックバーの上限値
#endif

#define check_N 3

#if check_N
TCHAR *check_name[]   = { "間隔を接するピクセルの色で埋める",
                          "トップとボトムの配置を逆にする",
                          "分離表示", };	// チェックボックス
int   check_default[] = { 1, 0, 0 };	// デフォルト
#endif


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
	func_init,NULL,	// 開始時,終了時に呼ばれる関数
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
	return proc[fp->track[tMODE]][fp->check[cSWAP]](fp,fpip);
}


/*--------------------------------------------------------------------
*	垂直方向分離 (上Top)
*-------------------------------------------------------------------*/
static BOOL separate_vertical_tff(FILTER *fp,FILTER_PROC_INFO *fpip)
{
	int i,j;
	PIXEL_YC *src;
	PIXEL_YC *dst;
	int pitch = fpip->max_w * 2;
	int rowsize = fpip->w * sizeof(PIXEL_YC);

	dst = fpip->ycp_temp;
	fpip->ycp_temp = fpip->ycp_edit;
	fpip->ycp_edit = dst;

	// 最大高が足りない時
	if(fpip->max_h < fpip->h+fp->track[tMRGN]){
		MessageBox(fp->hwnd,ERR_SHORT_OF_MAXHEIGHT,filtername,MB_OK|MB_ICONERROR);
		return FALSE;
	}

	// トップフィールド
	src = fpip->ycp_temp;
	for(i=fpip->h-fpip->h/2; i;--i){
		memcpy(dst,src,rowsize);
		src += pitch;
		dst += fpip->max_w;
	}

	// 間隔
	if(fp->check[cCOLOR]){	// 上下の色で塗りつぶす
		src -= pitch;
		for(i=(fp->track[tMRGN]+1)>>1;i;--i){
			memcpy(dst,src,rowsize);
			dst += fpip->max_w;
		}
		src = fpip->ycp_temp + fpip->max_w;
		for(i=fp->track[tMRGN]/2;i;--i){
			memcpy(dst,src,rowsize);
			dst += fpip->max_w;
		}
	}
	else {	// 黒で塗りつぶす
		for(j=fp->track[tMRGN];j;--j){
			memset(dst,0,rowsize);
			dst += fpip->max_w;
		}
	}

	// ボトムフィールド
	src = fpip->ycp_temp + fpip->max_w;
	for(i=fpip->h/2;i;--i){
		memcpy(dst,src,rowsize);
		src += pitch;
		dst += fpip->max_w;
	}

	fpip->h += fp->track[tMRGN];

	return TRUE;
}

/*--------------------------------------------------------------------
*	垂直方向分離 (上Bottom)
*-------------------------------------------------------------------*/
static BOOL separate_vertical_bff(FILTER *fp,FILTER_PROC_INFO *fpip)
{
	int i,j;
	PIXEL_YC *src;
	PIXEL_YC *dst;
	int pitch = fpip->max_w * 2;
	int rowsize = fpip->w * sizeof(PIXEL_YC);
	dst = fpip->ycp_temp;
	fpip->ycp_temp = fpip->ycp_edit;
	fpip->ycp_edit = dst;

	// 最大高が足りない時
	if(fpip->max_h < fpip->h+fp->track[tMRGN]){
		MessageBox(fp->hwnd,ERR_SHORT_OF_MAXHEIGHT,filtername,MB_OK|MB_ICONERROR);
		return FALSE;
	}

	// ボトムフィールド
	src = fpip->ycp_temp + fpip->max_w;
	for(i=fpip->h/2;i;--i){
		memcpy(dst,src,rowsize);
		src += pitch;
		dst += fpip->max_w;
	}

	// 間隔
	if(fp->check[cCOLOR]){	// 上下の色で塗りつぶす
		src -= pitch;
		for(i=(fp->track[tMRGN]+1)>>1;i;--i){
			memcpy(dst,src,rowsize);
			dst += fpip->max_w;
		}
		src = fpip->ycp_temp;
		for(i=fp->track[tMRGN]/2;i;--i){
			memcpy(dst,src,rowsize);
			dst += fpip->max_w;
		}
	}
	else {	// 黒で塗りつぶす
		for(j=fp->track[tMRGN];j;--j){
			memset(dst,0,rowsize);
			dst += fpip->max_w;
		}
	}

	// トップフィールド
	src = fpip->ycp_temp;
	for(i=fpip->h-fpip->h/2; i;--i){
		memcpy(dst,src,rowsize);
		src += pitch;
		dst += fpip->max_w;
	}

	fpip->h += fp->track[tMRGN];

	return TRUE;
}

/*--------------------------------------------------------------------
*	水平方向分離 (左Top)
*-------------------------------------------------------------------*/
static BOOL separate_horizon_tff(FILTER *fp,FILTER_PROC_INFO *fpip)
{
	// 最大幅が足りない時
	if(fpip->max_w < fpip->w*2+fp->track[tMRGN]){
		MessageBox(fp->hwnd,ERR_SHORT_OF_MAXWIDTH,filtername,MB_OK|MB_ICONERROR);
		return FALSE;
	}

	PIXEL_YC *src = fpip->ycp_edit;
	PIXEL_YC *dst = fpip->ycp_temp;
	fpip->ycp_edit = dst;
	fpip->ycp_temp = src;

	int i,j;
	int dst_next = fpip->max_w - fpip->w - fp->track[tMRGN];
	int row_size = fpip->w * sizeof(PIXEL_YC);

	for(i=fpip->h/2; i; --i){
		memcpy(dst,src,row_size);	// トップフィールド
		dst += fpip->w;

		// 間隔を埋める
		if(fp->check[cCOLOR]){
			for(j=(fp->track[tMRGN]+1)>>1; j; --j)
				*dst++ = src[fpip->w-1];
			for(j=fp->track[tMRGN]/2; j; --j)
				*dst++ = src[fpip->max_w];
		}
		else{	// 黒で埋める
			memset(dst,0,fp->track[tMRGN]*sizeof(PIXEL_YC));
			dst += fp->track[tMRGN];
		}

		src += fpip->max_w;
		memcpy(dst,src,row_size);	// ボトムフィールド

		src += fpip->max_w;
		dst += dst_next;
	}
	if(fpip->h%2){
		memcpy(dst,src,row_size);
		dst += fpip->w;
		if(fp->check[cCOLOR]){
			for(i=(fp->track[tMRGN]+1)/2;i;--i)
				*dst++ = src[fpip->w-1];
			memcpy(dst,dst-fpip->max_w,row_size+(fp->track[tMRGN]/2*sizeof(PIXEL_YC)));
		}
		else
			memset(dst,0,row_size+fp->track[tMRGN]*sizeof(PIXEL_YC));
	}

	fpip->w = fpip->w*2 + fp->track[tMRGN];
	fpip->h = (fpip->h + 1) >>1;

	return TRUE;
}
/*--------------------------------------------------------------------
*	水平方向分離 (左Bottom)
*-------------------------------------------------------------------*/
static BOOL separate_horizon_bff(FILTER *fp,FILTER_PROC_INFO *fpip)
{

	// 最大幅が足りない時
	if(fpip->max_w < fpip->w*2+fp->track[tMRGN]){
		MessageBox(fp->hwnd,ERR_SHORT_OF_MAXWIDTH,filtername,MB_OK|MB_ICONERROR);
		return FALSE;
	}

	PIXEL_YC *src = fpip->ycp_edit;
	PIXEL_YC *dst = fpip->ycp_temp;
	fpip->ycp_edit = dst;
	fpip->ycp_temp = src;

	int i,j;
	int pitch = fpip->max_w * 2;
	int t     = fpip->max_w + fpip->w - 1;
	int row_size = fpip->w * sizeof(PIXEL_YC);
	int dst_next = fpip->max_w - fpip->w - fp->track[tMRGN];

	for(i=fpip->h/2; i; --i){

		// ボトムフィールド
		memcpy(dst,src+fpip->max_w,row_size);
		dst += fpip->w;

		// 間隔を埋める
		if(fp->check[cCOLOR]){
			for(j=(fp->track[tMRGN]+1)>>1; j; --j)
				*dst++ = src[t];
			for(j=fp->track[tMRGN]/2; j; --j)
				*dst++ = *src;
		}
		else{	// 黒で埋める
			memset(dst,0,fp->track[tMRGN]*sizeof(PIXEL_YC));
			dst += fp->track[tMRGN];
		}

		// トップフィールド
		memcpy(dst,src,row_size);

		src += pitch;
		dst += dst_next;
	}
	if(fpip->h%2){
		if(fp->check[cCOLOR]){
			memcpy(dst,dst-fpip->max_w,row_size+((fp->track[tMRGN]+1)/2)*sizeof(PIXEL_YC));
			dst += fpip->w+(fp->track[tMRGN]+1)/2;
			for(i=fp->track[tMRGN]/2;i;--i)
				*dst++ = *src;
		}
		else{
			memset(dst,0,row_size+fp->track[tMRGN]*sizeof(PIXEL_YC));
			dst += fpip->w + fp->track[tMRGN];
		}

		memcpy(dst,src,row_size);
	}

	fpip->w = fpip->w*2 + fp->track[tMRGN];
	fpip->h = (fpip->h + 1) >>1;

	return TRUE;
}

/*--------------------------------------------------------------------
*	時間軸方向分離 (前Top)
*-------------------------------------------------------------------*/
static BOOL separate_time_tff(FILTER *fp,FILTER_PROC_INFO *fpip)
{
	PIXEL_YC* src;
	int src_w,src_h,src_pitch;
	fp->exfunc->set_ycp_filtering_cache_size(fp,fpip->w,fpip->h,CACHE_FRAME,NULL);
	src = fp->exfunc->get_ycp_filtering_cache_ex(fp,fpip->editp,fpip->frame/2,&src_w,&src_h);

	if(src==NULL) return FALSE;

	PIXEL_YC* dst;
	dst = fpip->ycp_edit;
	src_pitch = src_w * 2;
	int row_size = src_w * sizeof(PIXEL_YC);

	if(fpip->frame%2 == 0){	// トップフィールド
		for(int i=(src_h+1)/2;i;--i){
			memcpy(dst,src,row_size);
			dst += fpip->max_w;
			src += src_pitch;
		}
	}
	else {	// ボトムフィールド
		src += src_w;
		for(int i=src_h/2;i;--i){
			memcpy(dst,src,row_size);
			dst += fpip->max_w;
			src += src_pitch;
		}
		if(fpip->h%2){	// 高さ奇数の時、最下ラインを加える
			if(fp->check[cCOLOR])
				memcpy(dst,dst-fpip->max_w,row_size);
			else
				memset(dst,0,row_size);
		}
	}
	fpip->h = (fpip->h +1)/2;

	return TRUE;
}
/*--------------------------------------------------------------------
*	時間軸方向分離 (前Bottom)
*-------------------------------------------------------------------*/
static BOOL separate_time_bff(FILTER *fp,FILTER_PROC_INFO *fpip)
{
	PIXEL_YC* src;
	int src_w,src_h,src_pitch;
	fp->exfunc->set_ycp_filtering_cache_size(fp,fpip->w,fpip->h,CACHE_FRAME,NULL);
	src = fp->exfunc->get_ycp_filtering_cache_ex(fp,fpip->editp,fpip->frame/2,&src_w,&src_h);

	PIXEL_YC* dst;
	dst = fpip->ycp_edit;
	src_pitch = src_w * 2;
	int row_size = src_w * sizeof(PIXEL_YC);

	if(fpip->frame%2 == 0){	// ボトムフィールド
		src += src_w;
		for(int i=src_h/2;i;--i){
			memcpy(dst,src,row_size);
			dst += fpip->max_w;
			src += src_pitch;
		}
		if(fpip->h%2){	// 高さ奇数の時、最下ラインを加える
			if(fp->check[cCOLOR])
				memcpy(dst,dst-fpip->max_w,row_size);
			else
				memset(dst,0,row_size);
		}
	}
	else {	// トップフィールド
		for(int i=(src_h+1)/2;i;--i){
			memcpy(dst,src,row_size);
			dst += fpip->max_w;
			src += src_pitch;
		}
	}
	fpip->h = (fpip->h +1)/2;

	return TRUE;
}

/*--------------------------------------------------------------------
*	時間軸方向分離 type2 (前Top)
*-------------------------------------------------------------------*/
static BOOL separate_time2_tff(FILTER *fp,FILTER_PROC_INFO *fpip)
{
	PIXEL_YC* src;
	PIXEL_YC* dst;

	if(fpip->frame < (fpip->frame_n-fp->track[tMRGN])/2){
		// トップフィールド
		int src_pitch = fpip->max_w * 2;
		int row_size  = fpip->w * sizeof(PIXEL_YC);
		src = dst = fpip->ycp_edit;
		dst += fpip->max_w;
		src += src_pitch;

		for(int i=(fpip->h+1)/2 -1; i; --i){
			memcpy(dst,src,row_size);
			dst += fpip->max_w;
			src += src_pitch;
		}
	}
	else if(fpip->frame >= (fpip->frame_n+fp->track[tMRGN])/2){
		// ボトムフィールド
		int src_w,src_h,src_pitch;

		fp->exfunc->set_ycp_filtering_cache_size(fp,fpip->w,fpip->h,CACHE_FRAME,NULL);
		src = fp->exfunc->get_ycp_filtering_cache_ex(fp,fpip->editp,fpip->frame-(fpip->frame_n+fp->track[tMRGN])/2,&src_w,&src_h);
		if(src==NULL) return FALSE;

		dst =  fpip->ycp_edit;
		src += src_w;
		src_pitch = src_w * 2;
		int row_size  = src_w * sizeof(PIXEL_YC);

		for(int i=fpip->h/2; i; --i){
			memcpy(dst,src,row_size);
			dst += fpip->max_w;
			src += src_pitch;
		}
		if(fpip->h%2){	// 高さ奇数の時、最下ラインを加える
			if(fp->check[cCOLOR])
				memcpy(dst,dst-fpip->max_w,row_size);
			else
				memset(dst,0,row_size);
		}
	}
	else{	// 間隔
		if(fp->check[cCOLOR]){
			int src_w,src_h,src_pitch;
			fp->exfunc->set_ycp_filtering_cache_size(fp,fpip->w,fpip->h,CACHE_FRAME,NULL);

			if(fpip->frame > fpip->frame_n/2){
				// １フレーム目のボトムフィールド
				src = fp->exfunc->get_ycp_filtering_cache_ex(fp,fpip->editp,0,&src_w,&src_h);
				if(src==NULL) return FALSE;

				src += src_w;
				src_pitch = src_w * 2;
				dst =  fpip->ycp_edit;
				int row_size = src_w*sizeof(PIXEL_YC);
				for(int i=fpip->h/2;i;--i){
					memcpy(dst,src,row_size);
					dst += fpip->max_w;
					src += src_pitch;
				}
				if(fpip->h%2){	// 高さ奇数の時、最下ラインを加える
					if(fp->check[cCOLOR])
						memcpy(dst,dst-fpip->max_w,row_size);
					else
						memset(dst,0,row_size);
				}
			}else{
				// 最終フレームのトップフィールド
				src = fp->exfunc->get_ycp_filtering_cache_ex(fp,fpip->editp,(fpip->frame_n-fp->track[tMRGN])/2 -1,&src_w,&src_h);
				if(src==NULL) return FALSE;

				src_pitch = src_w * 2;
				dst =  fpip->ycp_edit;
				int row_size = src_w*sizeof(PIXEL_YC);
				for(int i=(fpip->h+1)/2;i;--i){
					memcpy(dst,src,row_size);
					dst += fpip->max_w;
					src += src_pitch;
				}
			}
		}
		else{
			dst = fpip->ycp_edit;
			int row_size = fpip->w*sizeof(PIXEL_YC);
			for(int i=(fpip->h+1)/2; i; --i){
				memset(dst,0,row_size);
				dst += fpip->max_w;
			}
		}
	}

	fpip->h = (fpip->h+1) / 2;

	return TRUE;
}
/*--------------------------------------------------------------------
*	時間軸方向分離 type2 (前Bottom)
*-------------------------------------------------------------------*/
static BOOL separate_time2_bff(FILTER *fp,FILTER_PROC_INFO *fpip)
{
	PIXEL_YC* src;
	PIXEL_YC* dst;

	if(fpip->frame < (fpip->frame_n-fp->track[tMRGN])/2){
		// ボトムフィールド
		src = dst = fpip->ycp_edit;
		src += fpip->max_w;
		int src_pitch = fpip->max_w * 2;
		int row_size  = fpip->w*sizeof(PIXEL_YC);

		for(int i=fpip->h/2; i; --i){
			memcpy(dst,src,row_size);
			dst += fpip->max_w;
			src += src_pitch;
		}
		if(fpip->h%2){	// 高さ奇数の時、最下ラインを加える
			if(fp->check[cCOLOR])
				memcpy(dst,dst-fpip->max_w,row_size);
			else
				memset(dst,0,row_size);
		}
	}
	else if(fpip->frame >= (fpip->frame_n+fp->track[tMRGN])/2){
		// トップフィールド
		int src_w,src_h,src_pitch;

		fp->exfunc->set_ycp_filtering_cache_size(fp,fpip->w,fpip->h,CACHE_FRAME,NULL);
		src = fp->exfunc->get_ycp_filtering_cache_ex(fp,fpip->editp,fpip->frame-(fpip->frame_n+fp->track[tMRGN])/2,&src_w,&src_h);
		if(src==NULL) return FALSE;

		dst =  fpip->ycp_edit;
		src_pitch = src_w * 2;
		int row_size  = src_w*sizeof(PIXEL_YC);

		for(int i=fpip->h/2; i; --i){
			memcpy(dst,src,row_size);
			dst += fpip->max_w;
			src += src_pitch;
		}
	}
	else{	// 間隔
		if(fp->check[cCOLOR]){
			int src_w,src_h,src_pitch;
			fp->exfunc->set_ycp_filtering_cache_size(fp,fpip->w,fpip->h,CACHE_FRAME,NULL);

			if(fpip->frame > fpip->frame_n/2){	// 間隔後半
				// １フレーム目のトップフィールド
				src = fp->exfunc->get_ycp_filtering_cache_ex(fp,fpip->editp,0,&src_w,&src_h);
				if(src==NULL) return FALSE;

				int row_size = src_w*sizeof(PIXEL_YC);
				src_pitch = src_w * 2;
				dst =  fpip->ycp_edit;

				for(int i=(fpip->h+1)/2;i;--i){
					memcpy(dst,src,row_size);
					dst += fpip->max_w;
					src += src_pitch;
				}
			}
			else {	// 間隔前半
				// 最終フレームのボトムフィールド
				src = fp->exfunc->get_ycp_filtering_cache_ex(fp,fpip->editp,(fpip->frame_n-fp->track[tMRGN])/2 -1,&src_w,&src_h);
				if(src==NULL) return FALSE;

				int row_size = src_w*sizeof(PIXEL_YC);
				src += src_w;
				src_pitch = src_w * 2;
				dst =  fpip->ycp_edit;

				for(int i=fpip->h/2;i;--i){
					memcpy(dst,src,row_size);
					dst += fpip->max_w;
					src += src_pitch;
				}
				if(fpip->h%2){	// 高さ奇数の時、最下ラインを加える
					if(fp->check[cCOLOR])
						memcpy(dst,dst-fpip->max_w,row_size);
					else
						memset(dst,0,row_size);
				}
			}
		}
		else{
			dst = fpip->ycp_edit;
			for(int i=(fpip->h+1)/2; i; --i){
				memset(dst,0,fpip->w*sizeof(PIXEL_YC));
				dst += fpip->max_w;
			}
		}
	}

	fpip->h = (fpip->h+1) / 2;

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

/*====================================================================
*	開始時初期化
*===================================================================*/
BOOL func_init( FILTER *fp )
{
	FILTER* wvfp;

	for(int n=0;(wvfp=(FILTER*)fp->exfunc->get_filterp(n))!=NULL;n++){
		if(lstrcmp(wvfp->information,FIELDWEAVE_INFO)==0)
			return TRUE;
	}

	MessageBox(fp->hwnd,NOT_FOUND_FIELDWV,filtername,MB_OK | MB_ICONERROR);

	return FALSE;
}


