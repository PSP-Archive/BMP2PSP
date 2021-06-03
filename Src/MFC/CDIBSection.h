//	CDIBSection 1.1.2


#ifndef __DIBSECTION__FOR_BITMAP__
#define __DIBSECTION__FOR_BITMAP__


class CDIBSection {
private:
	BITMAPINFO			*m_pbmInfo;
	LPBYTE				m_pPtr;
	int					m_nWidthBytes;
	int					m_nWidth;
	int					m_nHeight;
	int					m_nBitsPixel;
	int					m_nPlanes;
	HBITMAP				m_hBmp;

	VOID				Destroy();


public:
	CBitmap				m_bmpBmp;

	CDIBSection();
	~CDIBSection();

	BOOL				Create(int nWidth, int nHeight, int nBitCount, int nPlanes = 1);
	BOOL				Create(CString strBMPFile);


	CBitmap				*GetBitmap()	{return &m_bmpBmp;};
	int					GetWidthBytes()	{return m_nWidthBytes;};
	int					GetWidth()		{return m_nWidth;};
	int					GetHeight()		{return m_nHeight;};
	int					GetBitPixel()	{return m_nBitsPixel;};
	int					GetPlanes()		{return m_nPlanes;};
	LPBYTE				GetBits()		{return m_pPtr;};

	BOOL				ReSize(int nWidth, int nHeight);
	BOOL				ReSize2(int nWidth, int nHeight);
	BOOL				SaveBMP(CString strFileName);

	VOID				FillBitsWithAlpha();	// RGBA‚ÌA‚ÅRGB‚ð–„‚ß‚é

	static CDIBSection	*Duplicate(CDIBSection *pSrc);
	static BOOL			WriteIcon(CString strFile, CObArray *parrayDIBs);
};


/*
// ƒTƒ“ƒvƒ‹
#include "CDIBSection.h"

void CxxxDlg::MyFunc() 
{
	CDIBSection dib;
	if (!dib.Create(_T("D:\\Tmp\\256x256x32_A.bmp")))
		return;

	CDC			dcMem;
	dcMem.CreateCompatibleDC(NULL);
	pOld = dcMem.SelectObject(dib.GetBitmap());

	CClientDC	dc(this);
	dc.BitBlt(16, 16, dib.GetWidth(), dib.GetHeight(), &dcMem, 0, 0, SRCCOPY); 

	dcMem.SelectObject(pOld);
}
*/
#endif
