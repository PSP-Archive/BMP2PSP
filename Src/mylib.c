#include "mylib.h"
#include "types.h"
#include "string.h"
#include "syscall.h"



void psp_exit()
{
	sceKernelExitGame();
}


unsigned int psp_get_time()
{
	return (unsigned int)sceKernelLibcTime((void *) 0);
}


unsigned int psp_get_clock()
{
	return (unsigned int)sceKernelLibcClock();
}




// 128 バイトまでの文字列 8 個までを表示する
char arg_chars[8][128];

// %s のみ対応
int nsprintf(char *buf, char *fmt)
{
	if (buf == NULL) return 0;
	if (fmt == NULL) return 0;

	char *str = fmt;
	char *dst = buf;
	int argcount = 0;
	unsigned char ch = 0, bef = 0;
	while(*str != 0) {
		ch = *str++;
		if (bef != 0) {
			// 全角
			*(dst + 0) = bef;
			*(dst + 1) = ch;
			*(dst + 2) = '\0';
			dst+=2;
			bef = 0;
		} else {
			if (((ch >= 0x80) && (ch < 0xa0)) || (ch >= 0xe0)) {
				bef = ch;
			} else {
				// 半角
				if (ch == '%') {
					// %s
					if (*str == 's') {
						char *gs = arg_chars[argcount++];
						int l = strlen(gs);
						strcat(buf, gs);
						dst+=l;
						*dst = '\0';
						str ++;
					}
				} else {
					*dst = ch;
					*(dst + 1) = '\0';
					dst++;
				}
			}
		}
	}

	return strlen(buf);
}


static unsigned int rnd_seed = 0xB59FB701;	// 適当


// 0 ～ RAND_MAX(=32767) の値を返す
int nrand( void )
{
//	x(n + 1) = ax(n) + c
	unsigned int a = 79920567;
	unsigned int c = 93195199;
	rnd_seed = a * rnd_seed + c;

	int r = (int)(rnd_seed % RAND_MAX) ;
	return r;
}


void nrandomize(unsigned int seed)
{
	rnd_seed = seed;
}


char *nltoa(long value, char *string, int radix)
{
	if (string == NULL)
		return NULL;

	*string = '\0';

	int count = 0;

	long v = value;
	if (v < 0) {
		*string = '-';
		*(string + 1) = '\0';
		v = -v;
	}

	char bb[] = "0";
	int a = 0;
	do {
		a = v % radix;
		bb[0] = '0' + a;
		strcat(string, bb);
		count++;
		v /= radix;
	} while(v > 0);
	strcat(string, "");

	int i=0, j=0;
	if (*string == '-')
		j = 1;
	for(i=0; i<count/2; i++) {
		char u = *(string + i + j);
		*(string + i + j) = *(string + count - 1 - i +j);
		*(string + count - 1 - i + j) = u;
	}

	return string;
}
