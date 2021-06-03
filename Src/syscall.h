#ifndef __PSP_SYSCALL_H
#define __PSP_SYSCALL_H

#include "pad.h"
#include "file.h"


/* Not 100% sure on this, init with 0 */ 
void sceCtrlInit(int unknown); 

/* Pass 1 to enable analogue mode */ 
void sceCtrlSetAnalogMode(int on); 

/* Read in the controller data. Unknown should be set to 1 */ 
void sceCtrlRead(ctrl_data_t* paddata, int unknown); 


void sceDisplayWaitVblankStart();
void sceDisplaySetMode(long,long,long);
void sceDisplaySetFrameBuf(char *topaddr,long linesize,long pixelsize,long);


int  sceIoOpen(const char* file, int mode); 
void sceIoClose(int fd); 
int  sceIoRead(int fd, void *data, int size); 
int  sceIoWrite(int fd, void *data, int size); 
int  sceIoLseek(int fd, int offset, int whence); 
int  sceIoRemove(const char *file); 
int  sceIoMkdir(const char *dir, int mode); 
int  sceIoRmdir(const char *dir); 
int  sceIoRename(const char *oldname, const char *newname); 
//int sceIoDevctl(const char *name int cmd, void *arg, size_t arglen, void *buf, size_t *buflen); 
int  sceIoDopen(const char *fn); 
int  sceIoDread(int fd, dirent_t *de); 
void sceIoDclose(int fd); 


void sceKernelExitGame();


// 1970/01/01 00:00:00 からの経過秒を返すみたい
// 戻り値はおそらく 32bit 
// 引数は不明なのでとりあえず 0 で (笑)
unsigned int sceKernelLibcTime(void *dummy);

// アプリ起動時からの経過時間をミリ秒で
unsigned int sceKernelLibcClock();


/* 未実装群...
STUB_FUNC   0x71EC4271,sceKernelLibcGettimeofday 
STUB_FUNC   0x79D1C3FA,sceKernelDcacheWritebackAll 


void sceAudio_0();//
void sceAudio_1();//
long sceAudio_2(long, long, long, void *);//
long sceAudio_3(long, long samplecount, long);//init buffer? returns handle, minus if error
void sceAudio_4(long handle);//free buffer?
void sceAudio_5();//
long sceAudio_6(long, long);//
void sceAudio_7();//
void sceAudio_8();//

typedef int (*pg_threadfunc_t)(int args, void *argp);

int sceKernelCreateThread(const char *name, pg_threadfunc_t func, unsigned long priority, unsigned long stacksize, unsigned long attrib, void *unk);
int sceKernelStartThread(int hthread, int arg0, void *arg1);
void sceKernelExitThread(int ret);
int sceKernelWaitThreadEnd(int hthread, void *unk);
int sceKernelDeleteThread(int hthread);
*/


#endif
