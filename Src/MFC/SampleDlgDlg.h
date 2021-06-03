// SampleDlgDlg.h : �w�b�_�[ �t�@�C��
//

#if !defined(AFX_SAMPLEDLGDLG_H__738A5F16_13F1_40E6_919A_17F8835EDE9F__INCLUDED_)
#define AFX_SAMPLEDLGDLG_H__738A5F16_13F1_40E6_919A_17F8835EDE9F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CSampleDlgDlg �_�C�A���O



class CSampleDlgDlg : public CDialog
{
// �\�z
public:
	VOID MainRoutine(VOID);
	CSampleDlgDlg(CWnd* pParent = NULL);	// �W���̃R���X�g���N�^

// �_�C�A���O �f�[�^
	//{{AFX_DATA(CSampleDlgDlg)
	enum { IDD = IDD_SAMPLEDLG_DIALOG };
		// ����: ���̈ʒu�� ClassWizard �ɂ���ăf�[�^ �����o���ǉ�����܂��B
	//}}AFX_DATA

	// ClassWizard �͉��z�֐��̃I�[�o�[���C�h�𐶐����܂��B
	//{{AFX_VIRTUAL(CSampleDlgDlg)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV �̃T�|�[�g
	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	//}}AFX_VIRTUAL

// �C���v�������e�[�V����
public:
	CDIBSection	*GetDIBSection(VOID) {return &m_dibVRAM;}

protected:
	HICON	m_hIcon;
	HACCEL	m_hAccel;

	BOOL		SetClientSize(unsigned width, unsigned height, BOOL bRepaint, BOOL bForce);
	CWinThread	*m_pThread, *m_pThreadPSP;
	static UINT ThreadFunc(LPVOID);
	CDIBSection	m_dibVRAM;


	// �������ꂽ���b�Z�[�W �}�b�v�֐�
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
// Microsoft Visual C++ �͑O�s�̒��O�ɒǉ��̐錾��}�����܂��B

#endif // !defined(AFX_SAMPLEDLGDLG_H__738A5F16_13F1_40E6_919A_17F8835EDE9F__INCLUDED_)