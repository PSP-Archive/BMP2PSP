#include "stdafx.h"
#include "CDIBSection.h"
#include "SampleDlgDlg.h"

#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <direct.h>
#include <time.h>
#include <mmsystem.h>

#include "../file.h"
#include "../pg.h"
#include "../pad.h"

#define	PSP_SCREEN_WIDTH	480
#define	PSP_SCREEN_HEIGHT	272

LPBYTE					g_lpVRAM;
static	CDIBSection		g_dib;
CSampleDlgDlg			*g_pWnd = NULL;
DWORD					g_mtime = 0;

extern BOOL				g_bExitThreadPSP;
extern DWORD			g_dwVSYNCCounter;
extern BOOL				g_bModeThread;

extern long		pg_showframe;
extern long		pg_screenmode;



// �T���v���ɂ���Ă� xmain(argc, argv) ������̂ŁA�K�������Ă�
int				xmain(int argc, char *argv);



UINT ThreadFuncPSP(LPVOID lpData)
{
	g_pWnd = (CSampleDlgDlg *)lpData;

	g_dib.Create((PSP_SCREEN_WIDTH + 32), PSP_SCREEN_HEIGHT, 15);

	// DIBSection �̃A�h���X
	g_lpVRAM = g_pWnd->GetDIBSection()->GetBits();

	TCHAR buf[MAX_PATH];
	::GetCurrentDirectory(MAX_PATH, buf);
	CString s = buf;
	s += _T("\\EBOOT.PBP");
	s.Replace('\\', '/');

	xmain(1, (char *)(LPCTSTR)s);		// �������� PSP �\�[�X�ցB

	return 0;
}


void PresentDisplay()
{
	CClientDC dc(g_pWnd);

	if (pg_screenmode == 0) {	// off
		dc.BitBlt(0, 0, PSP_SCREEN_WIDTH, PSP_SCREEN_HEIGHT, NULL, 0, 0, BLACKNESS);
		return;
	}
	
	CDIBSection *pDIBSec = g_pWnd->GetDIBSection();
	CDC dcMem, dcMem2;
	dcMem.CreateCompatibleDC(NULL);
	dcMem2.CreateCompatibleDC(NULL);
	dcMem.SelectObject(pDIBSec->GetBitmap());
	dcMem2.SelectObject(g_dib.GetBitmap());

	int y = 0;
	if (pg_screenmode == 2 && pg_showframe == 1)
		y = PSP_SCREEN_HEIGHT;

	// �܂� PSP DIBSec ���� g_dib �ɓ]��
	dcMem2.BitBlt(0, 0, PSP_SCREEN_WIDTH, PSP_SCREEN_HEIGHT, &dcMem, 0, y, SRCCOPY);


	// Win �� RGB555 -> 0RRR RRGG GGGB BBBB
	// PSP �� RGB555 -> 1BBB BBGG GGGR RRRR
	// RGB ����ւ�
	LPWORD pBuf = (LPWORD)(g_dib.GetBits());
	BYTE r, b;
	WORD g, w;
	LONG ll = (PSP_SCREEN_WIDTH + 32) * (PSP_SCREEN_HEIGHT);
	for(LONG l=0; l<ll; l++) {
		w = *pBuf;
		b = (w & 0x7C00) >> 10;
		g = (w & 0x03E0);	// �ǂ����V�t�g����̂����� >>5 �͕s�v (g ���� WORD �Ȃ̂�)
		r = (w & 0x001F);
		w = (r << 10) | g | b;
		*pBuf++ = w;
	}

	// ��ʂւ̍ŏI�]��
	dc.BitBlt(0, 0, PSP_SCREEN_WIDTH, PSP_SCREEN_HEIGHT, &dcMem2, 0, 0, SRCCOPY);
}



//-----------------------------------------------------------------------------------
// PSP �V�X�e���R�[���� Win32 �ł���炵����p


void sceDisplayWaitVblankStart()
{
	DWORD d = g_dwVSYNCCounter;
	int c = 0;
	while (d == g_dwVSYNCCounter) {
		if (c++ > 5) {
//			PresentDisplay();
			c = 0;
		}
		::Sleep(1);
	}
}


void sceDisplaySetMode(long mode,long width,long height)
{
	// �������Ȃ��ł������낤...
	// 480 x 272 �Œ�I
	if (width != PSP_SCREEN_WIDTH || height != PSP_SCREEN_HEIGHT) {
		AfxMessageBox(_T("480 x 272 ���[�h�����Ή����Ă܂ւ�B�I�����܂����Ă�����"));
		g_pWnd->PostMessage(WM_QUIT);
	}
}


void sceDisplaySetFrameBuf(char *topaddr,long linesize,long pixelsize,long)
{
	PresentDisplay();
}


// ���͋@��̏����ݒ�
void sceCtrlInit(int unknown)
{
	g_mtime = ::timeGetTime();
}


// �A�i���O���͋@��̏����ݒ�
void sceCtrlSetAnalogMode(int on)
{
	// �����_�ł͂Ȃ�����Ȃ�
}


// ���ԁA�A�i���O�͖������I�I�I�I
void sceCtrlRead(ctrl_data_t *paddata, int unknown)
{
	::ZeroMemory(paddata, sizeof(ctrl_data_t));
	if (0x8000 & ::GetAsyncKeyState(VK_F1)		) {paddata->buttons |= CTRL_START;}
	if (0x8000 & ::GetAsyncKeyState(VK_F2)		) {paddata->buttons |= CTRL_SELECT;}
	if (0x8000 & ::GetAsyncKeyState(VK_LSHIFT)	) {paddata->buttons |= CTRL_LTRIGGER;}
	if (0x8000 & ::GetAsyncKeyState(VK_RSHIFT)	) {paddata->buttons |= CTRL_RTRIGGER;}
	if (0x8000 & ::GetAsyncKeyState(VK_UP)		) {paddata->buttons |= CTRL_UP;}
	if (0x8000 & ::GetAsyncKeyState(VK_DOWN)	) {paddata->buttons |= CTRL_DOWN;}
	if (0x8000 & ::GetAsyncKeyState(VK_LEFT)	) {paddata->buttons |= CTRL_LEFT;}
	if (0x8000 & ::GetAsyncKeyState(VK_RIGHT)	) {paddata->buttons |= CTRL_RIGHT;}
	if (0x8000 & ::GetAsyncKeyState('A')	) {paddata->buttons |= CTRL_SQUARE;}
	if (0x8000 & ::GetAsyncKeyState('W')	) {paddata->buttons |= CTRL_TRIANGLE;}
	if (0x8000 & ::GetAsyncKeyState('D')	) {paddata->buttons |= CTRL_CIRCLE;}
	if (0x8000 & ::GetAsyncKeyState('S')	) {paddata->buttons |= CTRL_CROSS;}

	if (pg_screenmode == 1)
		PresentDisplay();
} 


int sceIoOpen(const char* file, int mode)
{
	CString s = file;
	int TRUEMODE;
	if (mode == O_RDONLY)	TRUEMODE = 0;
	if (mode == O_WRONLY)	TRUEMODE = 1;
	if (mode == O_RDWR)		TRUEMODE = 2;
	if (mode == O_NBLOCK)	TRUEMODE = -1;
	if (mode == O_APPEND)	TRUEMODE = 8;
	if (mode == O_CREAT)	TRUEMODE = 0x0100;
	if (mode == O_TRUNC)	TRUEMODE = 0x0200;
	if (mode == O_NOWAIT)	TRUEMODE = -1;

	if (TRUEMODE == -1) {
		AfxMessageBox( _T("sceIoOpen() �őΉ����Ă��Ȃ��������n����܂����B�I�����܂�"));
		g_pWnd->PostMessage(WM_QUIT);
		return -1;
	}

	int fd = _topen(s, TRUEMODE | 0x8000);
	return fd;
}

 
void sceIoClose(int fd)
{
	_close(fd);
}


int sceIoRead(int fd, void *data, int size)
{
	return _read(fd, data, size);
}

 
int sceIoWrite(int fd, void *data, int size)
{
	return _write(fd, data, size);
}

 
int sceIoLseek(int fd, int offset, int whence)
{
TRACE(_T("sceIoLseek() �͌��؂��Ă��܂���"));
	return lseek(fd, offset, whence);
}

 
int sceIoRemove(const char *file)
{
TRACE(_T("sceIoRemove() �͌��؂��Ă��܂���"));
	CString s = file;
	int l0 = s.GetLength(), l=l0;
	while(s.GetAt(--l) != '/');
	s = s.Right(l0 - l - 1);
	
	return ::DeleteFile(s);
}

 
int sceIoMkdir(const char *dir, int mode)
{
TRACE(_T("sceIoMkdir() �͌��؂��Ă��܂���"));
	CString s = dir;
	int l0 = s.GetLength(), l=l0;
	while(s.GetAt(--l) != '/');
	s = s.Right(l0 - l - 1);
	
	return _tmkdir(s);
}

 
int sceIoRmdir(const char *dir)
{
TRACE(_T("sceIoRmdir() �͌��؂��Ă��܂���"));
	CString s = dir;
	int l0 = s.GetLength(), l=l0;
	while(s.GetAt(--l) != '/');
	s = s.Right(l0 - l - 1);
	
	return _trmdir(s);
}

 
int sceIoRename(const char *oldname, const char *newname)
{
TRACE(_T("sceIoRename() �͌��؂��Ă��܂���"));
	CString s0 = oldname;
	int l0 = s0.GetLength(), l=l0;
	while(s0.GetAt(--l) != '/');
	s0 = s0.Right(l0 - l - 1);
	
	CString s1 = newname;
	l0 = s1.GetLength(); l=l0;
	while(s1.GetAt(--l) != '/');
	s1 = s1.Right(l0 - l - 1);

	return _trename(s0, s1);
}


static WIN32_FIND_DATA g_dt;
static BOOL bFirstFFF = FALSE; 
int  sceIoDopen(const char *fn)
{
	CString s = fn;
	if (!(s.Right(1) == _T("/") || s.Right(1) == _T("\\"))) {
		s += _T("/");
	}
	s += _T("*.*");

	HANDLE h = ::FindFirstFile(s, &g_dt);
	if (h == INVALID_HANDLE_VALUE) {
		AfxMessageBox(_T("sceIoDopen() �����s���܂����B�I�����܂�"));
		g_pWnd->PostMessage(WM_QUIT);
	}
	bFirstFFF = TRUE;
	return (int)h;
}


int  sceIoDread(int fd, dirent_t *de)
{
	if (bFirstFFF) {
		bFirstFFF = FALSE;
		::lstrcpy(de->name, g_dt.cFileName);
		de->unk0 = 0;
		de->size = g_dt.nFileSizeLow;
		for(int i=0; i<19; i++)
			de->unk[i] = 0;
		::ZeroMemory(de->dmy, 128);
		de->type = (g_dt.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? TYPE_DIR : TYPE_FILE;
		return 1;
	}

	WIN32_FIND_DATA dt;
	HANDLE h = (HANDLE)fd;
	BOOL b = ::FindNextFile(h, &dt);

	::lstrcpy(de->name, dt.cFileName);
	de->unk0 = 0;
	de->size = dt.nFileSizeLow;
	for(int i=0; i<19; i++)
		de->unk[i] = 0;
	::ZeroMemory(de->dmy, 128);
	de->type = (dt.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? TYPE_DIR : TYPE_FILE;

	return b;
}


void sceIoDclose(int fd)
{
	::FindClose((HANDLE)fd);
} 

 
//int sceIoDevctl(const char *name int cmd, void *arg, size_t arglen, void *buf, size_t *buflen); 


void __exit()
{
	if (g_bModeThread)
		::PostMessage(g_pWnd->m_hWnd, WM_QUIT, 0, 0);
}


void sceKernelExitGame()
{
	__exit();
}


unsigned int sceKernelLibcTime(void *dummy)
{
	time_t t;
	time(&t);
	return (unsigned int)t;
}


unsigned int sceKernelLibcClock()
{
	DWORD t = ::timeGetTime();
	return (unsigned int)(t - g_mtime);
}