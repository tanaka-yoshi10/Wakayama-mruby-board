//***********************************************************
// EEPROM���t�@�C���̂悤�Ɏg���N���X
//
// �t�@�C����
//  �擪�Z�N�^�ɓ����Ă���0x00�܂ł�31�o�C�g�ȓ�
//
// �t�@�C���T�C�Y
//  �擪�Z�N�^�̃t�@�C�����ɑ���2�o�C�g�ɓ����Ă���
//
// �f�[�^
//  �擪�Z�N�^�̃t�@�C�����ɑ���2�o�C�g�ȍ~�ɓ����Ă���
//
// �擪�Z�N�^
//  xxxxxxxxxxxxxxxxxxx00 | 0000 | zzzzzzzzzzzzz...
//  �t�@�C�����@�@�@00�I�[ �T�C�Y�@���f�[�^
//
// Address 0x0000�`0x00FF�܂ł́AEEPROM��Push Pop�Ɏg����
//
//***********************************************************
#include <rxduino.h>
#include <eeprom.h>

#include <eepfile.h>
//#define    DEBUG                1        // Define if you want to debug

#ifdef DEBUG
#  define DEBUG_PRINT(m,v)    { Serial.print("** "); Serial.print((m)); Serial.print(":"); Serial.println((v)); }
#else
#  define DEBUG_PRINT(m,v)    // do nothing
#endif

#define EEPSIZE			0x8000
#define EEPSECTOR_SIZE	512
#define EEPSECTORS		(EEPSIZE / EEPSECTOR_SIZE)

#define EEPSTASECT		1
#define EEPENDSECT		(EEPSECTORS - 1)

#define EEPFAT_START	0x100	//0x100�`0x13F�܂ł�FAT�ۑ��̈�Ƃ��Ďg���Ă���

#define EEP_EMPTY	0		//���g�p
#define EEP_TOP		1		//�擪
#define EEP_USED	2		//�g�p��

//�錾
EEPFILE	EEP;

//Sect[]�� 0�`6�r�b�g(00�`7F)�����̃Z�N�^�������B���̃Z�N�^���������g���w���Ă���Ƃ��͍ŏI�Z�N�^�B
//�@�@�@�@�@�@�@�@�@9,10�r�b�g�́A0:���g�p�A1:�擪�A2:�g�p�� ������킷�B
//					11,12�r�b�g�́A0:�I�[�v�����Ă��Ȃ��A1:READ�I�[�v���A2:WRITE||APPEND�I�[�v�� ������킷�B
static unsigned short Sect[EEPSECTORS];		//512�o�C�g��1�Z�N�^�Ƃ��ĊǗ�����BsaveFat()�̃^�C�~���O��EEPROM�ɕۑ������B

//******************************************************
// FAT�Z�N�^�[��\�����܂�
//******************************************************
void EEPFILE::viewFat(void)
{
	char az[50];

	Serial.println();
	Serial.println("FAT Sector");
	for(long i=0; i<EEPSECTORS; i++){
		sprintf(az, "%02X-%04X ",i ,Sect[i] );
		Serial.print(az);
		if((i % 10) == 9){
			Serial.println();
		}
	}
	Serial.println();
}

//******************************************************
//�@�Z�N�^�f�[�^��\�����܂�
//******************************************************
void EEPFILE::viewSector(int sect)
{
	char az[50];
	Serial.println();
	Serial.print("Sector ");
	Serial.println(sect);

	int add = sect * EEPSECTOR_SIZE;

	Serial.println("ADDR 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F");
	for(int i=0; i<EEPSECTOR_SIZE/16; i++){

		sprintf(az, "%04X", add + 16 * i);
		Serial.print(az);

		for(int j=0; j<16; j++){
			sprintf(az, " %02X", EEPROM.read(add + 16 * i + j));
			Serial.print(az);
		}
		Serial.println();
	}
}

//******************************************************
// EEPROM FILE�V�X�e�������������܂�
//
// clear��1�̂Ƃ��́A�t�@�C���V�X�e�������������܂�
//******************************************************
void EEPFILE::begin(int clear)
{
	if (clear == 0){
		// EEPROM����FAT��ǂݍ��݂܂�
		for (int i=0; i<EEPSECTORS; i++){
			Sect[i] = EEPROM.read( EEPFAT_START + i*2 ) + (EEPROM.read( EEPFAT_START + i*2 + 1 )<<8);
		}
	}
	else{
		// �t�@�C���V�X�e�������������܂�

		for (int i=0; i<EEPSECTORS; i++){
			Sect[i] = (EEP_USED<<8);		//��x�A�S�Ďg�p���ɂ���
		}		

		for (int i=EEPSTASECT; i<=EEPENDSECT; i++){
			Sect[i] = (EEP_EMPTY<<8);		//FAT�g�p�Z�N�^�̂݋󂫏�Ԃɂ���
		}
		saveFat();
	}
}

//******************************************************
// �t�@�C�����I�[�v�����܂�
// mod: 0:read, 1:write�V�K, 2:Write�ǋL
// �G���[�̎��́A-1��Ԃ�
//******************************************************
int EEPFILE::fopen(FILEEEP *file, const char *filename, char mode)
{
	int sect = 0;
	int len = strlen(filename);
	unsigned long add = 0;

	//���ɃI�[�v�����Ă��Ȃ��ǂ����̃`�F�b�N
	sect = scanFilename(filename);
	if (sect != -1){
		if((Sect[sect] & ((EEP_READ | EEP_WRITE) << 10)) != 0){
			//���ɃI�[�v�����Ă���
			return -1;
		}
	}

	if (mode == EEP_READ){

		DEBUG_PRINT("mode", (int)mode);
		DEBUG_PRINT("sect", sect);

		//sect = scanFilename(filename);
		if (sect == -1){
			return -1;
		}

		Sect[sect] |= EEP_READ << 10;			//�t�@�C���g�p���t���O

		add = sect * EEPSECTOR_SIZE;
		file->filesize = (EEPROM.read( add + len + 1 ) & 0xFF) + ((EEPROM.read( add + len + 2 ) & 0xFF) << 8);
		file->stasector = sect;
		file->offsetaddress = len + 3;
		file->seek = 0;
		return 0;
	}
	else if (mode == EEP_WRITE || sect == -1){

		DEBUG_PRINT("mode", (int)mode);
		DEBUG_PRINT("sect", sect);

		//�t�@�C�������邩������Ȃ��̂ŁA�Ƃ肠�����폜���Ă����܂��B�Ȃ�������-1���Ԃ��Ă��邾��
		fdelete(filename);

		//�󂢂Ă���Z�N�^��T��
		sect = scanEmptySector((int)millis() & 0x3F);	//(int)millis()&0x3F�́A�����̗v�f���܂܂��Ă���Z�N�^�̕��������p(�E�F�A���x�����O���l��)
		if (sect == -1){
			//�t�@�C���������ς�������
			return -1;
		}

		// �t�@�C���|�C���^�̏������݂��s��
		setFile( file, filename, sect, EEP_WRITE );
		return 0;
	}
	else if (mode == EEP_APPEND){

		DEBUG_PRINT("mode", (int)mode);
		DEBUG_PRINT("sect", sect);

		Sect[sect] |= EEP_WRITE << 10;			//�t�@�C���g�p���t���O
		
		add = sect * EEPSECTOR_SIZE;
		file->filesize = (EEPROM.read( add + len + 1 ) & 0xFF) + ((EEPROM.read( add + len + 2 ) & 0xFF) << 8);
		file->stasector = sect;
		file->offsetaddress = len + 3;
		file->seek = file->filesize;
		return 0;
	}
	return -1;
}

//******************************************************
// �t�@�C���̃V�[�N
// orign����Ƃ����I�t�Z�b�g�l�Ɉړ�����
// �t�@�C���̐擪����̈ʒu��Ԃ�
//******************************************************
int EEPFILE::fseek(FILEEEP *file, int offset, int origin)
{
	long sk = 0;
	if (origin == EEP_SEEKTOP){
		sk = 0 + offset;
	}
	else if (origin == EEP_SEEKCUR){
		sk = file->seek + offset;
	}
	else if (origin == EEP_SEEKEND){
		sk = file->filesize + offset;
	}

	if (sk < 0){
		sk = 0;
	}
	else if (sk > file->filesize){
		sk = file->filesize;
	}

	file->seek = sk;

	return file->seek;
}

//******************************************************
// �t�@�C�����폜���܂�
// �G���[�̎��́A-1��Ԃ�
//******************************************************
int EEPFILE::fdelete(const char *filename)
{
	int sect = scanFilename(filename);

	if (sect == -1){
		return -1;
	}

	int next;
	while(true){
		next = Sect[sect] & 0x7F;		//���̃Z�N�^�ǂݍ���
		Sect[sect] = EEP_EMPTY << 8;	//���Z�N�^����ɂ���
		if (sect == next){				//�ŏI�Z�N�^�ɂ͎����̃Z�N�^�ԍ��������Ă���
			break;
		}
		sect = next;
	}

	//FAT�f�[�^��EEPROM�ɕۑ�����
	saveFat();

	return 0;
}

//******************************************************
// ��������
// file->seek�ʒu�Ɏw��ʂ̃f�[�^���������݂܂�
//******************************************************
int EEPFILE::fwrite(FILEEEP *file, char *arry, int *len)
{
	int mlen = 0;
	for(int i=0; i<*len; i++){
		if (fwrite(file, arry[i])==-1){
			*len = mlen;
			return -1;
		}
		mlen++;
	}
	return 1;
}

//******************************************************
// ��������
// file->seek�ʒu�ɏ������݂܂�
//******************************************************
int EEPFILE::fwrite(FILEEEP *file, char dat)
{
	int secadd = 0;

	//�������ރZ�N�^�����߂܂�
	int sect = getSect(file, &secadd);
	if (sect == -1){
		return -1;
	}

	//�������݂܂�
	int add = sect * EEPSECTOR_SIZE + secadd;
	epWrite( add, dat );

	//seek��1�i�߂܂�
	file->seek++;

	//seek�ʒu��fileSize�𒴂����ꍇ
	if (file->seek > file->filesize){
		file->filesize = file->seek;
	}
	return 1;
}

//******************************************************
// �ǂݍ���
// file->seek�ʒu��ǂݍ��݂܂�
//******************************************************
int EEPFILE::fread(FILEEEP *file)
{
	//seek�ʒu��fileSize�ȏ�̏ꍇ
	if (file->seek >= file->filesize){
		file->seek = file->filesize;
		return -1;
	}

	int secadd = 0;

	//�ǂݍ��ރZ�N�^�����߂܂�
	int sect = getSect(file, &secadd);
	//if (sect == -1){		//�����肦�Ȃ��̂�
	//	return -1;
	//}

	//�ǂݍ��݂܂�
	int add = sect * EEPSECTOR_SIZE + secadd;
	int ret = EEPROM.read( add );

	//seek��1�i�߂܂�
	file->seek++;

	return ret;
}

//******************************************************
// �t�@�C������܂�
//******************************************************
void EEPFILE::fclose(FILEEEP *file)
{
	DEBUG_PRINT("file->stasector", file->stasector);
	DEBUG_PRINT("file->filesize", file->filesize);
	DEBUG_PRINT("file->offsetaddress", file->offsetaddress);
	DEBUG_PRINT("file->seek", file->seek);

	int sect = file->stasector;
	if (sect < EEPSTASECT){	return;	}

	DEBUG_PRINT("fclose Sect[sect]", Sect[sect]);
	if(((Sect[sect] >> 10) & 0x3) == EEP_WRITE){
		//�t�@�C���T�C�Y���������݂܂�
		DEBUG_PRINT("fclose", "EEP_WRITE");
		int add = file->stasector * EEPSECTOR_SIZE;
		epWrite(add + file->offsetaddress - 1, (file->filesize>>8) & 0xFF);
		epWrite(add + file->offsetaddress - 2, file->filesize & 0xFF);
	}

	int next;
	while(true){
		Sect[sect] = (Sect[sect] & 0xF3FF) | (EEP_CLOSE << 10);	//�Z�N�^��Close�t���O�Z�b�g
		next = Sect[sect] & 0x7F;		//���̃Z�N�^�ǂݍ���
		if (sect == next){				//�ŏI�Z�N�^�ɂ͎����̃Z�N�^�ԍ��������Ă���
			break;
		}
		sect = next;
	}

	//FAT�f�[�^��EEPROM�ɕۑ�����
	saveFat();

	file->filesize = 0;
	file->offsetaddress = 0;
	file->seek = 0;
	file->stasector = -1;
}

//******************************************************
// �t�@�C���̏I�[������true��Ԃ��܂�
//******************************************************
bool EEPFILE::fEof(FILEEEP *file)
{
	return (file->seek >= file->filesize)?true:false;
}

//******************************************************
// �Z�N�^�ɕۑ�����Ă���t�@�C������Ԃ��܂�
// �Z�N�^�Ƀt�@�C������������� filename[0]=0 ���Ԃ�܂�
// �߂�l�̓t�@�C���T�C�Y�ł�
//******************************************************
int EEPFILE::fdir(int sect, char *filename)
{
	int ret = 0;
	//�t�@�C���擪�Z�N�^��
	if(((Sect[sect] >> 8) & 0x3) == EEP_TOP){
		getFilename(sect, filename);
		ret = ffilesize( filename );
	}
	else{
		filename[0] = 0;
	}
	return ret;
}

//******************************************************
// �t�@�C���T�C�Y��Ԃ��܂�
//******************************************************
int EEPFILE::ffilesize(const char *filename)
{
	int ret = 0;
	int sect = scanFilename(filename);
	int len = strlen(filename);

	if (sect != -1){
		int add = sect * EEPSECTOR_SIZE;
		ret = (EEPROM.read( add + len + 1 ) & 0xFF) + ((EEPROM.read( add + len + 2 ) & 0xFF) << 8);
	}
	return ret;
}


//***
//  Private methods
//***

//*********
// �t�@�C�������擾����
// �t�@�C��������(�o�C�g)���Ԃ�B�Ō��0x00�͐����Ȃ��B
//*********
int EEPFILE::getFilename(int sect, char *filename)
{
	//DEBUG_PRINT("getFilename sect", sect);

	if (sect < EEPSTASECT){ return 0; }

	unsigned long add = sect * EEPSECTOR_SIZE;
	int len = 0;
	for (int i = (int)add; i <= (int)add + EEPFILENAME_SIZE; i++){
		filename[len] = (char)EEPROM.read(i);

		if (filename[len] == 0){
			//DEBUG_PRINT("getFilename filename", filename);
			//DEBUG_PRINT("getFilename len", len);
			return len;
		}
		len++;
	}

	//DEBUG_PRINT("getFilename filename", filename);
	return 0;
}

//*********
// �t�@�C������T��(�t�@�C������31�o�C�g�܂�)
// ���������Z�N�^�ԍ���Ԃ�
// ������Ȃ��Ƃ��́A-1��Ԃ�
//*********
int EEPFILE::scanFilename(const char *filename)
{
	//DEBUG_PRINT("scanFilename", filename);

	int flen = strlen((const char*)filename);
	if(flen >= EEPFILENAME_SIZE){ return -1; }

	char fn[EEPFILENAME_SIZE];
	int len = 0;
	int ret = -1;
	for (int i = EEPSTASECT; i <= EEPENDSECT; i++){

		//DEBUG_PRINT("scanFilename i", i);

		if (((Sect[i]>>8) & 0x3) == EEP_TOP){

			len = getFilename(i, fn);
			if (flen == len){
				ret = i;
				for (int j = 0; j < len; j++){
					if (filename[j] != fn[j]){
						ret = -1;
						break;
					}
				}
			}
			if (ret != -1){	break;	}
		}
	}

	//DEBUG_PRINT("scanFilename ret", ret);
	return ret;
}

//*********
// �������܂�Ă��Ȃ��u���b�N��T��
// �u���b�N�ԍ���Ԃ�
// �S�ď������܂�Ă���Ƃ��́A-1��Ԃ�
//*********
int EEPFILE::scanEmptySector(int start)
{
	int s0 = EEPSTASECT;
	if (start >= EEPSTASECT && start <= EEPENDSECT){
		s0 = start;
	}

	for (int i = s0; i <= EEPENDSECT; i++){
		if (((Sect[i] >> 8) & 0x3) == EEP_EMPTY){
			return i;
		}
	}

	for (int i = EEPSTASECT; i < s0; i++){
		if (((Sect[i] >> 8) & 0x3) == EEP_EMPTY){
			return i;
		}
	}
	return -1;
}

//*********
// �t�@�C���|�C���^�̏������݂��s��
//*********
void EEPFILE::setFile( FILEEEP *file, const char *filename, int sect, int mode)
{
	int add = sect * EEPSECTOR_SIZE;
	int len = strlen(filename);
	int a1;

	//�Z�N�^�z����Z�b�g���܂�(�������g���Z�b�g)
	Sect[sect] = sect & 0x7F;		//�Ƃ肠�����g�b�v�Z�N�^�ԍ����Z�b�g����
	Sect[sect] |= EEP_TOP << 8;		//�t�@�C���g�b�v�t���O
	Sect[sect] |= mode << 10;		//�t�@�C���g�p���t���O

	//�t�@�C��������������
	for( int i=0; i<len; i++){
		epWrite( add + i, filename[i] );
	}
	epWrite( add + len, 0 );

	//�t�@�C���T�C�Y 0 �Z�b�g
	epWrite( add + len + 1, 0 );
	epWrite( add + len + 2, 0 );

	file->filesize = 0;
	file->stasector = sect;
	file->offsetaddress = len + 3;
	file->seek = 0;
}

//*********
// FAT��EEPROM�ɕۑ����܂�
//*********
void EEPFILE::saveFat(void)
{	
	//int a1, a2;
	for( int i=0; i<EEPSECTORS; i++){
		// 11,12�r�b�g�ڂ�0�ɂ��ĕۑ����� 1111 0011 1111 1111
		//a1 = (Sect[i] & (~(3 << 10))) & 0xFF;
		//a2 = ((Sect[i] & (~(3 << 10)))>>8) & 0xFF;
		epWrite( EEPFAT_START + i*2, (Sect[i] & (~(3 << 10))) & 0xFF);
		epWrite( EEPFAT_START + i*2 + 1, ((Sect[i] & (~(3 << 10)))>>8) & 0xFF);
	}
}

//*********
// file->seek�������ꏊ�̃Z�N�^�����߂܂�
// ���̃Z�N�^�̃A�h���X��add�ɓ���ĕԂ��܂�
// �S�ď������܂�Ă���Ƃ��́A-1��Ԃ�
//*********
int EEPFILE::getSect(FILEEEP *file, int *add)
{
	if(file->stasector < EEPSTASECT){
		return -1;
	}

	//���Z�N�^���������߂�
	int n = (file->offsetaddress + file->seek) / EEPSECTOR_SIZE;
	*add = (file->offsetaddress + file->seek) % EEPSECTOR_SIZE;

	int sect = file->stasector;

	//DEBUG_PRINT("getSect n", n);
	//DEBUG_PRINT("getSect add", *add);
	//DEBUG_PRINT("getSect file->filesize", file->filesize);
	//DEBUG_PRINT("getSect file->seek", file->seek);

	//�Z�N�^��ǂ�������
	for(int i=0; i<n; i++){
		sect = Sect[sect] & 0x7F;
	}
	//DEBUG_PRINT("getSect sect 1", sect);
	//DEBUG_PRINT("getSect EEP_WRITE", (Sect[file->stasector] >> 10) & 0x3);

	//�������݃I�[�v���̂Ƃ�
	if(((Sect[file->stasector] >> 10) & 0x3) == EEP_WRITE){
		//����؂�Ă���Ƃ��́A
		if(*add == 0){

			//DEBUG_PRINT("getSect file->stasector", file->stasector);
			//DEBUG_PRINT("getSect file->filesize", file->filesize);
			//DEBUG_PRINT("getSect file->offsetaddress", file->offsetaddress);
			//DEBUG_PRINT("getSect file->seek", file->seek);

			//����؂�Ă���̂ɁASect[sect]�ɂ��鎟�̃Z�N�^�ԍ����������g�ł�������A
			if (file->seek >= file->filesize){
				//�V�K�Z�N�^��p�ӂ���
				int newsect = scanEmptySector((int)millis() & 0x3F);	//(int)millis()&0x3F�́A�����̗v�f���܂܂��Ă���Z�N�^�̕��������p(�E�F�A���x�����O���l��)
				if (newsect == -1){
					//�t�@�C���������ς�������
					return -1;
				}
				//���̃Z�N�^������
				Sect[sect] = (Sect[sect] & 0xFF80) | newsect;

				DEBUG_PRINT("getSect newsect", newsect);

				//�Z�N�^�z����Z�b�g���܂�
				Sect[newsect] = newsect & 0x7F;			//�Ō���Ȃ̂Ŏ������g���Z�b�g
				Sect[newsect] |= EEP_USED << 8;			//�t�@�C�����[�Y�t���O
				Sect[newsect] |= EEP_WRITE << 10;		//�t�@�C���g�p���t���O
				sect = newsect;
			}
		}
	}
	return sect;
}

//*********
// EEPROM�ɏ������݂܂�
// �G���[�̂Ƃ���-1��Ԃ�
//*********
int EEPFILE::epWrite(unsigned long addr,unsigned char data)
{
	if (data != EEPROM.read(addr)){
		return EEPROM.write(addr, data);
	}
	return 1;
}
