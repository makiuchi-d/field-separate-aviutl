/*********************************************************************
* 	�t�B�[���h���� for AviUtl
* 								ver. 0.02
* [2003]
* 	11/12:	���J(0.01)
* 	12/30:	MODE2(������������)��ǉ�
* [2004]
* 	01/03:	MODE3(���Ԏ���������)��ǉ�
* 	08/18:	�啝�Ȏ蒼���J�n
* 			MODE4(���Ԏ��������� type2)��ǉ�(������)
* 			�g�b�v�ƃ{�g���̔z�u�����ւ����悤��(������)
* 	09/08:	�蒼�������B���Ƃ͒ǉ��@�\�̎����B
* 	09/16:	�悤�₭�S�@�\���������B
* 			�ł��R�s�y���炯�̌��ꂵ���\�[�X�Ɂc(0.02)
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


static BOOL separate_vertical_tff(FILTER *fp,FILTER_PROC_INFO *fpip);
static BOOL separate_vertical_bff(FILTER *fp,FILTER_PROC_INFO *fpip);
static BOOL separate_horizon_tff(FILTER *fp,FILTER_PROC_INFO *fpip);
static BOOL separate_horizon_bff(FILTER *fp,FILTER_PROC_INFO *fpip);
static BOOL separate_time_tff(FILTER *fp,FILTER_PROC_INFO *fpip);
static BOOL separate_time_bff(FILTER *fp,FILTER_PROC_INFO *fpip);
static BOOL separate_time2_tff(FILTER *fp,FILTER_PROC_INFO *fpip);
static BOOL separate_time2_bff(FILTER *fp,FILTER_PROC_INFO *fpip);

//----------------------------
//	�O���[�o���ϐ�
//----------------------------
const int   CACHE_FRAME       = 9;	// �L���b�V������t���[����
const char* NOT_FOUND_FIELDWV = "�t�B�[���h�����t�B���^��������܂���";

static BOOL (* const proc[4][2])(FILTER*,FILTER_PROC_INFO*) = {
	{ separate_vertical_tff , separate_vertical_bff },
	{ separate_horizon_tff  , separate_horizon_bff  },
	{ separate_time_tff     , separate_time_bff     },
	{ separate_time2_tff    , separate_time2_bff    }
};

typedef BOOL (*FUNCPROC)(FILTER*,FILTER_PROC_INFO*);
const FUNCPROC func = proc[0][0];

//----------------------------
//	FILTER_DLL�\����
//----------------------------
char filtername[] = FIELDSEPAR_NAME;
char filterinfo[] = FIELDSEPAR_INFO;
#define track_N 2
#if track_N
TCHAR *track_name[]   = { "�Ԋu", "Mode" };	// �g���b�N�o�[�̖��O
int   track_default[] = {     8 ,     0  };	// �g���b�N�o�[�̏����l
int   track_s[]       = {     0 ,     0  };	// �g���b�N�o�[�̉����l
int   track_e[]       = {    20 ,     3  };	// �g���b�N�o�[�̏���l
#endif

#define check_N 3

#if check_N
TCHAR *check_name[]   = { "�Ԋu��ڂ���s�N�Z���̐F�Ŗ��߂�",
                          "�g�b�v�ƃ{�g���̔z�u���t�ɂ���",
                          "�����\��", };	// �`�F�b�N�{�b�N�X
int   check_default[] = { 1, 0, 0 };	// �f�t�H���g
#endif


FILTER_DLL filter = {
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
	func_WndProc,	// �ݒ�E�B���h�E�v���V�[�W��
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
*===================================================================*/
BOOL func_proc(FILTER *fp,FILTER_PROC_INFO *fpip)
{
	return proc[fp->track[tMODE]][fp->check[cSWAP]](fp,fpip);
}


/*--------------------------------------------------------------------
*	������������ (��Top)
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

	// �ő卂������Ȃ���
	if(fpip->max_h < fpip->h+fp->track[tMRGN]){
		MessageBox(fp->hwnd,ERR_SHORT_OF_MAXHEIGHT,filtername,MB_OK|MB_ICONERROR);
		return FALSE;
	}

	// �g�b�v�t�B�[���h
	src = fpip->ycp_temp;
	for(i=fpip->h-fpip->h/2; i;--i){
		memcpy(dst,src,rowsize);
		src += pitch;
		dst += fpip->max_w;
	}

	// �Ԋu
	if(fp->check[cCOLOR]){	// �㉺�̐F�œh��Ԃ�
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
	else {	// ���œh��Ԃ�
		for(j=fp->track[tMRGN];j;--j){
			memset(dst,0,rowsize);
			dst += fpip->max_w;
		}
	}

	// �{�g���t�B�[���h
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
*	������������ (��Bottom)
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

	// �ő卂������Ȃ���
	if(fpip->max_h < fpip->h+fp->track[tMRGN]){
		MessageBox(fp->hwnd,ERR_SHORT_OF_MAXHEIGHT,filtername,MB_OK|MB_ICONERROR);
		return FALSE;
	}

	// �{�g���t�B�[���h
	src = fpip->ycp_temp + fpip->max_w;
	for(i=fpip->h/2;i;--i){
		memcpy(dst,src,rowsize);
		src += pitch;
		dst += fpip->max_w;
	}

	// �Ԋu
	if(fp->check[cCOLOR]){	// �㉺�̐F�œh��Ԃ�
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
	else {	// ���œh��Ԃ�
		for(j=fp->track[tMRGN];j;--j){
			memset(dst,0,rowsize);
			dst += fpip->max_w;
		}
	}

	// �g�b�v�t�B�[���h
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
*	������������ (��Top)
*-------------------------------------------------------------------*/
static BOOL separate_horizon_tff(FILTER *fp,FILTER_PROC_INFO *fpip)
{
	// �ő啝������Ȃ���
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
		memcpy(dst,src,row_size);	// �g�b�v�t�B�[���h
		dst += fpip->w;

		// �Ԋu�𖄂߂�
		if(fp->check[cCOLOR]){
			for(j=(fp->track[tMRGN]+1)>>1; j; --j)
				*dst++ = src[fpip->w-1];
			for(j=fp->track[tMRGN]/2; j; --j)
				*dst++ = src[fpip->max_w];
		}
		else{	// ���Ŗ��߂�
			memset(dst,0,fp->track[tMRGN]*sizeof(PIXEL_YC));
			dst += fp->track[tMRGN];
		}

		src += fpip->max_w;
		memcpy(dst,src,row_size);	// �{�g���t�B�[���h

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
*	������������ (��Bottom)
*-------------------------------------------------------------------*/
static BOOL separate_horizon_bff(FILTER *fp,FILTER_PROC_INFO *fpip)
{

	// �ő啝������Ȃ���
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

		// �{�g���t�B�[���h
		memcpy(dst,src+fpip->max_w,row_size);
		dst += fpip->w;

		// �Ԋu�𖄂߂�
		if(fp->check[cCOLOR]){
			for(j=(fp->track[tMRGN]+1)>>1; j; --j)
				*dst++ = src[t];
			for(j=fp->track[tMRGN]/2; j; --j)
				*dst++ = *src;
		}
		else{	// ���Ŗ��߂�
			memset(dst,0,fp->track[tMRGN]*sizeof(PIXEL_YC));
			dst += fp->track[tMRGN];
		}

		// �g�b�v�t�B�[���h
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
*	���Ԏ��������� (�OTop)
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

	if(fpip->frame%2 == 0){	// �g�b�v�t�B�[���h
		for(int i=(src_h+1)/2;i;--i){
			memcpy(dst,src,row_size);
			dst += fpip->max_w;
			src += src_pitch;
		}
	}
	else {	// �{�g���t�B�[���h
		src += src_w;
		for(int i=src_h/2;i;--i){
			memcpy(dst,src,row_size);
			dst += fpip->max_w;
			src += src_pitch;
		}
		if(fpip->h%2){	// ������̎��A�ŉ����C����������
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
*	���Ԏ��������� (�OBottom)
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

	if(fpip->frame%2 == 0){	// �{�g���t�B�[���h
		src += src_w;
		for(int i=src_h/2;i;--i){
			memcpy(dst,src,row_size);
			dst += fpip->max_w;
			src += src_pitch;
		}
		if(fpip->h%2){	// ������̎��A�ŉ����C����������
			if(fp->check[cCOLOR])
				memcpy(dst,dst-fpip->max_w,row_size);
			else
				memset(dst,0,row_size);
		}
	}
	else {	// �g�b�v�t�B�[���h
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
*	���Ԏ��������� type2 (�OTop)
*-------------------------------------------------------------------*/
static BOOL separate_time2_tff(FILTER *fp,FILTER_PROC_INFO *fpip)
{
	PIXEL_YC* src;
	PIXEL_YC* dst;

	if(fpip->frame < (fpip->frame_n-fp->track[tMRGN])/2){
		// �g�b�v�t�B�[���h
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
		// �{�g���t�B�[���h
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
		if(fpip->h%2){	// ������̎��A�ŉ����C����������
			if(fp->check[cCOLOR])
				memcpy(dst,dst-fpip->max_w,row_size);
			else
				memset(dst,0,row_size);
		}
	}
	else{	// �Ԋu
		if(fp->check[cCOLOR]){
			int src_w,src_h,src_pitch;
			fp->exfunc->set_ycp_filtering_cache_size(fp,fpip->w,fpip->h,CACHE_FRAME,NULL);

			if(fpip->frame > fpip->frame_n/2){
				// �P�t���[���ڂ̃{�g���t�B�[���h
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
				if(fpip->h%2){	// ������̎��A�ŉ����C����������
					if(fp->check[cCOLOR])
						memcpy(dst,dst-fpip->max_w,row_size);
					else
						memset(dst,0,row_size);
				}
			}else{
				// �ŏI�t���[���̃g�b�v�t�B�[���h
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
*	���Ԏ��������� type2 (�OBottom)
*-------------------------------------------------------------------*/
static BOOL separate_time2_bff(FILTER *fp,FILTER_PROC_INFO *fpip)
{
	PIXEL_YC* src;
	PIXEL_YC* dst;

	if(fpip->frame < (fpip->frame_n-fp->track[tMRGN])/2){
		// �{�g���t�B�[���h
		src = dst = fpip->ycp_edit;
		src += fpip->max_w;
		int src_pitch = fpip->max_w * 2;
		int row_size  = fpip->w*sizeof(PIXEL_YC);

		for(int i=fpip->h/2; i; --i){
			memcpy(dst,src,row_size);
			dst += fpip->max_w;
			src += src_pitch;
		}
		if(fpip->h%2){	// ������̎��A�ŉ����C����������
			if(fp->check[cCOLOR])
				memcpy(dst,dst-fpip->max_w,row_size);
			else
				memset(dst,0,row_size);
		}
	}
	else if(fpip->frame >= (fpip->frame_n+fp->track[tMRGN])/2){
		// �g�b�v�t�B�[���h
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
	else{	// �Ԋu
		if(fp->check[cCOLOR]){
			int src_w,src_h,src_pitch;
			fp->exfunc->set_ycp_filtering_cache_size(fp,fpip->w,fpip->h,CACHE_FRAME,NULL);

			if(fpip->frame > fpip->frame_n/2){	// �Ԋu�㔼
				// �P�t���[���ڂ̃g�b�v�t�B�[���h
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
			else {	// �Ԋu�O��
				// �ŏI�t���[���̃{�g���t�B�[���h
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
				if(fpip->h%2){	// ������̎��A�ŉ����C����������
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
*	�ݒ�E�B���h�E�v���V�[�W��
*===================================================================*/
BOOL func_WndProc( HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, void *editp, FILTER *fp )
{
	switch(message){
		case WM_KEYUP:	// ���C���E�B���h�E�֑���
		case WM_KEYDOWN:
		case WM_MOUSEWHEEL:
			SendMessage(GetWindow(hwnd, GW_OWNER), message, wparam, lparam);
			break;
	}

	return FALSE;
}

/*====================================================================
*	�J�n��������
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


