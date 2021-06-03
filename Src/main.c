#include "main.h"
#include "myproc.h"
#include "mylib.h"

#include "image_myship.c"
#include "image_kuma.c"
#include "image_bomb.c"

#define PAT_MYSHIP			0
#define PAT_KUMA			1
#define PAT_BOMB			2

#define	PAT_MYSHIP_WIDTH	32
#define	PAT_MYSHIP_HEIGHT	32
#define	PAT_KUMA_WIDTH		73
#define	PAT_KUMA_HEIGHT		106
#define	PAT_BOMB_WIDTH		768
#define	PAT_BOMB_HEIGHT		192

#define PAT_KUMA_PALNUM		256
#define GAME_WIDTH			224

#define MAX_CHRS			64
#define	KUMA_FACE_COLOR		255
#define	KUMA_TCOLOR			0

#define COLOR_WHITE			0xFFFF
#define COLOR_BLACK			0x0000
#define COLOR_GRAY			RGB(96,  96,  96)
#define COLOR_RED			RGB(255, 0,   0)
#define COLOR_YELLOW		RGB(255, 255, 0)

#define SCREEN_TRUE_WIDTH	(SCREEN_WIDTH + 32)

#define MAX_ENEMY_PAL		11


#define random(x) (nrand() % (x))


typedef struct _character {
	int type;
	int x;
	int y;
	int direction;
	int zoom;
} character;


character		chr[MAX_CHRS];
unsigned short	new_pal[MAX_ENEMY_PAL][PAT_KUMA_PALNUM];	// 色違い用パレット
unsigned int	g_gamescore		= 0;
bool			g_bPushed		= true;
int				g_kuma_zoom_rate1, g_kuma_zoom_rate2;

void gameover();


void makeCharacter(int i)
{
	chr[i].x			= - (PAT_KUMA_WIDTH / 4) + random(GAME_WIDTH);
	chr[i].y			= - (PAT_KUMA_HEIGHT / 2) - random(16) * 25;

	chr[i].type			= abs(random(MAX_ENEMY_PAL - 1)) + 1;
	chr[i].direction	= (random(100) > 50) ? 1 : -1;
	chr[i].zoom			= 256 * (g_kuma_zoom_rate1 + random(g_kuma_zoom_rate2)) / 256;
}


bool wait_key(int keycode)
{
	if (paddata.buttons & keycode) {
		if (!g_bPushed) {
			g_bPushed = true;
			return true;;
		} else
			return false;
	} else {
		g_bPushed = false;
		return false;
	}
}



void enemy_move(int i, int movespeed)
{
	chr[i].y += movespeed;
	if (chr[i].y >= SCREEN_HEIGHT)
		makeCharacter(i);
}


// true なら当たってない
bool collisionCheck(int x, int y)
{
	// 自機中央の 4 ドットが COLOR_GRAY ならセーフという
	// 超せこい仕様。ターボで移動量が大きくてもわざと考慮しない
	unsigned short *vptr = (unsigned short *)pgGetVramAddr(0, 0);
	unsigned short judge_color = COLOR_GRAY;
	int l, t;
	getScreenOrigin(&l, &t);
	x += l;
	y += t;

	unsigned short c1 = *(vptr + (x + 0) + (y + 0) * SCREEN_TRUE_WIDTH);
	unsigned short c2 = *(vptr + (x + 1) + (y + 0) * SCREEN_TRUE_WIDTH);
	unsigned short c3 = *(vptr + (x + 0) + (y + 1) * SCREEN_TRUE_WIDTH);
	unsigned short c4 = *(vptr + (x + 1) + (y + 1) * SCREEN_TRUE_WIDTH);

	if (c1 == judge_color && c2 == judge_color && c3 == judge_color && c3 == judge_color)
		return true;

	return false;
}


void game()
{
	int wd = (SCREEN_WIDTH - GAME_WIDTH) / 2;
	setScreenClip(wd, 0, wd + GAME_WIDTH, SCREEN_HEIGHT);
	setScreenOrigin(wd, 0);

	int x = (GAME_WIDTH - PAT_MYSHIP_WIDTH) / 2, y=200;
	int nChrs = 8;
	int MOVE_STEP = 2, MOVE_STEP_ENEMY = 1;

	int i, nCountDead= -1, bWarning = false;
	g_kuma_zoom_rate1 = 48; g_kuma_zoom_rate2 = 80;
	for(i=0; i<MAX_CHRS; i++)
		makeCharacter(i);

	g_gamescore = 0;

	for(;;) {
		clearVram(COLOR_GRAY);

		PSPDRAWSTRUCT dst;

		// クマ
		for(i=0; i<nChrs; i++) {
			dst.flag	= DRAW_NORMAL | DRAW_ZOOMX | DRAW_ZOOMY;
			dst.patnum	= PAT_KUMA;
			dst.chipnum	= 0;
			dst.param	= 0;
			dst.x		= chr[i].x;
			dst.y		= chr[i].y;
			dst.zoomx	= chr[i].zoom;
			dst.zoomy	= dst.zoomx;
			if (chr[i].direction == -1)
				dst.flag |= DRAW_REV_LR;

			// パレット差し替え
			if (bWarning)
				setPattern_Indexed(PAT_KUMA, new_pal[0], (unsigned char *)image_kuma, PAT_KUMA_WIDTH, PAT_KUMA_HEIGHT, 1, 1);
			else
				setPattern_Indexed(PAT_KUMA, new_pal[chr[i].type], (unsigned char *)image_kuma, PAT_KUMA_WIDTH, PAT_KUMA_HEIGHT, 1, 1);
			setTransparentColor(PAT_KUMA, KUMA_TCOLOR);
			patDraw(&dst);

			enemy_move(i, MOVE_STEP_ENEMY);
		}

		// 自機
		if (nCountDead < 0) {
			// 普通の時
			int mv = 1;
			sceCtrlRead(&paddata, 1);
			if (paddata.buttons & CTRL_START)	 psp_exit();
			if (paddata.buttons & CTRL_CIRCLE)	 mv = 4;
			if (paddata.buttons & CTRL_LEFT)	 x += -MOVE_STEP * mv;
			if (paddata.buttons & CTRL_RIGHT)	 x +=  MOVE_STEP * mv;
			if (paddata.buttons & CTRL_DOWN)	 y +=  MOVE_STEP * mv;
			if (paddata.buttons & CTRL_UP)		 y += -MOVE_STEP * mv;
			if (x < (-PAT_MYSHIP_WIDTH / 2 + 1)) x = -PAT_MYSHIP_WIDTH / 2 + 1;
			if (x >= (GAME_WIDTH - PAT_MYSHIP_WIDTH / 2 - 1)) x = GAME_WIDTH - PAT_MYSHIP_WIDTH / 2 - 1;
			if (y < PAT_MYSHIP_HEIGHT) y = PAT_MYSHIP_HEIGHT;
			if (y >= (SCREEN_HEIGHT - PAT_MYSHIP_HEIGHT / 2 - 1)) y = SCREEN_HEIGHT - PAT_MYSHIP_HEIGHT / 2 - 1;

			// 衝突チェック
			if (collisionCheck(x + 15, y + 10) == true) {
				dst.flag	= DRAW_NORMAL;
				dst.patnum	= PAT_MYSHIP;
				dst.chipnum	= 0;
				dst.x		= x;
				dst.y		= y;
				patDraw(&dst);
				g_gamescore++;

				if ((g_gamescore % 150) == 0) {
					if (++g_kuma_zoom_rate1 >=96)
						g_kuma_zoom_rate1 =96;
				}
				if ((g_gamescore % 400) == 0) {
					if (++g_kuma_zoom_rate2 >= 80)
						g_kuma_zoom_rate2 =80;
				}

				if ((g_gamescore % 70) == 0) {
					if (++nChrs == MAX_CHRS - 2) {
						// 高速化する警告のため全キャラ赤くなる
						bWarning = true;
					}
					if (nChrs >= MAX_CHRS) {
						nChrs = MAX_CHRS;
						MOVE_STEP_ENEMY = 2;
					}
				}
			} else
				nCountDead = 0;
		}

		if (nCountDead >= 0)  {
			// 爆発中
			dst.flag	= DRAW_ADDALPHA | DRAW_ZOOMX | DRAW_ZOOMY;
			dst.patnum	= PAT_BOMB;
			dst.chipnum	= nCountDead / 2;
			dst.param	= 255;
			dst.x		= x - getPatChipWidth(PAT_BOMB) - 32;
			dst.y		= y - getPatChipHeight(PAT_BOMB) - 32;
			dst.zoomx	= 256 * 3;
			dst.zoomy	= 256 * 3;
			patDraw(&dst);
			if (++nCountDead >= 32) return;
		}

		// スコア表示
		char buf[64];
		nltoa(g_gamescore, arg_chars[0], 10);
		nsprintf(buf, "[SCORE] [@#FFFFFF]%s");
		mh_printEx(0, 0, buf, COLOR_RED, 384, 384);

		pgScreenFlipV();
	}
}


void title()
{
	setScreenClip(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
	setScreenOrigin(0, 0);
	clearVram(COLOR_BLACK);

	PSPDRAWSTRUCT dst;
	dst.flag	= DRAW_NORMAL | DRAW_ZOOMX | DRAW_ZOOMY | DRAW_REV_LR;
	dst.patnum	= PAT_KUMA;
	dst.chipnum	= 0;
	dst.param	= 128;
	dst.x		= 0;
	dst.y		= -150;
	dst.zoomx	= 2048;
	dst.zoomy	= dst.zoomx;
	patDraw(&dst);

	fillBoxA(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, RGB(0, 104, 201), 224);
			
	int u = 50, v = 20;
	char *s = "せまりくる[@#FF0000]クマ[@#FFFFFF]をかわし続けろ！";
	mh_printEx(u + 1, v +   1, s, -1, 512, 512);
	mh_printEx(u, v +   0, s, COLOR_WHITE, 512, 512);

	s = "当たり判定は[@#FF00FF]まんなかのへん[@#FFFFFF]にしかないから";
	mh_printEx(u + 1, v +  31, s, -1, 512, 512);
	mh_printEx(u, v +  30, s, COLOR_WHITE, 512, 512);

	s = "少々のかすりはＯＫだ！";
	mh_printEx(u + 1, v +  61, s, -1, 512, 512);
	mh_printEx(u, v +  60, s, COLOR_WHITE, 512, 512);

	s = "(かすっても点にはならないぞ)";
	mh_printEx(u + 1, v +  91, "", -1, 512, 512);
	mh_printEx(u, v +  90, "", RGB(255,0,255), 512, 512);

	s = "[@#FFFF00]●[@#FFFFFF]でターボだ！";
	mh_printEx(u + 1, v + 131, s, -1, 512, 512);
	mh_printEx(u, v + 130, s, COLOR_WHITE, 512, 512);

	s = "ちなみにハイスコアやエンディングなんて無いぞ！";
	mh_printEx(u + 1, v + 181, s, -1, 384, 384);
	mh_printEx(u, v + 180, s, RGB(190,226,63), 384, 384);

	mh_printEx(u + 270, v + 230, "○ でスタートします", COLOR_WHITE, 256, 256);

	pgScreenFlipV();

	g_bPushed = true;
	while(1) {
		sceCtrlRead(&paddata, 1);
		if (paddata.buttons & CTRL_START)	psp_exit();
		if (wait_key(CTRL_CIRCLE))			break;
		pgWaitV();
	}

	clearVram(COLOR_BLACK);
	pgScreenFlipV();
	clearVram(COLOR_BLACK);
	pgScreenFlipV();
}


void gameover()
{
	bltFronttoBack(false);
	fillBoxA(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_BLACK, 128);

	mh_printEx((GAME_WIDTH - 45 * 4) / 2, 50, "GAME OVER", COLOR_RED, 1024, 1024);
	char buf[30];
	nltoa(g_gamescore, arg_chars[0], 10);
	nsprintf(buf, "スコア: %s");
	mh_printEx(25, 100, buf, COLOR_WHITE, 512, 512);
	mh_printEx(89, 148, "●", COLOR_RED, 640, 640);
	mh_printEx((GAME_WIDTH - 70 * 2) / 2, 150, "PUSH ○ BUTTON", COLOR_YELLOW, 512, 512);
	mh_printEx((GAME_WIDTH - 70 * 2) / 2, 170, " to continue", COLOR_YELLOW, 512, 512);
	pgScreenFlipV();

	g_bPushed = true;
	while(1) {
		sceCtrlRead(&paddata, 1);
		if (paddata.buttons & CTRL_START) psp_exit();
		if (wait_key(CTRL_CIRCLE)) break;
		pgWaitV();
	}

	clearVram(COLOR_BLACK);
	pgScreenFlipV();
	clearVram(COLOR_BLACK);
	pgScreenFlipV();
}



int xmain(int argc, char *argv)
{
	// 初期化
	pgInit(); pgScreenFrame(2,0); sceCtrlInit(0); sceCtrlSetAnalogMode(0);

	// 描画ルーチン用初期化
	nDrawRoutineInit();

	// 画像登録
	setPattern_RGB555A8(PAT_MYSHIP, (unsigned short *)image_myship, (unsigned char *)image_myship_alpha, PAT_MYSHIP_WIDTH, PAT_MYSHIP_HEIGHT, 1, 1);
	setPattern_Indexed(PAT_KUMA, (unsigned short *)image_kuma_palette, (unsigned char *)image_kuma, PAT_KUMA_WIDTH, PAT_KUMA_HEIGHT, 1, 1);
	setTransparentColor(PAT_KUMA, KUMA_TCOLOR);	// 透明色の設定
	setPattern_RGB555A8(PAT_BOMB, (unsigned short *)image_bomb2, (unsigned char *)image_bomb2_alpha, PAT_BOMB_WIDTH, PAT_BOMB_HEIGHT, 8, 2);

	// 超適当な乱数初期化
	nrandomize(psp_get_time());

	// 色違い用パレット作成
	int i;
	int new_pal_data[] = {
		RGB(228,70,70), RGB(154, 173, 242), RGB(153, 243, 182 ), RGB(254, 139, 196), RGB(251, 228, 138), RGB(176, 237, 243),
		RGB(207, 171, 248), RGB(186, 186, 186), RGB(230, 165, 141), RGB(190, 168, 164), RGB(255, 255, 255)
	};
	for(i=0; i<MAX_ENEMY_PAL; i++) {
		memcpy(new_pal[i], image_kuma_palette, PAT_KUMA_PALNUM * 2);
		new_pal[i][KUMA_FACE_COLOR] = new_pal_data[i];
	}

	title();
	for(;;) {
		game();
		gameover();
	}

	return 0;
}
