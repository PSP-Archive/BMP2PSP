/*
	�����Ɣ��ɍ���̂œK���ɍ쐬�B
*/


//-------------------------------------------------
// �`��ȊO�� PSP �V�X�e���R�[�������b�v
void			psp_exit();
unsigned int	psp_get_time();		// 1970/01/01 ����̌o�ߕb��b�Ŏ擾
unsigned int	psp_get_clock();	// �A�v�����N�����Ă���̌o�ߎ��Ԃ��~���b�Ŏ擾





//-------------------------------------------------
// �W�� C �֐��̂Ȃ񂿂���Ĕ�


#ifndef RAND_MAX
#define RAND_MAX 32767
#endif


extern char arg_chars[][128];


// �Ȃ񂿂���� sprintf
// ������̂ݎ󂯎�� ("%s" �̂� !)
// ������̓O���[�o���z�� arg_chars[8][128] �ɓ���Ă���
int nsprintf(char *buf, char *fmt);


// �Ȃ񂿂�� randomize
void nrandomize(unsigned int seed);


// �Ȃ񂿂�� rand
int nrand( void );


// �Ȃ񂿂���� ltoa
char *nltoa( long value, char *string, int radix );