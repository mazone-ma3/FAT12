/* FILE.COM (PC-98 1.22MB専用版) */
/* 使い方:	FILE AUTOEXEC.BAT	  → 画面表示 */
/*			FILE KAIJI.SYS 9000:0   → 9000:0000にロード */

/* 実験用 */
/* By m@3 with Grok 2025.12. */

#include <dos.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAXLENGTH 40000

#define DRIVENO 0

#define AH 0x76
#define DA_UA 0x90


/* PC-98 1.22MB用パラメータ */
#define ROOTDIR_LBA	5	// ルートディレクトリ開始LBA
#define ROOTDIR_NUM	6
#define DATA_LBA	11	// データ開始LBA
#define SECTORS_PER_CLUSTER	1

#define HEAD		2
#define SECTOR		8
#define SECTSIZE	1024

char buf[SECTSIZE];

int ax,bx,cx,dx;

extern DKB_read(char *);

//int int86(int num,union REGS *in, union REGS *out);
int read_lbatw(unsigned short lba, char *dst)
{
	union REGS regs, regs2;
	struct SREGS sregs;
	int h = HEAD;
	int s = SECTOR;
	int i,  a = 0;

	unsigned short cyl   = lba / (h * s);		   // 1シリンダ
	unsigned short head  = (lba / s) % h;//(lba % (h * s)) / s;
	unsigned short sect  =(lba % s) + 1;
	unsigned short restsect;

//	printf("read lba %d sect %d head %d cyl %d\n", lba, sect, head, cyl);


	regs.x.ax = (0x00 << 8) | 0x20 | DRIVENO;
//	regs.h.ah = 0x00;
//	regs.h.al = 0x20;
	regs.x.dx = 0x0003;
	regs.x.bx = 0x0208; //(head << 8) | (sect);
	int86(0x93, &regs, &regs2);
	a = regs2.x.ax;
	if((a & 0xff00)){ // == 0x8000){
		printf("set error %x\n", a);
		return 1;
	}


//#ifdef DEBUG
	ax = ((0x05 << 8) & 0xff00) | 0x20 | DRIVENO;
	bx = 1;
	cx = cyl;
	dx = ((head << 8) & 0xff00) | (sect);

	a = DKB_read(dst);
//#endif

	if((a & 0xff00)){ // == 0x8000){
		printf("error %x\n", cx);
		return 1;
	}
	for(i = 0 ; i < 1024; ++i){
		a = dst[i];
		if(a < 0x20)
			a = 0x20;
//		printf("%c", a);
	}

	return  0;
}

#ifdef DEBUG
/* PC-98 BIOS 1.22MB読み込み（INT 1Bh AH=22h） */
int read_lba98(unsigned short lba, char far *dst)
{
	union REGPACK regs;
//	int i,  a;
	int h = HEAD;
	int s = SECTOR;

	unsigned short cyl   = lba / (h * s);		   // 1シリンダ
	unsigned short head  = (lba / s) % h;//(lba % (h * s)) / s;
	unsigned short sect  =(lba % s) + 1;

//	printf("read lba %d sect %d head %d cyl %d\n", lba, sect, head, cyl);

	regs.x.ax = ((AH << 8) & 0xff00) | DA_UA | DRIVENO;
	regs.x.bx = SECTSIZE;
	regs.x.cx = ((3 << 8) & 0xff00) | (cyl & 0x00ff);
	regs.x.dx = ((head << 8) & 0xff00) | (sect & 0x00ff);
	regs.x.es = FP_SEG(dst);
	regs.x.bp = FP_OFF(dst);

	intr(0x1b, &regs);
//	if(!(regs.x.flags & 1)){
//	printf("success\n");
/*	for(i = 0 ; i < 1024; ++i){
		a = dst[i];
		if(a < 0x20)
			a = 0x20;
		printf("%c", a);
	}*/
//	return 0;
	return (regs.x.flags & 1);   // CF=1ならエラー
//		return 0;
//	}
//	printf("error\n");
//		return 1;
}
#endif

unsigned short cluster_to_lba(unsigned short c)
{
	return DATA_LBA + (c - 2) * SECTORS_PER_CLUSTER;
}

/* FAT12エントリ取得（キャッシュ付き） */
static unsigned short cached_sec = 0xFFFF;
static char fatbuf[SECTSIZE];

unsigned short next_cluster(unsigned short c)
{
	unsigned short offset = c + ((c >> 1) & 0xffff);	   // 
	unsigned short sec = 1 + ((offset >> 10) & 0xffff);	 // 
	unsigned short off = offset & 511;
	unsigned short val;

	if (sec != cached_sec) {
		if (read_lbatw(sec, fatbuf)) return 0x0FFFL;
		cached_sec = sec;
	}
//	val = *(unsigned short*)(fatbuf + off);
	val = (*(unsigned char*)(fatbuf + off)) |
			(*(unsigned char*)(fatbuf + off + 1) << 8);
	return (c & 1) ? (val >> 4) : (val & 0x0FFFL);
}

/* ファイル検索（8.3大文字比較） */
int find_file(const char *name8_3, unsigned short *cluster, unsigned int *size)
{
	char uname[13] = "           ";
	int e,j,s;
	char entry[13] = "           ";
	int i;

//	strncpy(uname, name8_3, 8);
	for (i = 0; i < 8 && name8_3[i] != '.'; i++)
		uname[i] = name8_3[i];
//	uname[8] = '.';
	strcpy(uname + 8, name8_3 + i);
	strupr(uname);

	for (s = 0; s < ROOTDIR_NUM; s++) {				 // ルートディレクトリ検索
		if (read_lbatw(ROOTDIR_LBA + s, buf)) return 1;
//		read_lbatw(ROOTDIR_LBA + s, buf);
//		printf("Done.\n");
//		continue;

		for (e = 0; e < SECTSIZE; e += 32) {
			strcpy(entry, "           ");
			if (buf[e] == 0) return 1;
			if (((unsigned char)buf[e] == 0xe5) || (buf[e+11] & 0x08)) continue;

//			for (i = 0; i < 8 && buf[e+i] != ' '; i++) entry[i] = toupper(buf[e+i]);
			for (i = 0; i < 8; i++) entry[i] = toupper(buf[e+i]);
//			if (buf[e+8] != ' ') {
				entry[8] = '.';
//				for (j = 0; j < 3 && buf[e+8+j] != ' '; j++) entry[9+j] = toupper(buf[e+8+j]);
				for (j = 0; j < 3; j++) entry[9+j] = toupper(buf[e+8+j]);
//			}
			entry[8+1+3] = '\0';
//			printf("%s %s\n", entry, uname);
			if (strcmp(entry, uname) == 0) {
//				*cluster = *(unsigned short*)(buf + e + 26);
				*cluster = (*(unsigned char*)(buf + e + 26))|
							(*(unsigned char*)(buf + e + 27) << 8);
//				*size	= *(unsigned short*)  (buf + e + 28);
				*size	= (*(unsigned char*)  (buf + e + 28)) |
							(*(unsigned char*)  (buf + e + 29) << 8) ;
							(*(unsigned char*)  (buf + e + 30) << 16) ;
							(*(unsigned char*)  (buf + e + 31) << 24) ;
//				printf("cluster %d size %d\n", *cluster, *size);
				return 0;
			}
		}
	}
	return 1;
}

/* ファイル読み込み本体 */
int load_file(const char *name, char *dest, unsigned int *size)
{
	unsigned short cluster;
	char *p = dest;
	unsigned short lba;
	unsigned int size2;
	int i; //,j,a;

	if (find_file(name, &cluster, size)) {
		printf("%s not found!\r\n", name);
		return 1;
	}

	if(*size >= MAXLENGTH)
		*size = MAXLENGTH;
	size2 = *size;
	printf("Loading %s (%u bytes)...", name, size2);

	while (size2 && cluster < 0x0FF8L) {
		lba = cluster_to_lba(cluster);
		for (i = 0; i < SECTORS_PER_CLUSTER; i++) {
			if (read_lbatw(lba + i, p)) {
				printf("Read error!\r\n");
				return 1;
			}


/*	for(j = 0 ; j < 1024; ++j){
		a = dest[j];
		if(a < 0x20)
			a = 0x20;
		printf("%c", a);
	}
*/
			p += 512;
			if (size2 <= 512) { size2 = 0; break; }
			size2 -= 512;
		}
		cluster = next_cluster(cluster);
//		printf("cluster %d\n", cluster);
	}
	printf("OK\r\n");
	return  0;
}

/* メイン */
int main(int argc, char **argv)
{
	static char tmp[MAXLENGTH];
	unsigned int size, i;
//	for(i = 0; i < MAXLENGTH; ++i){
//		tmp[i] = 0;
//	}

//	for(i = 0; i < 20000; ++i)
//		if (read_lbatw(i, tmp)) return 1;

	if (argc < 2) {
		printf("Usage: FILE filename [seg:off]\r\n");
		printf("Ex: FILE AUTOEXEC.BAT\r\n");
		printf("    FILE GAME.EXE 0\r\n");
		return 1;
	}

	if (argc == 2) {
		if(!load_file(argv[1], tmp, &size)){
			tmp[size - 1] = 0;
			printf("\r\n--- %s ---\r\n%s\r\n", argv[1], tmp);
		}
	} else {
		unsigned seg, off;
		sscanf(argv[2], "%x:%x", &seg, &off);
		if(!load_file(argv[1], off, &size)){
			printf("%s -> %04X:%04X\r\n", argv[1], seg, off);
		}
	}
	return 0;
}
