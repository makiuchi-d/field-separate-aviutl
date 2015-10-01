/*********************************************************************
* 	�t�B�[���h���� for AviUtl
* 								ver. 0.02
* [2003]
* 	11/12:	���J(0.01)
* 	12/30:	MODE2(������������)��ǉ�
* [2004]
* 	01/03:	MODE3(���Ԏ���������)��ǉ�
* 	08/18:	�啝�Ȏ蒼���J�n�B
* 			MODE4(���Ԏ��������� type2)��ǉ�(������)
* 			�g�b�v�ƃ{�g���̔z�u�����ւ����悤��(������)
* 	09/08:	�蒼�������B���Ƃ͒ǉ��@�\�̎����B
* 	09/17:	�S�Ă̋@�\�����������B(0.02)
* 
*********************************************************************/
/* TODO:
* 
* 	������Ԃł͏c�������ɂȂ邽�߃t�B���^�̌��ʂ����܂��Ă��܂�
* 		���������ɏc��{�ɂ���i�I�v�V�����j
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
* 		�t���[��������
*/
class DoubleFrame {	
	bool flag;
	int  margin;

	bool CheckFrameMax(FILTER* fp,int frame_n){	// �ő�t���[�����`�F�b�N
		SYS_INFO si;
		if(fp->exfunc->get_sys_info(NULL,&si)==FALSE)
			return false;
		if(si.max_frame < frame_n){	// ����Ȃ�
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
* 		���Ԏ�����(type1)�Ŕ{�����̑I��\���t���[���X�V
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
//	�v���g�^�C�v
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
//	�O���[�o���ϐ�
//----------------------------
static FILTER* sepfp;
const  char* NOT_FOUND_FIELDSP = "�t�B�[���h�����t�B���^��������܂���";
const  int   CACHE_FRAME       = 9;	// �L���b�V������t���[����

static BOOL (* const proc[4][2])(FILTER*,FILTER_PROC_INFO*) = {
	{ weave_vertical_tff , weave_vertical_bff },
	{ weave_horizon_tff  , weave_horizon_bff  },
	{ weave_time_tff     , weave_time_bff     },
	{ weave_time2_tff    , weave_time2_bff    }
};


//----------------------------
//	FILTER_DLL�\����
//----------------------------
char filtername[] = FIELDWEAVE_NAME;
char filterinfo[] = FIELDWEAVE_INFO;
#define track_N 0
#if track_N
TCHAR *track_name[]   = {  };	// �g���b�N�o�[�̖��O
int   track_default[] = {  };	// �g���b�N�o�[�̏����l
int   track_s[]       = {  };	// �g���b�N�o�[�̉����l
int   track_e[]       = {  };	// �g���b�N�o�[�̏���l
#endif

#define check_N 0
#if check_N
TCHAR *check_name[]   = {  };	// �`�F�b�N�{�b�N�X
int   check_default[] = {  };	// �f�t�H���g
#endif


FILTER_DLL filter = {
	FILTER_FLAG_ALWAYS_ACTIVE |	// �t�B���^����ɃA�N�e�B�u�ɂ��܂�
	FILTER_FLAG_NO_CONFIG |	// �ݒ�E�B���h�E��\�����Ȃ��悤�ɂ��܂�
	FILTER_FLAG_EX_INFORMATION,
	NULL,NULL,      	// �ݒ�E�C���h�E�̃T�C�Y
	filtername,     	// �t�B���^�̖��O
	track_N,        	// �g���b�N�o�[�̐�
#if track_N
	track_name,     	// �g���b�N�o�[�̖��O�S
	track_default,  	// �g���b�N�o�[�̏����l�S
	track_s,track_e,	// �g���b�N�o�[�̐��l�̉������
#else
	NULL,NULL,NULL,NULL,
#endif
	check_N,      	// �`�F�b�N�{�b�N�X�̐�
#if check_N
	check_name,   	// �`�F�b�N�{�b�N�X�̖��O�S
	check_default,	// �`�F�b�N�{�b�N�X�̏����l�S
#else
	NULL,NULL,
#endif
	func_proc,   	// �t�B���^�����֐�
	func_init,NULL,	// �J�n��,�I�����ɌĂ΂��֐�
	NULL,        	// �ݒ肪�ύX���ꂽ�Ƃ��ɌĂ΂��֐�
	NULL,			// �ݒ�E�B���h�E�v���V�[�W��
	NULL,NULL,   	// �V�X�e���Ŏg�p
	NULL,NULL,   	// �g���f�[�^�̈�
	filterinfo,  	// �t�B���^���
	NULL,			// �Z�[�u�J�n���O�ɌĂ΂��֐�
	NULL,			// �Z�[�u�I�����ɌĂ΂��֐�
	NULL,NULL,NULL,	// �V�X�e���Ŏg�p
	NULL,			// �g���̈揉���l
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
*	�t�B���^�����֐�
* 
* 	�t���[�����֘A
* 		MODE_T,MODE_T2�̎�
* 			�����\�����t���[����2�{
* 			�ʏ�    ��2�{���ăt�B���^�����Ė߂�
* 
* 		MODE_T�̕����\�����̂ݑI��͈�,�\���ʒu��2�{
* 
*===================================================================*/
static DoublePos   dblpos;
static DoubleFrame dblframe;

BOOL func_proc(FILTER *fp,FILTER_PROC_INFO *fpip)
{
	if(!sepfp) return FALSE;

	if(!fp->exfunc->is_filter_active(sepfp)){	// �t�B���^������
		dblpos.UnDouble(fp,fpip);
		dblframe.UnDouble(fp,fpip);
		return FALSE;
	}

	// �����\����
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
*	������������ (Top First)
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
*	������������ (Bottom First)
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
*	������������ (Top First)
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
*	������������ (Bottom First)
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
*	���Ԏ��������� (Top First)
*-------------------------------------------------------------------*/
static BOOL weave_time_tff(FILTER *fp,FILTER_PROC_INFO *fpip)
{
	int h = fpip->h * 2;
	if(h > fpip->max_h){
		MessageBox(fp->hwnd,ERR_SHORT_OF_MAXHEIGHT,filtername,MB_OK|MB_ICONERROR);
		return FALSE;
	}

	if(!dblframe.Double(fp,fpip))	// �t���[�����{��
		return FALSE;

	int src_w,src_h;

	fp->exfunc->set_ycp_filtering_cache_size(fp,fpip->w,fpip->h,CACHE_FRAME,NULL);
	PIXEL_YC* src1 = fp->exfunc->get_ycp_filtering_cache_ex(fp,fpip->editp,fpip->frame*2,&src_w,&src_h);	// top
	PIXEL_YC* src2 = fp->exfunc->get_ycp_filtering_cache_ex(fp,fpip->editp,fpip->frame*2+1,NULL,NULL);	// bottom
	PIXEL_YC* dst  = fpip->ycp_edit;

	if(src1==NULL||src2==NULL) return FALSE;

	int row_size = src_w * sizeof(PIXEL_YC);

	for(int i=src_h;i;--i){
		// �g�b�v�t�B�[���h
		memcpy(dst,src1,row_size);
		dst += fpip->max_w;

		// �{�g���t�B�[���h
		memcpy(dst,src2,row_size);
		src2 += src_w;

		dst += fpip->max_w;
		src1 += src_w;
	}

	fpip->h = h;

	// ���t���[��������
	dblframe.UnDouble(fp,fpip);

	return TRUE;
}
/*--------------------------------------------------------------------
*	���Ԏ��������� (Bottom First)
*-------------------------------------------------------------------*/
static BOOL weave_time_bff(FILTER *fp,FILTER_PROC_INFO *fpip)
{
	int h = fpip->h * 2;
	if(h > fpip->max_h){
		MessageBox(fp->hwnd,ERR_SHORT_OF_MAXHEIGHT,filtername,MB_OK|MB_ICONERROR);
		return FALSE;
	}

	if(!dblframe.Double(fp,fpip))	// �t���[�����{��
		return FALSE;

	int src_w,src_h;

	fp->exfunc->set_ycp_filtering_cache_size(fp,fpip->w,fpip->h,CACHE_FRAME,NULL);
	PIXEL_YC* src1 = fp->exfunc->get_ycp_filtering_cache_ex(fp,fpip->editp,fpip->frame*2,&src_w,&src_h);	// bottom
	PIXEL_YC* src2 = fp->exfunc->get_ycp_filtering_cache_ex(fp,fpip->editp,fpip->frame*2+1,NULL,NULL);	// top
	PIXEL_YC* dst  = fpip->ycp_edit;

	if(src1==NULL||src2==NULL) return FALSE;

	int row_size = src_w * sizeof(PIXEL_YC);

	for(int i=src_h;i;--i){
		// �g�b�v�t�B�[���h
		memcpy(dst,src2,row_size);
		dst += fpip->max_w;

		// �{�g���t�B�[���h
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
*	���Ԏ��������� type2 (Top First)
*-------------------------------------------------------------------*/
static BOOL weave_time2_tff(FILTER *fp,FILTER_PROC_INFO *fpip)
{
	// �����{��
	int h = fpip->h * 2;
	if(h > fpip->max_h){
		MessageBox(fp->hwnd,ERR_SHORT_OF_MAXHEIGHT,filtername,MB_OK|MB_ICONERROR);
		return FALSE;
	}

	if(!dblframe.Double(fp,fpip,sepfp->track[tMRGN]))	// �t���[�����{��
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
		// �g�b�v�t�B�[���h
		memcpy(dst,top,row_size);
		dst += fpip->max_w;

		// �{�g���t�B�[���h
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
*	���Ԏ��������� type2 (Bottom First)
*-------------------------------------------------------------------*/
static BOOL weave_time2_bff(FILTER *fp,FILTER_PROC_INFO *fpip)
{

	// �����{��
	int h = fpip->h * 2;
	if(h > fpip->max_h){
		MessageBox(fp->hwnd,ERR_SHORT_OF_MAXHEIGHT,filtername,MB_OK|MB_ICONERROR);
		return FALSE;
	}

	if(!dblframe.Double(fp,fpip,sepfp->track[tMRGN]))	// �t���[�����{��
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
		// �g�b�v�t�B�[���h
		memcpy(dst,top,row_size);
		dst += fpip->max_w;

		// �{�g���t�B�[���h
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
*	�J�n��������
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



