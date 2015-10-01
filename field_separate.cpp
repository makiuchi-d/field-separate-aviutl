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
//	FILTER_DLL�\����
//----------------------------
char filtername[] = "�t�B�[���h����";
char filterinfo[] = "�t�B�[���h���� ver 0.01 by MakKi";
#define track_N 1
#if track_N
TCHAR *track_name[]   = { "�Ԋu", };	// �g���b�N�o�[�̖��O
int   track_default[] = {     8 , };	// �g���b�N�o�[�̏����l
int   track_s[]       = {     0 , };	// �g���b�N�o�[�̉����l
int   track_e[]       = {    20 , };	// �g���b�N�o�[�̏���l
#endif

#define check_N 2
#if check_N
TCHAR *check_name[]   = { "�Ԋu���㉺�̐F�Ŗ��߂�", "�����\��", };	// �`�F�b�N�{�b�N�X
int   check_default[] = { 1, 0 };	// �f�t�H���g
#endif

#define tMRGN     0
#define cCOLOR    0
#define cDISP     1


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
	NULL,NULL,   	// �J�n��,�I�����ɌĂ΂��֐�
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
	int i,j;
	PIXEL_YC *src;
	PIXEL_YC *dst;
	int pitch = fpip->max_w * 2;

	dst = fpip->ycp_temp;

	// �g�b�v�t�B�[���h
	src = fpip->ycp_edit;
	for(i=fpip->h-fpip->h/2; i;--i){
		memcpy(dst,src,fpip->w*sizeof(PIXEL_YC));
		src += pitch;
		dst += fpip->max_w;
	}

	// �Ԋu
	if(!fp->check[cCOLOR]){	// ���œh��Ԃ�
		for(j=fp->track[tMRGN];j;--j){
			memset(dst,0,fpip->w*sizeof(PIXEL_YC));
			dst += fpip->max_w;
		}
	}
	else {	// �㉺�̐F�œh��Ԃ�
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

	// �{�g���t�B�[���h
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



