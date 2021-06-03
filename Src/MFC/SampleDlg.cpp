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
		// メモ - ClassWizard はこの位置にマッピング用のマクロを追加または削除します。
		//        この位置に生成されるコードを編集しないでください。
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSampleDlgApp クラスの構築

CSampleDlgApp::CSampleDlgApp()
{
	// TODO: この位置に構築用のコードを追加してください。
	// ここに InitInstance 中の重要な初期化処理をすべて記述してください。
}

/////////////////////////////////////////////////////////////////////////////
// 唯一の CSampleDlgApp オブジェクト

CSampleDlgApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CSampleDlgApp クラスの初期化

BOOL CSampleDlgApp::InitInstance()
{
	AfxEnableControlContainer();

#ifdef _AFXDLL
	Enable3dControls();
#else
	Enable3dControlsStatic();
#endif

	if (!IsWin2000orLater()) {
		AfxMessageBox(_T("このソフトウェアは Windows 2000 以降で動作します (Windows XP 以降を推奨)"));
		return FALSE;
	}

	CSampleDlgDlg dlg;
	m_pMainWnd = &dlg;
	int nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: ダイアログが <OK> で消された時のコードを
		//       記述してください。
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: ダイアログが <ｷｬﾝｾﾙ> で消された時のコードを
		//       記述してください。
	}

	// ダイアログが閉じられてからアプリケーションのメッセージ ポンプを開始するよりは、
	// アプリケーションを終了するために FALSE を返してください。
	return FALSE;
}


int CSampleDlgApp::ExitInstance() 
{
	return CWinApp::ExitInstance();
}
