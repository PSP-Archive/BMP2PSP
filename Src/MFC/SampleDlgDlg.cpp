#include "stdafx.h"
#include "CDIBSection.h"
#include "SampleDlg.h"
#include "SampleDlgDlg.h"
#include "CCommon.h"

#include "AFXPRIV.H"
#include "mmsystem.h"
#pragma comment (lib, "winmm.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


TCHAR	SZ_APP_MUTEX[]	= _T("�Ȃ񂿂����PSP");
TCHAR	SZ_APP_TITLE[]	= _T("�Ȃ񂿂����PSP 1.0.5 (Not Emulator)");
TCHAR	SZ_APP_TITLE2[]	= _T("(C) Copyright 2005 ���Ƃ̂Ђ�");


const DWORD g_dwINTERVAL_TIME = 10;
#define	INTERVAL_VSYNC	17


#define	PSP_SCREEN_WIDTH	480
#define	PSP_SCREEN_HEIGHT	272


DWORD	g_dwVSYNCCounter = 0;


//-----------------------------------------------------------
// �X���b�h�Ŏ��s����B�J�����͊O�������������B���Ȃ�B
//-----------------------------------------------------------
//#define __USE_THREAD


// �����[�X�r���h�ł̓X���b�h����ɂ���̂ŁA���̂܂܂ŁB
#ifndef _DEBUG
#ifndef __USE_THREAD
#define __USE_THREAD
#endif
#endif


#ifdef __USE_THREAD
BOOL g_bModeThread = TRUE;
#else
BOOL g_bModeThread = FALSE;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSampleDlgDlg �_�C�A���O

CSampleDlgDlg::CSampleDlgDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSampleDlgDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSampleDlgDlg)
		// ����: ���̈ʒu�� ClassWizard �ɂ���ă����o�̏��������ǉ�����܂��B
	//}}AFX_DATA_INIT
	// ����: LoadIcon �� Win32 �� DestroyIcon �̃T�u�V�[�P���X��v�����܂���B
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CSampleDlgDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSampleDlgDlg)
		// ����: ���̏ꏊ�ɂ� ClassWizard �ɂ���� DDX �� DDV �̌Ăяo�����ǉ�����܂��B
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CSampleDlgDlg, CDialog)
	//{{AFX_MSG_MAP(CSampleDlgDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_COMMAND(ID_HELP, OnHelp)
	ON_WM_DROPFILES()
	ON_COMMAND(ID_HELP2, OnHelp2)
	ON_WM_CLOSE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CSampleDlgDlg ���b�Z�[�W �n���h��
//---------------------------------------------------------------------------------------
void CSampleDlgDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
		OnHelp();
	else
		CDialog::OnSysCommand(nID, lParam);
}

extern void PresentDisplay();


void CSampleDlgDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this);
		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;
		dc.DrawIcon(x, y, m_hIcon);
	} else {
#ifdef __USE_THREAD
		if (m_pThreadPSP) {
			PresentDisplay();
		}
#endif
		CDialog::OnPaint();
	}
}


HCURSOR CSampleDlgDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}


void CSampleDlgDlg::OnHelp() 
{
	// F1 �� START �Ŏg���̂Ńi�V��
}


void CSampleDlgDlg::OnHelp2() 
{
	CAboutDlg dlg;
	dlg.DoModal();
}
//---------------------------------------------------------------------------------------


extern UINT ThreadFuncPSP(LPVOID lpData);


BOOL CSampleDlgDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}
	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);
	
	SetWindowText(SZ_APP_TITLE);

	// �A�N�Z�����[�^
	m_hAccel = ::LoadAccelerators(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_ACCELERATOR1));

	this->SetClientSize(PSP_SCREEN_WIDTH, PSP_SCREEN_HEIGHT, true, true);

	// DIBSection (512x272)x2 (480x72�ł͂Ȃ�)
	m_dibVRAM.Create(PSP_SCREEN_WIDTH + 32, (PSP_SCREEN_HEIGHT + 8) * 2, 15);

	// �X���b�h�쐬 (17ms = 1/60 �Ƃ��ċ[�� VSYNC)
	m_pThread = AfxBeginThread(ThreadFunc, NULL);
	m_pThreadPSP = NULL;

#ifdef __USE_THREAD
	m_pThreadPSP = AfxBeginThread(ThreadFuncPSP, this);
#endif

	// ���̑��̏�����
	::timeBeginPeriod(1);
	
	return TRUE;
}



// ---------------------------------------------------------------------------------------------
// WinGL L1 0.11e �̃\�[�X����q��
//
// --------------------------------------------------------------------------
// [WinGL ���쌠]
//����    Bio_100% WinGL L1 Version 0.11e
//���쌠  Copyright(C) 1995-1997 ����,alty / Bio_100%
//����    http://bio.and.or.jp/wingl.html  �����  NIFTY-Serve FGALGL LIB 5
// --------------------------------------------------------------------------
//
// ���������݂͗��˂���̃T�C�g
// http://www.ss.iij4u.or.jp/~koizuka/winglbeta.html
// �ł̂ݒ񋟒�(�H)
BOOL CSampleDlgDlg::SetClientSize(unsigned width, unsigned height, BOOL bRepaint, BOOL bForce) {
	RECT rect, crect;
	GetWindowRect(&rect);
	GetClientRect(&crect);

	if (width)
		width += rect.right - rect.left - crect.right;
	else
		width = rect.right - rect.left;
	if (height)
		height += rect.bottom - rect.top - crect.bottom;
	else
		height = rect.bottom - rect.top;

	unsigned ScreenWidth =	::GetSystemMetrics(SM_CXFULLSCREEN);
	unsigned ScreenHeight =	::GetSystemMetrics(SM_CYFULLSCREEN);
	if (rect.left + width > ScreenWidth) {
		rect.left = ScreenWidth - width;
		if (rect.left < 0)
			rect.left = 0;
	}
	if (rect.top + height > ScreenHeight) {
		rect.top = ScreenHeight - height;
		if (rect.top < 0)
			rect.top = 0;
	}

	BOOL bOk = ((rect.left + width) <= ScreenWidth && (rect.top + height) <= ScreenHeight);
	if (bForce  ||  bOk)
		MoveWindow(rect.left, rect.top, width, height, bRepaint);

	return bOk;
}
// ---------------------------------------------------------------------------------------------


BOOL CSampleDlgDlg::PreTranslateMessage(MSG* pMsg) 
{
	if (CWnd::PreTranslateMessage(pMsg))
		return TRUE;
	return (m_hAccel != NULL) && ::TranslateAccelerator(m_hWnd, m_hAccel, pMsg);
}


void CSampleDlgDlg::OnDropFiles(HDROP hDropInfo) 
{
}


LRESULT CSampleDlgDlg::WindowProc(UINT message, WPARAM wParam, LPARAM lParam) 
{
#ifndef __USE_THREAD
	if (message == WM_KICKIDLE) {
		MainRoutine();
	}
#endif

	return CDialog::WindowProc(message, wParam, lParam);
}


//--------------------------------------------------------------------------------------------------------------
// ���C�����[�`��
//
VOID CSampleDlgDlg::MainRoutine(VOID)
{
#ifndef __USE_THREAD
		ThreadFuncPSP(this);
#endif
}


// VSYNC���荞�݂�\�����߁A�P�Ȃ�E�F�C�g�ő�p
UINT CSampleDlgDlg::ThreadFunc(LPVOID)
{
	// �����ɂ� 16msec �Œ�łȂ��A�O��Ƃ̍�����v�Z���Ȃ���΂Ȃ�Ȃ���
	// �ʓ|�Ȃ̂ňꗥ�Œ�
	while(1) {
		g_dwVSYNCCounter++;
		::Sleep(INTERVAL_VSYNC);
	}

	return 0;
}

void CSampleDlgDlg::OnClose() 
{
	// TODO: ���̈ʒu�Ƀ��b�Z�[�W �n���h���p�̃R�[�h��ǉ����邩�܂��̓f�t�H���g�̏������Ăяo���Ă�������
	if (m_pThreadPSP)
		delete m_pThreadPSP;

	if (m_pThread)
		delete m_pThread;

	::timeEndPeriod(1);
	if (m_hAccel)
		::DestroyAcceleratorTable(m_hAccel);
	
	CDialog::OnClose();
}