#include "myproc.h"
#include "pg.h"
#include "file.h"
#include "syscall.h"


#define			MAX_PATTERN		128		// 読み込んだ画像ファイル(パターン)等に応じて適当に設定すること


#define			FRAMESIZE		0x44000	//in byte	pg.c にある値
extern long		pg_drawframe;

//-----------------------------------------------------------------------------------------------------
#define	PAT_TYPE_RGB555		0			// 16bpp αなし
#define	PAT_TYPE_RGB555A8	1			// 16bpp αあり
#define	PAT_TYPE_INDEXED	2			// 8bpp
#define	PATDRAW_DIR_MASK	0x0000000F	// 00000000 00000000 00000000 00001111	(方向決定のマスク) 

#define __swap(x,y) {int __tmp__ = x; x = y; y = __tmp__; }
#define SCREEN_TRUE_WIDTH	(SCREEN_WIDTH + 32)

typedef struct _pattern_info {
	int				datatype;
	int				width;				// パターン(画像全体)サイズ
	int				height;
	int				chip_width;			// チップ(個々)サイズ
	int				chip_height;
	int				t_index;			// Indexed パターンの透明色
	unsigned short	*ptr;
	unsigned char	*ptrAlpha;			// RGB555 パターンのα
	unsigned short	*ptrPalette;
} pattern_info;

static pattern_info			gra_data[MAX_PATTERN];

static int					clipLeft;
static int					clipRight;
static int					clipTop;
static int					clipBottom;
static int					originLeft;
static int					originTop;

static void					makeMulTable();
static int					strRRGGBBtoi(char *p);
static unsigned short		MulTable5x5[65536];		// BYTE * BYTE
static unsigned short		MulTable16s8[65536];	// WORD / 255
static int					workbufX[SCREEN_WIDTH];
static int					workbufY[SCREEN_HEIGHT];
static void					patDraw_RGB555(PSPDRAWSTRUCT *dst);		
static void					patDraw_RGB555A8(PSPDRAWSTRUCT *dst);
static void					patDraw_Indexed(PSPDRAWSTRUCT *dst);
static void					patDraw_Sub(PSPDRAWSTRUCT *dst);
static void					Draw_Char_Hankaku_Ex(int x,int y,unsigned char ch,int col, unsigned int mag256X, unsigned int mag256Y);
static void					Draw_Char_Zenkaku_Ex(int x,int y,unsigned char u,unsigned char d,int col, unsigned int mag256X, unsigned int mag256Y);




//-----------------------------------------------------------------------------------------------------
//	初期化
//		最初あたりで必ず呼んでください。
//-----------------------------------------------------------------------------------------------------
void nDrawRoutineInit()
{
	setScreenOrigin(0, 0);
	setScreenClip(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	memset(gra_data, 0, sizeof(gra_data)); 
	makeMulTable();
}


//-----------------------------------------------------------------------------------------------------
// パターン管理
//		画像データの管理情報を設定する。
//
// パターンとチップの定義：
//		:1 つのファイルの画像データ全体       = パターン
//		:パターン内を横、縦に分割したそれぞれ = チップ
//
//		チップは横方向に並び、折り返す。例えば 100 x 100 のパターンを 2 x 2 のチップに分割
//		したとき、チップ番号は 左上が 0, 右上が 1, 左下が 2, 右下が 3 となる。
//		1 x 1 に分割すれば、パターンを 1 個のチップとして扱える。
//
//		画像データは RGB555(A8) (ハイカラー) と Indexed (256色) に分けられる。
//		RGB555 は 1 ピクセル 2 バイトで、左上から横方向に走査したもの。
//		RGB555A8 は RGB555 に加えて 1 ピクセル 1 バイトの α を持つ。		
//		Indexed は 1 ピクセル 1 バイトで、左上から横方向に走査したもの。
//			別途、パレットデータ (2 バイトハイカラー で MAX 256 個) が必要。
//
//		num は管理上のパターン番号
//
//		[Indexed] の場合
//			data_palette はパレットデータが格納された unsigned short 型の配列
//
//			data はピクセルデータが格納された unsigned char 型の配列
//
//		[RGB555] の場合
//			data はピクセルデータが格納された unsigned short 型の配列
//
//		[RGB555A8] の場合
//			data はピクセルデータが格納された unsigned short 型の配列
//
//			data_alpha はαデータが格納された unsigned char 型の配列
//
//		※ data, data_palette, data_alpha の引数の組み合わせと順番に注意すること
//		~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//		w, h はパターン全体の横・縦サイズ
//
//		pw, ph は、パターン内のチップの横・縦サイズ。
//
//-----------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------
// Indexed パターン用
//-----------------------------------------------------------------------------------------------------
bool setPattern_Indexed(int num, unsigned short *data_palette, unsigned char *data, int w, int h, int pw, int ph)
{
	if (num <0 || num >=MAX_PATTERN)
		return false;

	gra_data[num].datatype			= PAT_TYPE_INDEXED;
	gra_data[num].ptr				= (unsigned short *)data;
	gra_data[num].ptrPalette		= data_palette;
	gra_data[num].width				= w;
	gra_data[num].height			= h;
	gra_data[num].t_index			= -1;	// デフォルトでは透明色は無効

	if (pw == 0) pw = 1;
	if (ph == 0) ph = 1;
	gra_data[num].chip_width		= w / pw;
	gra_data[num].chip_height		= h / ph;

	return true;
}


//-----------------------------------------------------------------------------------------------------
// RGB555 パターン用
//-----------------------------------------------------------------------------------------------------
bool setPattern_RGB555(int num, unsigned short *data, int w, int h, int pw, int ph)
{
	if (num <0 || num >=MAX_PATTERN)
		return false;

	gra_data[num].datatype			= PAT_TYPE_RGB555;
	gra_data[num].ptr				= data;
	gra_data[num].width				= w;
	gra_data[num].height			= h;

	if (pw == 0) pw = 1;
	if (ph == 0) ph = 1;
	gra_data[num].chip_width		= w / pw;
	gra_data[num].chip_height		= h / ph;

	return true;
}


//-----------------------------------------------------------------------------------------------------
// RGB555A8 パターン用
//-----------------------------------------------------------------------------------------------------
bool setPattern_RGB555A8(int num, unsigned short *data, unsigned char *data_aplha, int w, int h, int pw, int ph)
{
	if (num <0 || num >=MAX_PATTERN)
		return false;

	gra_data[num].datatype			= PAT_TYPE_RGB555A8;
	gra_data[num].ptr				= data;
	gra_data[num].ptrAlpha			= data_aplha;
	gra_data[num].width				= w;
	gra_data[num].height			= h;

	if (pw == 0) pw = 1;
	if (ph == 0) ph = 1;
	gra_data[num].chip_width		= w / pw;
	gra_data[num].chip_height		= h / ph;

	return true;
}


//-----------------------------------------------------------------------------------------------------
// Indexed パターンの透明色の設定 (設定しなければ無効になっている)
//-----------------------------------------------------------------------------------------------------
bool setTransparentColor(int num, int t_color)
{
	if (num <0 || num >=MAX_PATTERN)
		return false;

	if (gra_data[num].datatype == PAT_TYPE_INDEXED) {
		gra_data[num].t_index	= t_color;
		return true;
	}
	return false;
}


//-----------------------------------------------------------------------------------------------------
//	なんちゃって描画ルーチン
//		画像データを表示する。特徴は激遅速度と怪しい描画
//
//	[使い方]
//		PSPDRAWSTRUCT dst;
//		dst.flag		= DRAW_NORMAL;	// 描画方法を指定する
//
//			[いずれか 1 つ]
//			DRAW_NORMAL			通常描画
//			DRAW_REV_UD			上下反転
//			DRAW_REV_LR			左右反転
//			※ DRAW_REV_LR | DRAW_REV_UD とすれば上下左右反転
//
//			[上と組み合わせる]	
//			無指定				通常描画
//			DRAW_BRIGHTNESS		輝度変更
//			DRAW_ALPHA			通常の半透明
//			DRAW_ADDALPHA		加算半透明
//
//
//			[RGB555A8 の場合]
//			DRAW_NOT_USE_PAT_ALPHA を OR すれば、αを無視する
//
//			[Indexed の場合]
//			DRAW_NOT_USE_TCOLOR を OR すれば、透明色を無視する
//
//		dst.patnum		=   0;	// パターン番号
//		dst.chipnum		=   0;	// チップ番号
//		dst.x			=   0;	// 横座標
//		dst.y			=   0;	// 縦座標
//		dst.param		=   0;	// 輝度・透明度・アルファ (0 ～ 255)
//
//		dst.zoomx		= 256;	// 横方向拡大
//		dst.zoomy		= 256;	// 縦方向拡大
//			256 で等倍 (x1)、512 で x2, 128 で x1/2 のように 256 が基準
//
//		patDraw(&dst);			// 描画
//
//-----------------------------------------------------------------------------------------------------
void patDraw(PSPDRAWSTRUCT *old_dst)
{
	PSPDRAWSTRUCT dst;
	memcpy(&dst, old_dst, sizeof(dst));
	dst.x	+= originLeft;
	dst.y	+= originTop;

	patDraw_Sub(&dst);
}


//-----------------------------------------------------------------------------------------------------
//	クリップ設定
//-----------------------------------------------------------------------------------------------------
void setScreenClip(int left, int top, int right, int bottom)
{
	if (left < 0) left = 0;
	if (right > SCREEN_WIDTH) right = SCREEN_WIDTH;
	if (top < 0) top = 0;
	if (bottom > SCREEN_HEIGHT) bottom = SCREEN_HEIGHT;

	if (left > right) __swap(left, right);
	if (top > bottom) __swap(top, bottom);

	clipLeft	= left;
	clipTop		= top;
	clipRight	= right;
	clipBottom	= bottom;
}


//-----------------------------------------------------------------------------------------------------
//	クリップ取得
//-----------------------------------------------------------------------------------------------------
void getScreenClip(int *left, int *top, int *right, int *bottom)
{
	*left	= clipLeft;
	*top	= clipTop;
	*right	= clipRight;
	*bottom	= clipBottom;
}


//-----------------------------------------------------------------------------------------------------
//	スクリーン座標の原点位置変更
//-----------------------------------------------------------------------------------------------------
void setScreenOrigin(int left, int top)
{
	if (left < 0) left = 0;
	if (top < 0) top = 0;

	originLeft	= left;
	originTop	= top;
}


//-----------------------------------------------------------------------------------------------------
//	スクリーン座標の原点位置取得
//-----------------------------------------------------------------------------------------------------
void getScreenOrigin(int *left, int *top)
{
	*left	= originLeft;
	*top	= originTop;
}


//-----------------------------------------------------------------------------------------------------
// パターンのチップ幅取得
//-----------------------------------------------------------------------------------------------------
int getPatChipWidth(int pat_num)
{
	return gra_data[pat_num].chip_width;
}


//-----------------------------------------------------------------------------------------------------
// パターンのチップ高さ取得
//-----------------------------------------------------------------------------------------------------
int getPatChipHeight(int pat_num)
{
	return gra_data[pat_num].chip_height;
}


//-----------------------------------------------------------------------------------------------------
// バイナリファイルを読み込む
//
//	バッファ p はあらかじめ用意しておくこと。PSP の場合は配列
//	戻り値は読み込んだバイト数
//
//		PSP のファイルパスは、例えばメモリースティックの
//		/PSP/GAME/TEST フォルダにインストールした場合は
//		MS0:/PSP/GAME/TEST のようなパスになる。
//
//		xmain(argc, argv) で実行した場合は、実行ファイルのフルパスが
//		argv に渡される。すなわち
//		argv = "MS0:/PSP/GAME/TEST/EBOOT.PBP"
//		である。
//	
//-----------------------------------------------------------------------------------------------------
long loadBinaryFile(char *file, unsigned char *p, long size)
{
	if (file == NULL)
		return 0;

	long result = 0;
	int fd = sceIoOpen(file, O_RDONLY);
	if (fd < 0)
		return 0;
	result = sceIoRead(fd, p, size);
	sceIoClose(fd);

	return result;
}


//-----------------------------------------------------------------------------------------------------
// 指定した範囲を塗りつぶす。α値指定可能
//-----------------------------------------------------------------------------------------------------
void fillBoxA(unsigned long x1, unsigned long y1, unsigned long x2, unsigned long y2, unsigned long color, int alpha)
{
	// 範囲外なら終了
	if ((int)x2 <	clipLeft)	return;
	if ((int)x1 >=	clipRight)	return;
	if ((int)y2 <	clipTop)	return;
	if ((int)y1 >=	clipBottom)	return;
	
	// 画面上の描きはじめ位置
	int xs = ((int)x1 >= clipLeft) ? x1 : clipLeft;
	int ys = ((int)y1 >= clipTop) ? y1 : clipTop;
	int xe = ((int)x2 < clipRight)	? x2 : clipRight;
	int ye = ((int)y2 < clipBottom)	? y2 : clipBottom;

	unsigned short r, g, b, r0, g0, b0, u0, b1, r1, g1;
	unsigned short *vptr = (unsigned short *)pgGetVramAddr(0, 0);
	unsigned short c = (unsigned short)color;
	vptr += xs + ys * SCREEN_TRUE_WIDTH;
	int vofs = xe - xs - SCREEN_TRUE_WIDTH;

	int x0, y0, a = (int)alpha, a2 = 255 - a;

	if (alpha == 255 || alpha < 0) {
		for(y0=ys; y0<ye; y0++) {
			for(x0=xs; x0<xe; x0++) {
				*vptr++ = c;
			}
			vptr -= vofs;
		} 
	} else {
		b1 = (c & 0x7C00) >> 10;
		g1 = (c & 0x03E0) >> 5;
		r1 = (c & 0x001F);

		for(y0=ys; y0<ye; y0++) {
			for(x0=xs; x0<xe; x0++) {

				u0 = *vptr;
				b0 = (u0 & 0x7C00) >> 10;
				g0 = (u0 & 0x03E0) >> 5;
				r0 = (u0 & 0x001F);

				// b = (b * alpha + b0 * (255 - alpha)) / 255;
				b = MulTable16s8[MulTable5x5[(b1 << 8) + a] + MulTable5x5[(b0 << 8) + a2]];
				g = MulTable16s8[MulTable5x5[(g1 << 8) + a] + MulTable5x5[(g0 << 8) + a2]];
				r = MulTable16s8[MulTable5x5[(r1 << 8) + a] + MulTable5x5[(r0 << 8) + a2]];
				*vptr++ = (b << 10) | (g << 5) | r;
			}
			vptr -= vofs;
		} 
	}
}


//-----------------------------------------------------------------------------------------------------
// クリップ対応画面クリア
//-----------------------------------------------------------------------------------------------------
void clearVram(unsigned long color)
{
	unsigned short *vptr = (unsigned short *)pgGetVramAddr(0, 0);
	unsigned short c = (unsigned short)color;
	vptr += clipLeft + clipTop * SCREEN_TRUE_WIDTH;

	int x0, y0;
	for(y0 = clipTop; y0 < clipBottom; y0++) {
		for(x0 = clipLeft; x0<clipRight; x0++)
			*vptr++ = c;

		vptr -= clipRight - clipLeft - SCREEN_TRUE_WIDTH;
	} 
}



//-----------------------------------------------------------------------------------------------------
//	拡張版：文字列描画
//		pg.c の mh_print() の拡張版です。
//
//		クリッピングと拡大縮小に対応しています。拡大縮小のパラメータは縦横独立です。
//		256 で等倍、512 で x2、 128 で x1/2 のようになっています (1 ～ 122880)
//		カラー col が -1 だったら、色指定プレフィクスを無視し、かつ黒色で書きます
//-----------------------------------------------------------------------------------------------------
void mh_printEx(int x,int y,char *str,int col, unsigned int mag256X, unsigned int mag256Y)
{
	// 拡大のほうは 122880 に上限を設定しているが、122880 = 256 * 512、つまり
	// 1 ドットが 512 ドットまで拡大されるので、映像的にそれ以上は意味がないから。
	// 理論上の制限ではない
	if (mag256X == 0 || mag256X > 122880)
		return;
	if (mag256Y == 0 || mag256Y > 122880)
		return;

	x += originLeft;
	y += originTop;

	unsigned char ch = 0,bef = 0;
	bool bNoPrefix = false;
	if (col < 0) {
		bNoPrefix = true;
		col = RGB(0,0,0);
	}

	int mz = (10 * (int)mag256X) >> 8, mh = mz >> 1;

	while(*str != 0) {
		ch = (unsigned char)*str++;
		if (bef!=0) {
			Draw_Char_Zenkaku_Ex(x,y,bef,ch,col, mag256X, mag256Y);
			x += mz;
			bef=0;
		} else {
			if (((ch>=0x80) && (ch<0xa0)) || (ch>=0xe0)) {
				bef = ch;
			} else {
				// [@#CRRGGBB] でなんちゃって文字色変更
				//
				// "Hello [@#FF0000]PSP" で PSP 以降が赤色になる。
				//
				//		[@ と来たら必ず #RRGGBB があると仮定。
				//		"Hello P[@" で終わらないこと
				//		"[@" で始まっても無視される
				if (*(str + 0) == '@') {
					if (*(str - 1) == '[') {
						if (!bNoPrefix)
							col = strRRGGBBtoi(str + 2);
						str += 9;
						continue;
					}
				}
				Draw_Char_Hankaku_Ex(x,y,ch,col, mag256X, mag256Y);
				x += mh;
			}
		}
	}
}


//-----------------------------------------------------------------------------------------------------
// 表画面から裏画面へ転送
//		bEnableClip = true ならクリップ有効
//		bEnableClip = false なら全画面転送
//-----------------------------------------------------------------------------------------------------
void bltFronttoBack(bool bEnableClip)
{
	unsigned short *vptrSrc, *vptrDst = (unsigned short *)pgGetVramAddr(0, 0);

	if (pg_drawframe) {
		vptrSrc = vptrDst - FRAMESIZE / 2;
	} else {
		vptrSrc = vptrDst + FRAMESIZE / 2;
	}

	int l, r, t, b;
	if (bEnableClip) {
		getScreenClip(&l, &t, &r, &b);
		vptrDst += l + t * SCREEN_TRUE_WIDTH;
		vptrSrc += l + t * SCREEN_TRUE_WIDTH;
	} else {
		l = t = 0;
		r = SCREEN_WIDTH;
		b = SCREEN_HEIGHT;
	}

	int x0, y0;
	for(y0 = t; y0 < b; y0++) {
		for(x0 = l; x0 < r; x0++)
			*vptrDst++ = *vptrSrc++;

		vptrDst -= r - l - SCREEN_TRUE_WIDTH;
		vptrSrc -= r - l - SCREEN_TRUE_WIDTH;
	} 
}





//-----------------------------------------------------------------------------------------------------
// 下働きルーチン


// 演算テーブルとか初期化
static void makeMulTable()
{
	unsigned short i, j;
	long l;
	for(l=0; l<65536; l++)
		MulTable16s8[l] = l / 255;

	for(j=0; j<256; j++)
		for(i=0; i<256; i++)
			MulTable5x5[(j << 8) + i] = i * j;
}



// font.c の const を外しておく...
extern unsigned char hankaku_font10[];
extern unsigned short zenkaku_font10[];
extern unsigned short font404[];


void Draw_Char_Hankaku_Ex(int x,int y,unsigned char ch,int col, unsigned int mag256X, unsigned int mag256Y)
{
	unsigned char  *fnt;
	unsigned char  pt;
	int x1,y1;

	// 範囲外なら終了
	int a, b, mx, my;
	int magX = mag256X, magY = mag256Y;
	a = magX >> 8;				//mag256 / 256;
	b = 5 * (magX - (a << 8)) >> 8;	//10 * (mag256 - (a * 256)) / 256;
	mx = 5 * a + b;
	a = magY >> 8;				//mag256 / 256;
	b = 10 * (magY - (a << 8)) >> 8;	//10 * (mag256 - (a * 256)) / 256;
	my = 10 * a + b;

	if (mx < 1 || my < 1)		return;
	if (x + mx <	clipLeft)	return;
	if (x >=		clipRight)	return;
	if (y + my <	clipTop)	return;
	if (y >=		clipBottom)	return;

	// mapping
	if (ch<0x20)
		ch = 0;
	else if (ch<0x80)
		ch -= 0x20;
	else if (ch<0xa0)
		ch = 0;
	else
		ch -= 0x40;
	
	fnt = (unsigned char *)&hankaku_font10[ch*10];
	unsigned short c = (unsigned short)col;

	// 画面上の描きはじめ位置
	int xs = (x >= clipLeft)		? x : clipLeft;
	int ys = (y >= clipTop)			? y : clipTop;
	int xe = (x + mx < clipRight)	? x + mx : clipRight;
	int ye = (y + my < clipBottom)	? y + my : clipBottom;
	unsigned short *vptr = (unsigned short *)pgGetVramAddr(0, 0);
	vptr += xs + ys * SCREEN_TRUE_WIDTH;

	// 画像データの開始位置
	int tx = (x >= clipLeft) ? 0 : clipLeft - x;
	int ty = (y >= clipTop) ? 0 : clipTop - y;

	int uu;;
	for(uu=0; uu<xe - xs; uu++)
		workbufX[uu] = ((uu + tx) << 8) / magX;

	for(uu=0; uu<ye - ys; uu++)
		workbufY[uu] = ((uu + ty) << 8) / magY;

	for(y1=ys; y1<ye; y1++) {
		pt = fnt[workbufY[y1 - ys]];
		for(x1=xs; x1<xe; x1++) {
			if ((pt >> workbufX[x1 - xs]) & 1)
				*vptr = c;

			vptr++; 
		}
		vptr -= xe - xs - SCREEN_TRUE_WIDTH;
	}
}


void Draw_Char_Zenkaku_Ex(int x,int y,unsigned char u,unsigned char d,int col, unsigned int mag256X, unsigned int mag256Y)
{
	unsigned short *fnt;
	unsigned short pt;
	int x1, y1;
	
	unsigned long n;
	unsigned short code;
	int j;
	
	// 範囲外なら終了
	int a, b, mx, my;
	int magX = mag256X, magY = mag256Y;
	a = magX >> 8;				//mag256 / 256;
	b = 10 * (magX - (a << 8)) >> 8;	//10 * (mag256 - (a * 256)) / 256;
	mx = 10 * a + b;
	a = magY >> 8;				//mag256 / 256;
	b = 10 * (magY - (a << 8)) >> 8;	//10 * (mag256 - (a * 256)) / 256;
	my = 10 * a + b;

	if (mx < 1 || my < 1)		return;
	if (x + mx <	clipLeft)	return;
	if (x >=		clipRight)	return;
	if (y + my <	clipTop)	return;
	if (y >=		clipBottom)	return;

	// SJISコードの生成
	code = u;
	code = (code<<8) + d;
	
	// SJISからEUCに変換
	if(code >= 0xE000) code-=0x4000;
	code = ((((code>>8)&0xFF)-0x81)<<9) + (code&0x00FF);
	if((code & 0x00FF) >= 0x80) code--;
	if((code & 0x00FF) >= 0x9E) code+=0x62;
	else code-=0x40;
	code += 0x2121 + 0x8080;
	
	// EUCから恵梨沙フォントの番号を生成
	n = (((code>>8)&0xFF)-0xA1)*(0xFF-0xA1)	+ (code&0xFF)-0xA1;

	j=0;
	while(font404[j]) {
		if(code >= font404[j]) {
			if(code <= font404[j]+font404[j+1]-1) {
				n = -1;
				break;
			} else {
				n-=font404[j+1];
			}
		}
		j+=2;
	}
	fnt = (unsigned short *)&zenkaku_font10[n*10];
	
	unsigned short c = (unsigned short)col;

	int xs = (x >= clipLeft)		? x : clipLeft;
	int ys = (y >= clipTop)			? y : clipTop;
	int xe = (x + mx < clipRight)	? x + mx : clipRight;
	int ye = (y + my < clipBottom)	? y + my : clipBottom;
	unsigned short *vptr = (unsigned short *)pgGetVramAddr(0, 0);
	vptr += xs + ys * SCREEN_TRUE_WIDTH;

	int tx = (x >= clipLeft) ? 0 : clipLeft - x;
	int ty = (y >= clipTop) ? 0 : clipTop - y;

	int uu;;
	for(uu=0; uu<xe - xs; uu++)
		workbufX[uu] = ((uu + tx) << 8) / magX;

	for(uu=0; uu<ye - ys; uu++)
		workbufY[uu] = ((uu + ty) << 8) / magY;

	for(y1=ys; y1<ye; y1++) {
		pt = fnt[workbufY[y1 - ys]];
		for(x1=xs; x1<xe; x1++) {
			if ((pt >> (workbufX[x1 - xs])) & 1)
				*vptr = c;

			vptr++; 
		}
		vptr -= xe - xs - SCREEN_TRUE_WIDTH;
	}
}


int strRRGGBBtoi(char *p)
{
	int c = 0;
	unsigned char buf[10];
	memcpy(buf, p, 6);
	int i = -1;

	char c1 = 0, c2 = 0;
	unsigned short r = 0, g = 0, b = 0;

	for(i=0; i<6; i+=2) {
		c1 = buf[i];
		if (c1 >= 'A' && c1 <= 'F') c1 -= 'A' - 10;
		else
		if (c1 >= 'a' && c1 <= 'f') c1 -= 'a' - 10;
		else
		if (c1 >= '0' && c1 <= '9') c1 -= '0';
		c2 = buf[i+1];
		if (c2 >= 'A' && c2 <= 'F') c2 -= 'A' - 10;
		else
		if (c2 >= 'a' && c2 <= 'f') c2 -= 'a' - 10;
		else
		if (c2 >= '0' && c2 <= '9') c2 -= '0';

		if (i==0)
			r = (unsigned short) (c2 << 4) | c1;
		if (i==2)
			g = (unsigned short) (c2 << 4) | c1;
		if (i==4)
			b = (unsigned short) (c2 << 4) | c1;
	}

	return RGB(r, g, b);
}


unsigned short inline atBRIGHTNESS(unsigned short u, int a)
{
	unsigned short b, g, r;
	b = (u & 0x7C00) >> 10;
	g = (u & 0x03E0) >> 5;
	r = (u & 0x001F);

	b = MulTable16s8[MulTable5x5[(b << 8) + a]];
	g = MulTable16s8[MulTable5x5[(g << 8) + a]];
	r = MulTable16s8[MulTable5x5[(r << 8) + a]];

	return (b << 10) | (g << 5) | r;
}


unsigned short inline atALPHA(unsigned short *vptr, unsigned short u, int a, int a2)
{
	unsigned short b, g, r, u0, b0, g0, r0;
	b = (u & 0x7C00) >> 10;
	g = (u & 0x03E0) >> 5;
	r = (u & 0x001F);

	u0 = *vptr;
	b0 = (u0 & 0x7C00) >> 10;
	g0 = (u0 & 0x03E0) >> 5;
	r0 = (u0 & 0x001F);

	b = MulTable16s8[MulTable5x5[(b << 8) + a] + MulTable5x5[(b0 << 8) + a2]];
	g = MulTable16s8[MulTable5x5[(g << 8) + a] + MulTable5x5[(g0 << 8) + a2]];
	r = MulTable16s8[MulTable5x5[(r << 8) + a] + MulTable5x5[(r0 << 8) + a2]];

	return (b << 10) | (g << 5) | r;
}


unsigned short inline atADDALPHA(unsigned short *vptr, unsigned short u, int a)
{
	unsigned short b, g, r, u0, b0, g0, r0;
	b = (u & 0x7C00) >> 10;
	g = (u & 0x03E0) >> 5;
	r = (u & 0x001F);

	u0 = *vptr;
	b0 = (u0 & 0x7C00) >> 10;
	g0 = (u0 & 0x03E0) >> 5;
	r0 = (u0 & 0x001F);

	b = MulTable16s8[MulTable5x5[(b << 8) + a] + MulTable5x5[(b0 << 8) + 255]];
	g = MulTable16s8[MulTable5x5[(g << 8) + a] + MulTable5x5[(g0 << 8) + 255]];
	r = MulTable16s8[MulTable5x5[(r << 8) + a] + MulTable5x5[(r0 << 8) + 255]];
	if (b > 31) b = 31;
	if (r > 31) r = 31;
	if (g > 31) g = 31;

	return (b << 10) | (g << 5) | r;
}


void patDraw_Sub(PSPDRAWSTRUCT *dst)
{
	if (dst->flag & DRAW_ZOOMX) {
		if (dst->zoomx <= 0) return;
	} else
		dst->zoomx = 256;

	if (dst->flag & DRAW_ZOOMY) {
		if (dst->zoomy <= 0) return;
	} else
		dst->zoomy = 256;

	unsigned short *data = gra_data[dst->patnum].ptr;
	int	w	= gra_data[dst->patnum].width;
	int	h	= gra_data[dst->patnum].height;
	int	pw	= gra_data[dst->patnum].chip_width;
	int	ph	= gra_data[dst->patnum].chip_height;
	int cn	= dst->chipnum;
	int pn	= dst->patnum;
	if (pw == w && ph == h) { cn = 0; }

	int ta, tb, mx, my;
	int magX = dst->zoomx, magY = dst->zoomy;
	ta = magX >> 8;				//mag256 / 256;
	tb = pw * (magX - (ta << 8)) >> 8;	//10 * (mag256 - (a * 256)) / 256;
	mx = pw * ta + tb;
	ta = magY >> 8;				//mag256 / 256;
	tb = ph * (magY - (ta << 8)) >> 8;	//10 * (mag256 - (a * 256)) / 256;
	my = ph * ta + tb;

	if (dst->x + mx <	clipLeft)	return;
	if (dst->x >=		clipRight)	return;
	if (dst->y + my <	clipTop)	return;
	if (dst->y >=		clipBottom)	return;

	int xs = (dst->x >= clipLeft)		? dst->x : clipLeft;
	int ys = (dst->y >= clipTop)		? dst->y : clipTop;
	int xe = (dst->x + mx < clipRight)	? dst->x + mx : clipRight;
	int ye = (dst->y + my < clipBottom)	? dst->y + my : clipBottom;

	int tx = (dst->x >= clipLeft) ? 0 : clipLeft - (dst->x);
	int ty = (dst->y >= clipTop) ? 0 : clipTop - (dst->y);

	unsigned short *vptr = (unsigned short *)pgGetVramAddr(0, 0);

	int vXofs = SCREEN_TRUE_WIDTH - (xe - xs);
	int x1, y1, yOfs, a, a2, tcolor = gra_data[pn].t_index, dd, pa, pa2;
	unsigned short r, g, b, u, r0, g0, b0, u0;

	bool bBRIGHTNESS			= (dst->flag & DRAW_BRIGHTNESS) > 0 ? true : false;
	bool bALPHA				= (dst->flag & DRAW_ALPHA) > 0 ? true : false;
	bool bADDALPHA			= (dst->flag & DRAW_ADDALPHA) > 0 ? true : false;
	bool bNOT_USE_PAT_ALPHA	= (dst->flag & DRAW_NOT_USE_PAT_ALPHA) > 0 ? 1 : 0;
	bool bNOT_USE_TCOLOR		= (tcolor < 0) ? 1 : ((dst->flag & DRAW_NOT_USE_TCOLOR) ? 1 : 0);
	unsigned int flag = dst->flag & PATDRAW_DIR_MASK;

	unsigned char  *pAlpha			= gra_data[pn].ptrAlpha;
	unsigned char  *pold_alpha		= pAlpha;
	unsigned short *pPal			= gra_data[pn].ptrPalette;
	unsigned char  *dataIDX			= (unsigned char  *)data;
	unsigned short *pold_data		= data;
	unsigned char  *pold_dataIDX	= dataIDX;

	vptr += xs + ys * SCREEN_TRUE_WIDTH;
	int add_ofs = (cn / (w / pw) * ph * w) + (pw * (cn % (w / pw))); 
	data += add_ofs; 
	pAlpha += add_ofs;

	int uu;;
	bool bZX = dst->zoomx != 256;
	bool bZY = dst->zoomy != 256;

	if (flag == DRAW_NORMAL) {
		for(uu=0; uu<xe - xs; uu++)
			workbufX[uu] = bZX ? (((uu + tx) << 8) / dst->zoomx) : (uu + tx);

		for(uu=0; uu<ye - ys; uu++)
			workbufY[uu] = bZY ? (((uu + ty) << 8) / dst->zoomy) : (uu + ty);
	} else
	if (flag == DRAW_REV_LR) {
		for(uu=0; uu<xe - xs; uu++)
			workbufX[uu] = bZX ? ((pw - 1) - ((uu + tx) << 8) / dst->zoomx) : ((pw - 1) - (uu + tx));

		for(uu=0; uu<ye - ys; uu++)
			workbufY[uu] = bZY ? (((uu + ty) << 8) / dst->zoomy) : (uu + ty);
	} else
	if (flag == DRAW_REV_UD) {
		for(uu=0; uu<xe - xs; uu++)
			workbufX[uu] = bZX ? (((uu + tx) << 8) / dst->zoomx) : (uu + tx);

		for(uu=0; uu<ye - ys; uu++)
			workbufY[uu] = bZY ? ((ph - 1) - ((uu + ty) << 8) / dst->zoomy) : ((ph - 1) - (uu + ty));
	} else
	if (flag == (DRAW_REV_LR | DRAW_REV_UD)) {
		for(uu=0; uu<xe - xs; uu++)
			workbufX[uu] = bZX ? ((pw - 1) - ((uu + tx) << 8) / dst->zoomx) : ((pw - 1) - (uu + tx));

		for(uu=0; uu<ye - ys; uu++)
			workbufY[uu] = bZY ? ((ph - 1) - ((uu + ty) << 8) / dst->zoomy) : ((ph - 1) - (uu + ty));
	}


	if (gra_data[pn].datatype == PAT_TYPE_INDEXED) {
		a = dst->param; a2 = 255 - a;
		if (bBRIGHTNESS) {
			if (bNOT_USE_TCOLOR) {
				for(y1=ys; y1<ye; y1++) {
					yOfs = workbufY[y1 - ys];
					pold_dataIDX = dataIDX + w * yOfs;
					for(x1=xs; x1<xe; x1++) {
						dd = *(pold_dataIDX + workbufX[x1 - xs]);
						u = pPal[dd];
						*vptr++ = atBRIGHTNESS(u, a);
					}
					vptr +=  vXofs;
				}
			} else {
				for(y1=ys; y1<ye; y1++) {
					yOfs = workbufY[y1 - ys];
					pold_dataIDX = dataIDX + w * yOfs;
					for(x1=xs; x1<xe; x1++) {
						dd = *(pold_dataIDX + workbufX[x1 - xs]);
						if (tcolor == dd) {
							vptr++;
							continue;
						}
						u = pPal[dd];
						*vptr++ = atBRIGHTNESS(u, a);
					}
					vptr +=  vXofs;
				}
			}
		} else
		if (bALPHA) {
			if (bNOT_USE_TCOLOR) {
				for(y1=ys; y1<ye; y1++) {
					yOfs = workbufY[y1 - ys];
					pold_dataIDX = dataIDX + w * yOfs;
					for(x1=xs; x1<xe; x1++) {
						dd = *(pold_dataIDX + workbufX[x1 - xs]);
						u = pPal[dd];
						*vptr++ = atALPHA(vptr, u, a, a2);
					}
					vptr +=  vXofs;
				}
			} else {
				for(y1=ys; y1<ye; y1++) {
					yOfs = workbufY[y1 - ys];
					pold_dataIDX = dataIDX + w * yOfs;
					for(x1=xs; x1<xe; x1++) {
						dd = *(pold_dataIDX + workbufX[x1 - xs]);
						if (tcolor == dd) {
							vptr++;
							continue;
						}
						u = pPal[dd];
						*vptr++ = atALPHA(vptr, u, a, a2);
					}
					vptr +=  vXofs;
				}
			}
		} else
		if (bADDALPHA) {
			if (bNOT_USE_TCOLOR) {
				for(y1=ys; y1<ye; y1++) {
					yOfs = workbufY[y1 - ys];
					pold_dataIDX = dataIDX + w * yOfs;
					for(x1=xs; x1<xe; x1++) {
						dd = *(pold_dataIDX + workbufX[x1 - xs]);
						u = pPal[dd];
						*vptr++ = atADDALPHA(vptr, u, a);
					}
					vptr +=  vXofs;
				}
			} else {
				for(y1=ys; y1<ye; y1++) {
					yOfs = workbufY[y1 - ys];
					pold_dataIDX = dataIDX + w * yOfs;
					for(x1=xs; x1<xe; x1++) {
						dd = *(pold_dataIDX + workbufX[x1 - xs]);
						if (tcolor == dd) {
							vptr++;
							continue;
						}
						u = pPal[dd];
						*vptr++ = atADDALPHA(vptr, u, a);
					}
					vptr +=  vXofs;
				}
			}
		} else {
			if (bNOT_USE_TCOLOR) {
				for(y1=ys; y1<ye; y1++) {
					yOfs = workbufY[y1 - ys];
					pold_dataIDX = dataIDX + w * yOfs;
					for(x1=xs; x1<xe; x1++) {
						dd = *(pold_dataIDX + workbufX[x1 - xs]);
						*vptr++ = pPal[dd];
					}
					vptr +=  vXofs;
				}
			} else {
				for(y1=ys; y1<ye; y1++) {
					yOfs = workbufY[y1 - ys];
					pold_dataIDX = dataIDX + w * yOfs;
					for(x1=xs; x1<xe; x1++) {
						dd = *(pold_dataIDX + workbufX[x1 - xs]);
						if (tcolor == dd) {
							vptr++;
							continue;
						}
						*vptr++ = pPal[dd];
					}
					vptr +=  vXofs;
				}
			}
		}
	} else

	if (gra_data[pn].datatype == PAT_TYPE_RGB555 || bNOT_USE_PAT_ALPHA) {
		a = dst->param; a2 = 255 - a;
		if (bBRIGHTNESS) {
			for(y1=ys; y1<ye; y1++) {
				yOfs = workbufY[y1 - ys];
				pold_data = data + w * yOfs;
				for(x1=xs; x1<xe; x1++) {
					u = *(pold_data + workbufX[x1 - xs]);
					*vptr++ = atBRIGHTNESS(u, a);
				}
				vptr +=  vXofs;
			}
		} else
		if (bALPHA) {
			for(y1=ys; y1<ye; y1++) {
				yOfs = workbufY[y1 - ys];
				pold_data = data + w * yOfs;
				for(x1=xs; x1<xe; x1++) {
					u = *(pold_data + workbufX[x1 - xs]);
					*vptr++ = atALPHA(vptr, u, a, a2);
				}
				vptr +=  vXofs;
			}
		} else
		if (bADDALPHA) {
			for(y1=ys; y1<ye; y1++) {
				yOfs = workbufY[y1 - ys];
				pold_data = data + w * yOfs;
				for(x1=xs; x1<xe; x1++) {
					u = *(pold_data + workbufX[x1 - xs]);
					*vptr++ = atADDALPHA(vptr, u, a);
				}
				vptr +=  vXofs;
			}
		} else {
			for(y1=ys; y1<ye; y1++) {
				yOfs = workbufY[y1 - ys];
				pold_data = data + w * yOfs;
				for(x1=xs; x1<xe; x1++) {
					*vptr++ = *(pold_data + workbufX[x1 - xs]);
				}
				vptr +=  vXofs;
			}
		}
	} else

	if (gra_data[pn].datatype == PAT_TYPE_RGB555A8) {
		a = dst->param; a2 = 255 - a;
		if (bBRIGHTNESS) {
			for(y1=ys; y1<ye; y1++) {
				yOfs = workbufY[y1 - ys];
				pold_data = data + w * yOfs;
				pold_alpha = pAlpha + w * yOfs;
				for(x1=xs; x1<xe; x1++) {
					pa = *(pold_alpha + workbufX[x1 - xs]);
					if (pa == 0) {
						vptr++;
					} else {
						pa2 = 255 - pa;

						u = *(pold_data + workbufX[x1 - xs]);
						b = (u & 0x7C00) >> 10;
						g = (u & 0x03E0) >> 5;
						r = (u & 0x001F);
						b = MulTable16s8[MulTable5x5[(b << 8) + a]];	// b' = b * a (a:0-1) 
						g = MulTable16s8[MulTable5x5[(g << 8) + a]];
						r = MulTable16s8[MulTable5x5[(r << 8) + a]];

						u0 = *vptr;
						b0 = (u0 & 0x7C00) >> 10;
						g0 = (u0 & 0x03E0) >> 5;
						r0 = (u0 & 0x001F);

						b = MulTable16s8[MulTable5x5[(b << 8) + pa] + MulTable5x5[(b0 << 8) + pa2]];	// b'' = b' * pa + bv * (1 - pa)
						r = MulTable16s8[MulTable5x5[(r << 8) + pa] + MulTable5x5[(r0 << 8) + pa2]];
						g = MulTable16s8[MulTable5x5[(g << 8) + pa] + MulTable5x5[(g0 << 8) + pa2]];
						if (b > 31) b = 31;
						if (r > 31) r = 31;
						if (g > 31) g = 31;
						*vptr++ = (b << 10) | (g << 5) | r;
					}

				}
				vptr +=  vXofs;
			}
		} else
		if (bALPHA) {
			for(y1=ys; y1<ye; y1++) {
				yOfs = workbufY[y1 - ys];
				pold_data = data + w * yOfs;
				pold_alpha = pAlpha + w * yOfs;
				for(x1=xs; x1<xe; x1++) {
					pa = *(pold_alpha + workbufX[x1 - xs]);
					if (pa == 0) {
						vptr++;
					} else {
						// Pixel のアルファを変える
						pa = MulTable16s8[MulTable5x5[(pa << 8) + a]];	// pa = pa * a (pa, a:0-1)
						pa2 = 255 - pa;

						u = *(pold_data + workbufX[x1 - xs]);
						b = (u & 0x7C00) >> 10;
						g = (u & 0x03E0) >> 5;
						r = (u & 0x001F);

						u0 = *vptr;
						b0 = (u0 & 0x7C00) >> 10;
						g0 = (u0 & 0x03E0) >> 5;
						r0 = (u0 & 0x001F);

						b = MulTable16s8[MulTable5x5[(b << 8) + pa] + MulTable5x5[(b0 << 8) + pa2]]; 
						r = MulTable16s8[MulTable5x5[(r << 8) + pa] + MulTable5x5[(r0 << 8) + pa2]]; 
						g = MulTable16s8[MulTable5x5[(g << 8) + pa] + MulTable5x5[(g0 << 8) + pa2]]; 
						if (b > 31) b = 31;
						if (r > 31) r = 31;
						if (g > 31) g = 31;
						*vptr++ = (b << 10) | (g << 5) | r;
					}
				}
				vptr +=  vXofs;
			}
		} else
		if (bADDALPHA) {
			for(y1=ys; y1<ye; y1++) {
				yOfs = workbufY[y1 - ys];
				pold_data = data + w * yOfs;
				pold_alpha = pAlpha + w * yOfs;
				for(x1=xs; x1<xe; x1++) {
					pa = *(pold_alpha + workbufX[x1 - xs]);
					if (pa == 0) {
						vptr++;
					} else {
						pa = MulTable16s8[MulTable5x5[(pa << 8) + a]]; // 最終的なα

						u = *(pold_data + workbufX[x1 - xs]);
						b = (u & 0x7C00) >> 10;
						g = (u & 0x03E0) >> 5;
						r = (u & 0x001F);

						u0 = *vptr;
						b0 = (u0 & 0x7C00) >> 10;
						g0 = (u0 & 0x03E0) >> 5;
						r0 = (u0 & 0x001F);

						b = MulTable16s8[MulTable5x5[(b << 8) + pa] + MulTable5x5[(b0 << 8) + 255]]; 
						r = MulTable16s8[MulTable5x5[(r << 8) + pa] + MulTable5x5[(r0 << 8) + 255]]; 
						g = MulTable16s8[MulTable5x5[(g << 8) + pa] + MulTable5x5[(g0 << 8) + 255]]; 
						if (b > 31) b = 31;
						if (r > 31) r = 31;
						if (g > 31) g = 31;
						*vptr++ = (b << 10) | (g << 5) | r;
					}
				}
				vptr +=  vXofs;
			}
		} else {
			for(y1=ys; y1<ye; y1++) {
				yOfs = workbufY[y1 - ys];
				pold_data = data + w * yOfs;
				pold_alpha = pAlpha + w * yOfs;
				for(x1=xs; x1<xe; x1++) {
					pa = *(pold_alpha + workbufX[x1 - xs]);	// Pixelのα
					if (pa == 0) {
						vptr++;
					} else
						if (pa == 255) {
						*vptr++ = *(pold_data + workbufX[x1 - xs]);
					} else {
						pa2 = 255 - pa;
						u = *(pold_data + workbufX[x1 - xs]);	// 今から書くPixel RGB
						*vptr++ = atALPHA(vptr, u, pa, pa2);
					}
				}
				vptr +=  vXofs;
			}
		}
	
	}

}
