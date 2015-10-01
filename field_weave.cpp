/*********************************************************************
* 	フィールド結合 for AviUtl
* 								ver. 0.02
* [2003]
* 	11/12:	公開(0.01)
* 	12/30:	MODE2(水平方向分離)を追加
* [2004]
* 	01/03:	MODE3(時間軸方向分離)を追加
* 	08/18:	大幅な手直し開始。
* 			MODE4(時間軸方向分離 type2)を追加(未実装)
* 			トップとボトムの配置を入れ替えれるように(未実装)
* 	09/08:	手直し完了。あとは追加機能の実装。
* 	09/17:	全ての機能を実装完了。(0.02)
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


//----------------------------
//	class
//----------------------------

/*--------------------------------------------------------------------
* 	class DoubleFrame
* 		フレーム数調節
*/
class DoubleFrame {	
	bool flag;
	int  margin;

	bool CheckFrameMax(FILTER* fp,int frame_n){	// 最大フレーム数チェック
		SYS_INFO si;
		if(fp->exfunc->get_sys_info(NULL,&si)==FALSE)
			return false;
		if(si.max_frame < frame_n){	// 足りない
			MessageBox(fp->hwnd,ERR_SHORT_OF_MAXFRAME,fp->name,MB_OK|MB_ICONERROR);
			return false;
		}
		return true;
	}

public:
	DoubleFrame(void){ flag = false; margin = 0; }

	bool Double(FILTER* fp,FILTER_PROC_INFO* fpip,int mrgn =0)
	{
		int frame_n = 0;
		if(!flag){
			frame_n = fp->exfunc->get_frame_n(fpip->editp) * 2 + mrgn;
		}
		else if(margin!=mrgn){
			frame_n = fp->exfunc->get_frame_n(fpip->editp) - margin + mrgn;
		}
		if(frame_n && CheckFrameMax(fp,frame_n)){
			fp->exfunc->set_frame_n(fpip->editp,frame_n);
			flag = true;
			margin = mrgn;
		}

		return flag;
	}
	bool UnDouble(FILTER* fp,FILTER_PROC_INFO* fpip)
	{
		if(flag==false) return true;
		flag = false;
		int frame_n = fp->exfunc->get_frame_n(fpip->editp) - margin;
		frame_n = (frame_n + 1) /2;
		fp->exfunc->set_frame_n(fpip->editp,frame_n);
		margin = 0;

		if(fpip->frame>=frame_n)
			fp->exfunc->set_frame(fpip->editp,frame_n-1);
		return true;
	}
};

/*--------------------------------------------------------------------
* 	class DoublePos
* 		時間軸分離(type1)で倍加時の選択･表示フレーム更新
*/
class DoublePos{
	bool flag;

public:
	DoublePos(void){ flag = false; }

	bool Double(FILTER* fp,FILTER_PROC_INFO* fpip)
	{
		if(flag) return true;

		int s,e;
		if(!fp->exfunc->get_select_frame(fpip->editp,&s,&e))
			return false;

		fp->exfunc->set_select_frame(fpip->editp,s*2,e*2+1);
		fp->exfunc->set_frame(fpip->editp,fpip->frame*2);
		flag = true;

		return true;
	}
	bool UnDouble(FILTER* fp,FILTER_PROC_INFO* fpip)
	{
		if(!flag) return true;

		int s,e;
		if(!fp->exfunc->get_select_frame(fpip->editp,&s,&e))
			return false;

		fp->exfunc->set_select_frame(fpip->editp,s/2,e/2);
		fp->exfunc->set_frame(fpip->editp,fpip->frame/2);
		flag = false;

		return true;
	}
};


//----------------------------
//	プロトタイプ
//----------------------------
static BOOL weave_vertical_tff(FILTER *fp,FILTER_PROC_INFO *fpip);
static BOOL weave_vertical_bff(FILTER *fp,FILTER_PROC_INFO *fpip);
static BOOL weave_horizon_tff(FILTER *fp,FILTER_PROC_INFO *fpip);
static BOOL weave_horizon_bff(FILTER *fp,FILTER_PROC_INFO *fpip);
static BOOL weave_time_tff(FILTER *fp,FILTER_PROC_INFO *fpip);
static BOOL weave_time_bff(FILTER *fp,FILTER_PROC_INFO *fpip);
static BOOL weave_time2_tff(FILTER *fp,FILTER_PROC_INFO *fpip);
static BOOL weave_time2_bff(FILTER *fp,FILTER_PROC_INFO *fpip);

//----------------------------
//	グローバル変数
//----------------------------
static FILTER* sepfp;
const  char* NOT_FOUND_FIELDSP = "フィールド分離フィルタが見つかりません";
const  int   CACHE_FRAME       = 9;	// キャッシュするフレーム数

static BOOL (* const proc[4][2])(FILTER*,FILTER_PROC_INFO*) = {
	{ weave_vertical_tff , weave_vertical_bff },
	{ weave_horizon_tff  , weave_horizon_bff  },
	{ weave_time_tff     , weave_time_bff     },
	{ weave_time2_tff    , weave_time2_bff    }
};


//----------------------------
//	FILTER_DLL構造体
//----------------------------
char filtername[] = FIELDWEAVE_NAME;
char filterinfo[] = FIELDWEAVE_INFO;
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
* 
* 	フレーム数関連
* 		MODE_T,MODE_T2の時
* 			分離表示→フレーム数2倍
* 			通常    →2倍してフィルタかけて戻す
* 
* 		MODE_Tの分離表示時のみ選択範囲,表示位置を2倍
* 
*===================================================================*/
static DoublePos   dblpos;
static DoubleFrame dblframe;

BOOL func_proc(FILTER *fp,FILTER_PROC_INFO *fpip)
{
	if(!sepfp) return FALSE;

	if(!fp->exfunc->is_filter_active(sepfp)){	// フィルタ無効時
		dblpos.UnDouble(fp,fpip);
		dblframe.UnDouble(fp,fpip);
		return FALSE;
	}

	// 分離表示時
	if(sepfp->check[cDISP]){
		switch(sepfp->track[tMODE]){
			case MODE_V:
			case MODE_H:
				dblpos.UnDouble(fp,fpip);
				dblframe.UnDouble(fp,fpip);
				break;
			case MODE_T:
				dblframe.Double(fp,fpip);
				dblpos.Double(fp,fpip);
				break;
			case MODE_T2:
				dblpos.UnDouble(fp,fpip);
				dblframe.Double(fp,fpip,sepfp->track[tMRGN]);
				break;
		}
		return TRUE;
	}

	dblpos.UnDouble(fp,fpip);

	return proc[sepfp->track[tMODE]][sepfp->check[cSWAP]](fp,fpip);
}


/*--------------------------------------------------------------------
*	垂直方向結合 (Top First)
*-------------------------------------------------------------------*/
static BOOL weave_vertical_tff(FILTER*fp,FILTER_PROC_INFO *fpip)
{
	dblframe.UnDouble(fp,fpip);

	int h     = fpip->h - sepfp->track[tMRGN];

	PIXEL_YC* top    = fpip->ycp_edit;
	PIXEL_YC* bottom = fpip->ycp_edit + fpip->max_w * (fpip->h - h/2);

	PIXEL_YC* dst  = fpip->ycp_temp;
	fpip->ycp_temp = fpip->ycp_edit;
	fpip->ycp_edit = dst;

	int row_size = fpip->w * sizeof(PIXEL_YC);

	for(int i=h/2;i;--i){
		memcpy(dst,top,row_size);
		dst += fpip->max_w;

		memcpy(dst,bottom,row_size);
		dst += fpip->max_w;

		top    += fpip->max_w;
		bottom += fpip->max_w;
	}

	fpip->h = h;

	return TRUE;
}
/*--------------------------------------------------------------------
*	垂直方向結合 (Bottom First)
*-------------------------------------------------------------------*/
static BOOL weave_vertical_bff(FILTER*fp,FILTER_PROC_INFO *fpip)
{
	dblframe.UnDouble(fp,fpip);

	int h     = fpip->h - sepfp->track[tMRGN];

	PIXEL_YC* bottom = fpip->ycp_edit;
	PIXEL_YC* top    = fpip->ycp_edit + fpip->max_w * (fpip->h -h/2);

	PIXEL_YC* dst  = fpip->ycp_temp;
	fpip->ycp_temp = fpip->ycp_edit;
	fpip->ycp_edit = dst;

	int row_size = fpip->w * sizeof(PIXEL_YC);

	for(int i=h/2;i;--i){
		memcpy(dst,top,row_size);
		dst += fpip->max_w;

		memcpy(dst,bottom,row_size);
		dst += fpip->max_w;

		top    += fpip->max_w;
		bottom += fpip->max_w;
	}

	fpip->h = h;

	return TRUE;
}

/*--------------------------------------------------------------------
*	水平方向結合 (Top First)
*-------------------------------------------------------------------*/
static BOOL weave_horizon_tff(FILTER*fp,FILTER_PROC_INFO *fpip)
{
	dblframe.UnDouble(fp,fpip);

	int h = fpip->h * 2;
	if(h > fpip->max_h){
		MessageBox(fp->hwnd,ERR_SHORT_OF_MAXHEIGHT,filtername,MB_OK|MB_ICONERROR);
		return FALSE;
	}

	PIXEL_YC *src;
	PIXEL_YC *dst;
	fpip->w = (fpip->w - sepfp->track[tMRGN])/2;
	int pitch = fpip->w + sepfp->track[tMRGN];
	int row_size = fpip->w * sizeof(PIXEL_YC);

	src = fpip->ycp_edit;
	dst = fpip->ycp_temp;
	fpip->ycp_edit = dst;
	fpip->ycp_temp = src;

	for(int i=fpip->h;i; --i){
		memcpy(dst,src,row_size);
		dst += fpip->max_w;
		memcpy(dst,src+pitch,row_size);
		dst += fpip->max_w;
		src += fpip->max_w;
	}

	fpip->h = h;

	return TRUE;
}
/*--------------------------------------------------------------------
*	水平方向結合 (Bottom First)
*-------------------------------------------------------------------*/
static BOOL weave_horizon_bff(FILTER*fp,FILTER_PROC_INFO *fpip)
{
	dblframe.UnDouble(fp,fpip);

	int h = fpip->h * 2;
	if(h > fpip->max_h){
		MessageBox(fp->hwnd,ERR_SHORT_OF_MAXHEIGHT,filtername,MB_OK|MB_ICONERROR);
		return FALSE;
	}

	PIXEL_YC *src;
	PIXEL_YC *dst;
	fpip->w = (fpip->w - sepfp->track[tMRGN])/2;
	int pitch = fpip->w + sepfp->track[tMRGN];
	int row_size = fpip->w * sizeof(PIXEL_YC);

	src = fpip->ycp_edit;
	dst = fpip->ycp_temp;
	fpip->ycp_edit = dst;
	fpip->ycp_temp = src;

	for(int i=fpip->h; i; --i){
		memcpy(dst,src+pitch,row_size);
		dst += fpip->max_w;
		memcpy(dst,src,row_size);
		dst += fpip->max_w;
		src += fpip->max_w;
	}

	fpip->h = h;

	return TRUE;
}

/*--------------------------------------------------------------------
*	時間軸方向結合 (Top First)
*-------------------------------------------------------------------*/
static BOOL weave_time_tff(FILTER *fp,FILTER_PROC_INFO *fpip)
{
	int h = fpip->h * 2;
	if(h > fpip->max_h){
		MessageBox(fp->hwnd,ERR_SHORT_OF_MAXHEIGHT,filtername,MB_OK|MB_ICONERROR);
		return FALSE;
	}

	if(!dblframe.Double(fp,fpip))	// フレーム数倍加
		return FALSE;

	int src_w,src_h;

	fp->exfunc->set_ycp_filtering_cache_size(fp,fpip->w,fpip->h,CACHE_FRAME,NULL);
	PIXEL_YC* src1 = fp->exfunc->get_ycp_filtering_cache_ex(fp,fpip->editp,fpip->frame*2,&src_w,&src_h);	// top
	PIXEL_YC* src2 = fp->exfunc->get_ycp_filtering_cache_ex(fp,fpip->editp,fpip->frame*2+1,NULL,NULL);	// bottom
	PIXEL_YC* dst  = fpip->ycp_edit;

	if(src1==NULL||src2==NULL) return FALSE;

	int row_size = src_w * sizeof(PIXEL_YC);

	for(int i=src_h;i;--i){
		// トップフィールド
		memcpy(dst,src1,row_size);
		dst += fpip->max_w;

		// ボトムフィールド
		memcpy(dst,src2,row_size);
		src2 += src_w;

		dst += fpip->max_w;
		src1 += src_w;
	}

	fpip->h = h;

	// 総フレーム数半減
	dblframe.UnDouble(fp,fpip);

	return TRUE;
}
/*--------------------------------------------------------------------
*	時間軸方向結合 (Bottom First)
*-------------------------------------------------------------------*/
static BOOL weave_time_bff(FILTER *fp,FILTER_PROC_INFO *fpip)
{
	int h = fpip->h * 2;
	if(h > fpip->max_h){
		MessageBox(fp->hwnd,ERR_SHORT_OF_MAXHEIGHT,filtername,MB_OK|MB_ICONERROR);
		return FALSE;
	}

	if(!dblframe.Double(fp,fpip))	// フレーム数倍加
		return FALSE;

	int src_w,src_h;

	fp->exfunc->set_ycp_filtering_cache_size(fp,fpip->w,fpip->h,CACHE_FRAME,NULL);
	PIXEL_YC* src1 = fp->exfunc->get_ycp_filtering_cache_ex(fp,fpip->editp,fpip->frame*2,&src_w,&src_h);	// bottom
	PIXEL_YC* src2 = fp->exfunc->get_ycp_filtering_cache_ex(fp,fpip->editp,fpip->frame*2+1,NULL,NULL);	// top
	PIXEL_YC* dst  = fpip->ycp_edit;

	if(src1==NULL||src2==NULL) return FALSE;

	int row_size = src_w * sizeof(PIXEL_YC);

	for(int i=src_h;i;--i){
		// トップフィールド
		memcpy(dst,src2,row_size);
		dst += fpip->max_w;

		// ボトムフィールド
		memcpy(dst,src1,row_size);
		src2 += src_w;

		dst += fpip->max_w;
		src1 += src_w;
	}

	fpip->h = h;

	dblframe.UnDouble(fp,fpip);

	return TRUE;
}

/*--------------------------------------------------------------------
*	時間軸方向結合 type2 (Top First)
*-------------------------------------------------------------------*/
static BOOL weave_time2_tff(FILTER *fp,FILTER_PROC_INFO *fpip)
{
	// 高さ倍加
	int h = fpip->h * 2;
	if(h > fpip->max_h){
		MessageBox(fp->hwnd,ERR_SHORT_OF_MAXHEIGHT,filtername,MB_OK|MB_ICONERROR);
		return FALSE;
	}

	if(!dblframe.Double(fp,fpip,sepfp->track[tMRGN]))	// フレーム数倍加
		return FALSE;

	fp->exfunc->set_ycp_filtering_cache_size(fp,fpip->w,fpip->h,CACHE_FRAME,NULL);
	int src_w,src_h;
	int f = fpip->frame + (fp->exfunc->get_frame_n(fpip->editp)+sepfp->track[tMRGN])/2;
	PIXEL_YC* btm = fp->exfunc->get_ycp_filtering_cache_ex(fp,fpip->editp,f,&src_w,&src_h);
	PIXEL_YC* top = fp->exfunc->get_ycp_filtering_cache_ex(fp,fpip->editp,fpip->frame,NULL,NULL);
	if(btm==NULL||top==NULL) return FALSE;

	PIXEL_YC* dst = fpip->ycp_edit;

	int row_size = src_w * sizeof(PIXEL_YC);

	for(int i=src_h;i;--i){
		// トップフィールド
		memcpy(dst,top,row_size);
		dst += fpip->max_w;

		// ボトムフィールド
		memcpy(dst,btm,row_size);

		btm += src_w;
		top += src_w;
		dst += fpip->max_w;
	}

	fpip->h = h;

	dblframe.UnDouble(fp,fpip);

	return TRUE;
}

/*--------------------------------------------------------------------
*	時間軸方向結合 type2 (Bottom First)
*-------------------------------------------------------------------*/
static BOOL weave_time2_bff(FILTER *fp,FILTER_PROC_INFO *fpip)
{

	// 高さ倍加
	int h = fpip->h * 2;
	if(h > fpip->max_h){
		MessageBox(fp->hwnd,ERR_SHORT_OF_MAXHEIGHT,filtername,MB_OK|MB_ICONERROR);
		return FALSE;
	}

	if(!dblframe.Double(fp,fpip,sepfp->track[tMRGN]))	// フレーム数倍加
		return FALSE;

	fp->exfunc->set_ycp_filtering_cache_size(fp,fpip->w,fpip->h,CACHE_FRAME,NULL);
	int src_w,src_h;
	int f = fpip->frame + (fp->exfunc->get_frame_n(fpip->editp)+sepfp->track[tMRGN])/2;
	PIXEL_YC* top = fp->exfunc->get_ycp_filtering_cache_ex(fp,fpip->editp,f,&src_w,&src_h);
	PIXEL_YC* btm = fp->exfunc->get_ycp_filtering_cache_ex(fp,fpip->editp,fpip->frame,NULL,NULL);
	if(btm==NULL||top==NULL) return FALSE;

	PIXEL_YC* dst = fpip->ycp_edit;

	int row_size = src_w * sizeof(PIXEL_YC);

	for(int i=src_h;i;--i){
		// トップフィールド
		memcpy(dst,top,row_size);
		dst += fpip->max_w;

		// ボトムフィールド
		memcpy(dst,btm,row_size);

		btm += src_w;
		top += src_w;
		dst += fpip->max_w;
	}

	fpip->h = h;

	dblframe.UnDouble(fp,fpip);

	return TRUE;
}


/*====================================================================
*	開始時初期化
*===================================================================*/
BOOL func_init( FILTER *fp )
{
	sepfp = NULL;
	for(int n=0;(sepfp=(FILTER*)fp->exfunc->get_filterp(n))!=NULL;n++){
		if(lstrcmp(sepfp->information,FIELDSEPAR_INFO)==0)
			return TRUE;
	}

	MessageBox(fp->hwnd,NOT_FOUND_FIELDSP,filtername,MB_OK | MB_ICONERROR);

	return FALSE;
}



