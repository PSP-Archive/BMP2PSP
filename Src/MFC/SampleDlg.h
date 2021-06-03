// SampleDlg.h : SAMPLEDLG アプリケーションのメイン ヘッダー ファイルです。
//

#if !defined(AFX_SAMPLEDLG_H__30FABEA2_9629_4B75_93BB_86C6B8C3C1DF__INCLUDED_)
#define AFX_SAMPLEDLG_H__30FABEA2_9629_4B75_93BB_86C6B8C3C1DF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// メイン シンボル

/////////////////////////////////////////////////////////////////////////////
// CSampleDlgApp:
// このクラスの動作の定義に関しては SampleDlg.cpp ファイルを参照してください。
//

class CSampleDlgApp : public CWinApp
{
private:

public:
	CSampleDlgApp();

// オーバーライド
	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CSampleDlgApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// インプリメンテーション

	//{{AFX_MSG(CSampleDlgApp)
		// メモ - ClassWizard はこの位置にメンバ関数を追加または削除します。
		//        この位置に生成されるコードを編集しないでください。
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_SAMPLEDLG_H__30FABEA2_9629_4B75_93BB_86C6B8C3C1DF__INCLUDED_)
