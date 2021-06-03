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


TCHAR	SZ_APP_MUTEX[]	= _T("なんちゃってPSP");
TCHAR	SZ_APP_TITLE[]	= _T("なんちゃってPSP 1.0.5 (Not Emulator)");
TCHAR	SZ_APP_TITLE2[]	= _T("(C) Copyright 2005 そとのひと");


const DWORD g_dwINTERVAL_TIME = 10;
#define	INTERVAL_VSYNC	17


#define	PSP_SCREEN_WIDTH	480
#define	PSP_SCREEN_HEIGHT	272


DWORD	g_dwVSYNCCounter = 0;


//-----------------------------------------------------------
// スレッドで実行する。開発中は外した方がいい。かなり。
//-----------------------------------------------------------
//#define __USE_THREAD


// リリースビルドではスレッド動作にするので、このままで。
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
// CSampleDlgDlg ダイアログ

CSampleDlgDlg::CSampleDlgDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSampleDlgDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSampleDlgDlg)
		// メモ: この位置に ClassWizard によってメンバの初期化が追加されます。
	//}}AFX_DATA_INIT
	// メモ: LoadIcon は Win32 の DestroyIcon のサブシーケンスを要求しません。
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CSampleDlgDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSampleDlgDlg)
		// メモ: この場所には ClassWizard によって DDX と DDV の呼び出しが追加されます。
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
// CSampleDlgDlg メッセージ ハンドラ
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
	// F1 は START で使うのでナシに
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

	// アクセラレータ
	m_hAccel = ::LoadAccelerators(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_ACCELERATOR1));

	this->SetClientSize(PSP_SCREEN_WIDTH, PSP_SCREEN_HEIGHT, true, true);

	// DIBSection (512x272)x2 (480x72ではない)
	m_dibVRAM.Create(PSP_SCREEN_WIDTH + 32, (PSP_SCREEN_HEIGHT + 8) * 2, 15);

	// スレッド作成 (17ms = 1/60 として擬似 VSYNC)
	m_pThread = AfxBeginThread(ThreadFunc, NULL);
	m_pThreadPSP = NULL;

#ifdef __USE_THREAD
	m_pThreadPSP = AfxBeginThread(ThreadFuncPSP, this);
#endif

	// その他の初期化
	::timeBeginPeriod(1);
	
	return TRUE;
}



// ---------------------------------------------------------------------------------------------
// WinGL L1 0.11e のソースから拝借
//
// --------------------------------------------------------------------------
// [WinGL 著作権]
//名称    Bio_100% WinGL L1 Version 0.11e
//著作権  Copyright(C) 1995-1997 恋塚,alty / Bio_100%
//所在    http://bio.and.or.jp/wingl.html  および  NIFTY-Serve FGALGL LIB 5
// --------------------------------------------------------------------------
//
// ただし現在は恋塚さんのサイト
// http://www.ss.iij4u.or.jp/~koizuka/winglbeta.html
// でのみ提供中(？)
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
// メインルーチン
//
VOID CSampleDlgDlg::MainRoutine(VOID)
{
#ifndef __USE_THREAD
		ThreadFuncPSP(this);
#endif
}


// VSYNC割り込みを表すため、単なるウェイトで代用
UINT CSampleDlgDlg::ThreadFunc(LPVOID)
{
	// 厳密には 16msec 固定でなく、前回との差から計算しなければならないが
	// 面倒なので一律固定
	while(1) {
		g_dwVSYNCCounter++;
		::Sleep(INTERVAL_VSYNC);
	}

	return 0;
}

void CSampleDlgDlg::OnClose() 
{
	// TODO: この位置にメッセージ ハンドラ用のコードを追加するかまたはデフォルトの処理を呼び出してください
	if (m_pThreadPSP)
		delete m_pThreadPSP;

	if (m_pThread)
		delete m_pThread;

	::timeEndPeriod(1);
	if (m_hAccel)
		::DestroyAcceleratorTable(m_hAccel);
	
	CDialog::OnClose();
}
