#ifndef	___CCOMMON_H___
#define	___CCOMMON_H___


BOOL	IsWin2000orLater(VOID);
CString	GetNowTime();
DWORD	LittleEndianToBigEndian(DWORD dwVal);
CString	GetEXEFolder(VOID);
CString GetTempFolder(VOID);
CString GetFullPath(CString strFolder, CString strFile);


VOID	CreateShortCutAtStartUp(CString strLinkName); // strLinkName = \\Name.link
VOID	DeleteShortCutAtStartUp(CString strLinkName); // strLinkName = \\Name.link


#endif
