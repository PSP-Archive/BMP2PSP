#include "stdafx.h"
#include "CDIBSection.h"
#include "SampleDlg.h"
#include "SampleDlgDlg.h"
#include "CCommon.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


extern TCHAR SZ_APP_MUTEX[];


/////////////////////////////////////////////////////////////////////////////
// CSampleDlgApp

BEGIN_MESSAGE_MAP(CSampleDlgApp, CWinApp)
	//{{AFX_MSG_MAP(CSampleDlgApp)
		// ���� - ClassWizard �͂��̈ʒu�Ƀ}�b�s���O�p�̃}�N����ǉ��܂��͍폜���܂��B
		//        ���̈ʒu�ɐ��������R�[�h��ҏW���Ȃ��ł��������B
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSampleDlgApp �N���X�̍\�z

CSampleDlgApp::CSampleDlgApp()
{
	// TODO: ���̈ʒu�ɍ\�z�p�̃R�[�h��ǉ����Ă��������B
	// ������ InitInstance ���̏d�v�ȏ��������������ׂċL�q���Ă��������B
}

/////////////////////////////////////////////////////////////////////////////
// �B��� CSampleDlgApp �I�u�W�F�N�g

CSampleDlgApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CSampleDlgApp �N���X�̏�����

BOOL CSampleDlgApp::InitInstance()
{
	AfxEnableControlContainer();

#ifdef _AFXDLL
	Enable3dControls();
#else
	Enable3dControlsStatic();
#endif

	if (!IsWin2000orLater()) {
		AfxMessageBox(_T("���̃\�t�g�E�F�A�� Windows 2000 �ȍ~�œ��삵�܂� (Windows XP �ȍ~�𐄏�)"));
		return FALSE;
	}

	CSampleDlgDlg dlg;
	m_pMainWnd = &dlg;
	int nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: �_�C�A���O�� <OK> �ŏ����ꂽ���̃R�[�h��
		//       �L�q���Ă��������B
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: �_�C�A���O�� <��ݾ�> �ŏ����ꂽ���̃R�[�h��
		//       �L�q���Ă��������B
	}

	// �_�C�A���O�������Ă���A�v���P�[�V�����̃��b�Z�[�W �|���v���J�n������́A
	// �A�v���P�[�V�������I�����邽�߂� FALSE ��Ԃ��Ă��������B
	return FALSE;
}


int CSampleDlgApp::ExitInstance() 
{
	return CWinApp::ExitInstance();
}