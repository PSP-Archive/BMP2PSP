// SampleDlg.h : SAMPLEDLG �A�v���P�[�V�����̃��C�� �w�b�_�[ �t�@�C���ł��B
//

#if !defined(AFX_SAMPLEDLG_H__30FABEA2_9629_4B75_93BB_86C6B8C3C1DF__INCLUDED_)
#define AFX_SAMPLEDLG_H__30FABEA2_9629_4B75_93BB_86C6B8C3C1DF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// ���C�� �V���{��

/////////////////////////////////////////////////////////////////////////////
// CSampleDlgApp:
// ���̃N���X�̓���̒�`�Ɋւ��Ă� SampleDlg.cpp �t�@�C�����Q�Ƃ��Ă��������B
//

class CSampleDlgApp : public CWinApp
{
private:

public:
	CSampleDlgApp();

// �I�[�o�[���C�h
	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(CSampleDlgApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����

	//{{AFX_MSG(CSampleDlgApp)
		// ���� - ClassWizard �͂��̈ʒu�Ƀ����o�֐���ǉ��܂��͍폜���܂��B
		//        ���̈ʒu�ɐ��������R�[�h��ҏW���Ȃ��ł��������B
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ �͑O�s�̒��O�ɒǉ��̐錾��}�����܂��B

#endif // !defined(AFX_SAMPLEDLG_H__30FABEA2_9629_4B75_93BB_86C6B8C3C1DF__INCLUDED_)