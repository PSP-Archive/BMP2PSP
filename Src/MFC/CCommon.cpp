#include "stdafx.h"


// Windows 2000 ˆÈ~‚©’²‚×‚é
BOOL IsWin2000orLater(VOID)
{
	OSVERSIONINFO info;
	info.dwOSVersionInfoSize = sizeof(info);
	::GetVersionEx(&info);
	
	return (info.dwPlatformId == VER_PLATFORM_WIN32_NT) && (info.dwMajorVersion >= 5);
}
