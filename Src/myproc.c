#include "myproc.h"
#include "pg.h"
#include "file.h"
#include "syscall.h"


#define			MAX_PATTERN		128		// �ǂݍ��񂾉摜�t�@�C��(�p�^�[��)���ɉ����ēK���ɐݒ肷�邱��


#define			FRAMESIZE		0x44000	//in byte	pg.c �ɂ���l
extern long		pg_drawframe;

//-----------------------------------------------------------------------------------------------------
#define	PAT_TYPE_RGB555		0			// 16bpp ���Ȃ�
#define	PAT_TYPE_RGB555A8	1			// 16bpp ������
#define	PAT_TYPE_INDEXED	2			// 8bpp
#define	PATDRAW_DIR_MASK	0x0000000F	// 00000000 00000000 00000000 00001111	(��������̃}�X�N) 

#define __swap(x,y) {int __tmp__ = x; x = y; y = __tmp__; }
#define SCREEN_TRUE_WIDTH	(SCREEN_WIDTH + 32)

typedef struct _pattern_info {
	int				datatype;
	int				width;				// �p�^�[��(�摜�S��)�T�C�Y
	int				height;
	int				chip_width;			// �`�b�v(�X)�T�C�Y
	int				chip_height;
	int				t_index;			// Indexed �p�^�[���̓����F
	unsigned short	*ptr;
	unsigned char	*ptrAlpha;			// RGB555 �p�^�[���̃�
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
//	������
//		�ŏ�������ŕK���Ă�ł��������B
//-----------------------------------------------------------------------------------------------------
void nDrawRoutineInit()
{
	setScreenOrigin(0, 0);
	setScreenClip(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	memset(gra_data, 0, sizeof(gra_data)); 
	makeMulTable();
}


//-----------------------------------------------------------------------------------------------------
// �p�^�[���Ǘ�
//		�摜�f�[�^�̊Ǘ�����ݒ肷��B
//
// �p�^�[���ƃ`�b�v�̒�`�F
//		:1 �̃t�@�C���̉摜�f�[�^�S��       = �p�^�[��
//		:�p�^�[���������A�c�ɕ����������ꂼ�� = �`�b�v
//
//		�`�b�v�͉������ɕ��сA�܂�Ԃ��B�Ⴆ�� 100 x 100 �̃p�^�[���� 2 x 2 �̃`�b�v�ɕ���
//		�����Ƃ��A�`�b�v�ԍ��� ���オ 0, �E�オ 1, ������ 2, �E���� 3 �ƂȂ�B
//		1 x 1 �ɕ�������΁A�p�^�[���� 1 �̃`�b�v�Ƃ��Ĉ�����B
//
//		�摜�f�[�^�� RGB555(A8) (�n�C�J���[) �� Indexed (256�F) �ɕ�������B
//		RGB555 �� 1 �s�N�Z�� 2 �o�C�g�ŁA���ォ�牡�����ɑ����������́B
//		RGB555A8 �� RGB555 �ɉ����� 1 �s�N�Z�� 1 �o�C�g�� �� �����B		
//		Indexed �� 1 �s�N�Z�� 1 �o�C�g�ŁA���ォ�牡�����ɑ����������́B
//			�ʓr�A�p���b�g�f�[�^ (2 �o�C�g�n�C�J���[ �� MAX 256 ��) ���K�v�B
//
//		num �͊Ǘ���̃p�^�[���ԍ�
//
//		[Indexed] �̏ꍇ
//			data_palette �̓p���b�g�f�[�^���i�[���ꂽ unsigned short �^�̔z��
//
//			data �̓s�N�Z���f�[�^���i�[���ꂽ unsigned char �^�̔z��
//
//		[RGB555] �̏ꍇ
//			data �̓s�N�Z���f�[�^���i�[���ꂽ unsigned short �^�̔z��
//
//		[RGB555A8] �̏ꍇ
//			data �̓s�N�Z���f�[�^���i�[���ꂽ unsigned short �^�̔z��
//
//			data_alpha �̓��f�[�^���i�[���ꂽ unsigned char �^�̔z��
//
//		�� data, data_palette, data_alpha �̈����̑g�ݍ��킹�Ə��Ԃɒ��ӂ��邱��
//		~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
//		w, h �̓p�^�[���S�̂̉��E�c�T�C�Y
//
//		pw, ph �́A�p�^�[�����̃`�b�v�̉��E�c�T�C�Y�B
//
//-----------------------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------------------
// Indexed �p�^�[���p
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
	gra_data[num].t_index			= -1;	// �f�t�H���g�ł͓����F�͖���

	if (pw == 0) pw = 1;
	if (ph == 0) ph = 1;
	gra_data[num].chip_width		= w / pw;
	gra_data[num].chip_height		= h / ph;

	return true;
}


//-----------------------------------------------------------------------------------------------------
// RGB555 �p�^�[���p
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
// RGB555A8 �p�^�[���p
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
// Indexed �p�^�[���̓����F�̐ݒ� (�ݒ肵�Ȃ���Ζ����ɂȂ��Ă���)
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
//	�Ȃ񂿂���ĕ`�惋�[�`��
//		�摜�f�[�^��\������B�����͌��x���x�Ɖ������`��
//
//	[�g����]
//		PSPDRAWSTRUCT dst;
//		dst.flag		= DRAW_NORMAL;	// �`����@���w�肷��
//
//			[�����ꂩ 1 ��]
//			DRAW_NORMAL			�ʏ�`��
//			DRAW_REV_UD			�㉺���]
//			DRAW_REV_LR			���E���]
//			�� DRAW_REV_LR | DRAW_REV_UD �Ƃ���Ώ㉺���E���]
//
//			[��Ƒg�ݍ��킹��]	
//			���w��				�ʏ�`��
//			DRAW_BRIGHTNESS		�P�x�ύX
//			DRAW_ALPHA			�ʏ�̔�����
//			DRAW_ADDALPHA		���Z������
//
//
//			[RGB555A8 �̏ꍇ]
//			DRAW_NOT_USE_PAT_ALPHA �� OR ����΁A���𖳎�����
//
//			[Indexed �̏ꍇ]
//			DRAW_NOT_USE_TCOLOR �� OR ����΁A�����F�𖳎�����
//
//		dst.patnum		=   0;	// �p�^�[���ԍ�
//		dst.chipnum		=   0;	// �`�b�v�ԍ�
//		dst.x			=   0;	// �����W
//		dst.y			=   0;	// �c���W
//		dst.param		=   0;	// �P�x�E�����x�E�A���t�@ (0 �` 255)
//
//		dst.zoomx		= 256;	// �������g��
//		dst.zoomy		= 256;	// �c�����g��
//			256 �œ��{ (x1)�A512 �� x2, 128 �� x1/2 �̂悤�� 256 ���
//
//		patDraw(&dst);			// �`��
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
//	�N���b�v�ݒ�
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
//	�N���b�v�擾
//-----------------------------------------------------------------------------------------------------
void getScreenClip(int *left, int *top, int *right, int *bottom)
{
	*left	= clipLeft;
	*top	= clipTop;
	*right	= clipRight;
	*bottom	= clipBottom;
}


//-----------------------------------------------------------------------------------------------------
//	�X�N���[�����W�̌��_�ʒu�ύX
//-----------------------------------------------------------------------------------------------------
void setScreenOrigin(int left, int top)
{
	if (left < 0) left = 0;
	if (top < 0) top = 0;

	originLeft	= left;
	originTop	= top;
}


//-----------------------------------------------------------------------------------------------------
//	�X�N���[�����W�̌��_�ʒu�擾
//-----------------------------------------------------------------------------------------------------
void getScreenOrigin(int *left, int *top)
{
	*left	= originLeft;
	*top	= originTop;
}


//-----------------------------------------------------------------------------------------------------
// �p�^�[���̃`�b�v���擾
//-----------------------------------------------------------------------------------------------------
int getPatChipWidth(int pat_num)
{
	return gra_data[pat_num].chip_width;
}


//-----------------------------------------------------------------------------------------------------
// �p�^�[���̃`�b�v�����擾
//-----------------------------------------------------------------------------------------------------
int getPatChipHeight(int pat_num)
{
	return gra_data[pat_num].chip_height;
}


//-----------------------------------------------------------------------------------------------------
// �o�C�i���t�@�C����ǂݍ���
//
//	�o�b�t�@ p �͂��炩���ߗp�ӂ��Ă������ƁBPSP �̏ꍇ�͔z��
//	�߂�l�͓ǂݍ��񂾃o�C�g��
//
//		PSP �̃t�@�C���p�X�́A�Ⴆ�΃������[�X�e�B�b�N��
//		/PSP/GAME/TEST �t�H���_�ɃC���X�g�[�������ꍇ��
//		MS0:/PSP/GAME/TEST �̂悤�ȃp�X�ɂȂ�B
//
//		xmain(argc, argv) �Ŏ��s�����ꍇ�́A���s�t�@�C���̃t���p�X��
//		argv �ɓn�����B���Ȃ킿
//		argv = "MS0:/PSP/GAME/TEST/EBOOT.PBP"
//		�ł���B
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
// �w�肵���͈͂�h��Ԃ��B���l�w��\
//-----------------------------------------------------------------------------------------------------
void fillBoxA(unsigned long x1, unsigned long y1, unsigned long x2, unsigned long y2, unsigned long color, int alpha)
{
	// �͈͊O�Ȃ�I��
	if ((int)x2 <	clipLeft)	return;
	if ((int)x1 >=	clipRight)	return;
	if ((int)y2 <	clipTop)	return;
	if ((int)y1 >=	clipBottom)	return;
	
	// ��ʏ�̕`���͂��߈ʒu
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
// �N���b�v�Ή���ʃN���A
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
//	�g���ŁF������`��
//		pg.c �� mh_print() �̊g���łł��B
//
//		�N���b�s���O�Ɗg��k���ɑΉ����Ă��܂��B�g��k���̃p�����[�^�͏c���Ɨ��ł��B
//		256 �œ��{�A512 �� x2�A 128 �� x1/2 �̂悤�ɂȂ��Ă��܂� (1 �` 122880)
//		�J���[ col �� -1 ��������A�F�w��v���t�B�N�X�𖳎����A�����F�ŏ����܂�
//-----------------------------------------------------------------------------------------------------
void mh_printEx(int x,int y,char *str,int col, unsigned int mag256X, unsigned int mag256Y)
{
	// �g��̂ق��� 122880 �ɏ����ݒ肵�Ă��邪�A122880 = 256 * 512�A�܂�
	// 1 �h�b�g�� 512 �h�b�g�܂Ŋg�傳���̂ŁA�f���I�ɂ���ȏ�͈Ӗ����Ȃ�����B
	// ���_��̐����ł͂Ȃ�
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
				// [@#CRRGGBB] �łȂ񂿂���ĕ����F�ύX
				//
				// "Hello [@#FF0000]PSP" �� PSP �ȍ~���ԐF�ɂȂ�B
				//
				//		[@ �Ɨ�����K�� #RRGGBB ������Ɖ���B
				//		"Hello P[@" �ŏI���Ȃ�����
				//		"[@" �Ŏn�܂��Ă����������
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
// �\��ʂ��痠��ʂ֓]��
//		bEnableClip = true �Ȃ�N���b�v�L��
//		bEnableClip = false �Ȃ�S��ʓ]��
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
// ���������[�`��


// ���Z�e�[�u���Ƃ�������
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



// font.c �� const ���O���Ă���...
extern unsigned char hankaku_font10[];
extern unsigned short zenkaku_font10[];
extern unsigned short font404[];


void Draw_Char_Hankaku_Ex(int x,int y,unsigned char ch,int col, unsigned int mag256X, unsigned int mag256Y)
{
	unsigned char  *fnt;
	unsigned char  pt;
	int x1,y1;

	// �͈͊O�Ȃ�I��
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

	// ��ʏ�̕`���͂��߈ʒu
	int xs = (x >= clipLeft)		? x : clipLeft;
	int ys = (y >= clipTop)			? y : clipTop;
	int xe = (x + mx < clipRight)	? x + mx : clipRight;
	int ye = (y + my < clipBottom)	? y + my : clipBottom;
	unsigned short *vptr = (unsigned short *)pgGetVramAddr(0, 0);
	vptr += xs + ys * SCREEN_TRUE_WIDTH;

	// �摜�f�[�^�̊J�n�ʒu
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
	
	// �͈͊O�Ȃ�I��
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

	// SJIS�R�[�h�̐���
	code = u;
	code = (code<<8) + d;
	
	// SJIS����EUC�ɕϊ�
	if(code >= 0xE000) code-=0x4000;
	code = ((((code>>8)&0xFF)-0x81)<<9) + (code&0x00FF);
	if((code & 0x00FF) >= 0x80) code--;
	if((code & 0x00FF) >= 0x9E) code+=0x62;
	else code-=0x40;
	code += 0x2121 + 0x8080;
	
	// EUC����b�����t�H���g�̔ԍ��𐶐�
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
						// Pixel �̃A���t�@��ς���
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
						pa = MulTable16s8[MulTable5x5[(pa << 8) + a]]; // �ŏI�I�ȃ�

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
					pa = *(pold_alpha + workbufX[x1 - xs]);	// Pixel�̃�
					if (pa == 0) {
						vptr++;
					} else
						if (pa == 255) {
						*vptr++ = *(pold_data + workbufX[x1 - xs]);
					} else {
						pa2 = 255 - pa;
						u = *(pold_data + workbufX[x1 - xs]);	// �����珑��Pixel RGB
						*vptr++ = atALPHA(vptr, u, pa, pa2);
					}
				}
				vptr +=  vXofs;
			}
		}
	
	}

}