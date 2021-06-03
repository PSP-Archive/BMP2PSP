// SampleDlgDlg.h : ヘッダー ファイル
//

#if !defined(AFX_SAMPLEDLGDLG_H__738A5F16_13F1_40E6_919A_17F8835EDE9F__INCLUDED_)
#define AFX_SAMPLEDLGDLG_H__738A5F16_13F1_40E6_919A_17F8835EDE9F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CSampleDlgDlg ダイアログ



class CSampleDlgDlg : public CDialog
{
// 構築
public:
	VOID MainRoutine(VOID);
	CSampleDlgDlg(CWnd* pParent = NULL);	// 標準のコンストラクタ

// ダイアログ データ
	//{{AFX_DATA(CSampleDlgDlg)
	enum { IDD = IDD_SAMPLEDLG_DIALOG };
		// メモ: この位置に ClassWizard によってデータ メンバが追加されます。
	//}}AFX_DATA

	// ClassWizard は仮想関数のオーバーライドを生成します。
	//{{AFX_VIRTUAL(CSampleDlgDlg)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV のサポート
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// インプリメンテーション
public:
	CDIBSection	*GetDIBSection(VOID) {return &m_dibVRAM;}

protected:
	HICON	m_hIcon;
	HACCEL	m_hAccel;

	BOOL		SetClientSize(unsigned width, unsigned height, BOOL bRepaint, BOOL bForce);
	CWinThread	*m_pThread, *m_pThreadPSP;
	static UINT ThreadFunc(LPVOID);
	CDIBSection	m_dibVRAM;


	// 生成されたメッセージ マップ関数
	//{{AFX_MSG(CSampleDlgDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnHelp();
	afx_msg void OnDropFiles(HDROP hDropInfo);
	afx_msg void OnHelp2();
	afx_msg void OnClose();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ は前行の直前に追加の宣言を挿入します。

#endif // !defined(AFX_SAMPLEDLGDLG_H__738A5F16_13F1_40E6_919A_17F8835EDE9F__INCLUDED_)
