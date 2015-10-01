/*********************************************************************
* 	�t�B�[���h���� for AviUtl
* 								ver. 0.01
* [2003]
* 	11/12:	���J(0.01)
* 
*********************************************************************/
#include <windows.h>
#include <math.h>
#include "filter.h"


//----------------------------
//	�O���[�o���ϐ�
//----------------------------
static FILTER* sepfp;
const  char* FIELDSEP_NAME     = "�t�B�[���h����";
const  char* NOT_FOUND_FIELDSP = "�t�B�[���h�����t�B���^��������܂���";

//----------------------------
//	FILTER_DLL�\����
//----------------------------
char filtername[] = "�t�B�[���h����";
char filterinfo[] = "�t�B�[���h���� ver 0.01 by MakKi";
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

#define tMRGN     0
#define cCOLOR    0
#define cDISP     1


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
*	�J�n��������
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



