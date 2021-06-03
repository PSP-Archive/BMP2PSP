#include "stdafx.h"


extern TCHAR	SZ_APP_TITLE[];
extern TCHAR	SZ_APP_TITLE2[];


CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


#include "resource.h"

BOOL CAboutDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	CWnd *pWnd = NULL;
	pWnd = GetDlgItem(IDC_STATIC_VERNAME);
	if (pWnd)
		pWnd->SetWindowText(SZ_APP_TITLE);

	pWnd = GetDlgItem(IDC_STATIC_VERDATE);
	if (pWnd)
		pWnd->SetWindowText(SZ_APP_TITLE2);

	return TRUE;
}
