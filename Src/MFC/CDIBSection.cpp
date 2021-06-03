#include "stdafx.h"
#include "CDIBSection.h"


CDIBSection::CDIBSection() {
	m_hBmp			= NULL;
	m_pPtr			= NULL;
	m_nWidth		= 0;
	m_nWidthBytes	= 0;
	m_nHeight		= 0;
	m_nBitsPixel	= 0;
	m_nPlanes		= 0;
	m_pbmInfo		= NULL;
}


CDIBSection::~CDIBSection() {
	Destroy();
}


VOID CDIBSection::Destroy() {
	if (m_hBmp) {
		::DeleteObject(m_hBmp);
		m_hBmp			= NULL;
		m_pPtr			= NULL;
		m_nWidth		= 0;
		m_nWidthBytes	= 0;
		m_nHeight		= 0;
		m_nBitsPixel	= 0;
		m_nPlanes		= 0;
		m_bmpBmp.Detach();
		m_bmpBmp.DeleteObject();
	}

	if (m_pbmInfo) {
		delete m_pbmInfo;
		m_pbmInfo = NULL;
	}
}



// 任意サイズのを作成
// 今回は RGB555 専用ということで。
BOOL CDIBSection::Create(int nWidth, int nHeight, int nBitCount, int nPlanes)
{
	Destroy();

	if (nBitCount != 15)
		return FALSE;

	m_pbmInfo		= (BITMAPINFO *)malloc(sizeof(BITMAPINFO) + sizeof(RGBQUAD) * 64);

	BITMAPINFOHEADER bi;
	bi.biSize			= sizeof(BITMAPINFOHEADER);
	bi.biWidth			= nWidth;
	bi.biHeight			= -nHeight;
	bi.biPlanes			= nPlanes;
	bi.biBitCount		= 16;
	bi.biCompression	= BI_BITFIELDS;
	bi.biSizeImage		= 0;
	bi.biXPelsPerMeter	= 0;
	bi.biYPelsPerMeter	= 0;
	bi.biClrUsed		= 0;
	bi.biClrImportant	= 0;

	m_pbmInfo->bmiHeader	= bi;
	LPDWORD lpMASK = (LPDWORD)m_pbmInfo->bmiColors;
	lpMASK[0] = 0x7C00; lpMASK[1] = 0x03E0;	// RGB555
//		lpMASK[0] = 0xF800;	lpMASK[1] = 0x07E0;	// RGB565
	lpMASK[2] = 0x001F;

	m_hBmp = ::CreateDIBSection(NULL, m_pbmInfo, DIB_RGB_COLORS, (LPVOID *)&m_pPtr, NULL, 0);
	if (!m_hBmp) {
		Destroy();
		return FALSE;
	}

	BITMAP bm;
	::GetObject(m_hBmp, sizeof(bm), &bm);

	m_nWidthBytes	= bm.bmWidthBytes;
	m_nWidth		= bm.bmWidth;
	m_nHeight		= bm.bmHeight;
	m_nBitsPixel	= bm.bmBitsPixel;
	m_nPlanes		= bm.bmPlanes;

	m_bmpBmp.Attach(m_hBmp);

	return TRUE;
}


//---------------------------------------------------------
// 今回関係のないメソッドは消しておきます。
BOOL CDIBSection::Create(CString strBMPFile)
{
	return FALSE;
}


VOID CDIBSection::FillBitsWithAlpha()
{
}


CDIBSection *CDIBSection::Duplicate(CDIBSection *pSrc)
{
	return NULL;
}


BOOL CDIBSection::ReSize(int nWidth, int nHeight)
{
	return FALSE;
}


BOOL CDIBSection::SaveBMP(CString strFileName)
{
	return FALSE;
}


BOOL CDIBSection::WriteIcon(CString strFile, CObArray *parrayDIBs)
{
	return FALSE;
}



BOOL CDIBSection::ReSize2(int nWidth, int nHeight)
{
	return FALSE;
}
