/*
	無いと非常に困るので適当に作成。
*/


//-------------------------------------------------
// 描画以外の PSP システムコールをラップ
void			psp_exit();
unsigned int	psp_get_time();		// 1970/01/01 からの経過秒を秒で取得
unsigned int	psp_get_clock();	// アプリが起動してからの経過時間をミリ秒で取得





//-------------------------------------------------
// 標準 C 関数のなんちゃって版


#ifndef RAND_MAX
#define RAND_MAX 32767
#endif


extern char arg_chars[][128];


// なんちゃって sprintf
// 文字列のみ受け取る ("%s" のみ !)
// 文字列はグローバル配列 arg_chars[8][128] に入れておく
int nsprintf(char *buf, char *fmt);


// なんちゃて randomize
void nrandomize(unsigned int seed);


// なんちゃて rand
int nrand( void );


// なんちゃって ltoa
char *nltoa( long value, char *string, int radix );
