/* FILE.COM (PC-98 1.22MB専用版) */
/* 使い方:	FILE AUTOEXEC.BAT	  → 画面表示 */
/*			FILE KAIJI.SYS 9000:0   → 9000:0000にロード */

/* 実験用 */
/* By m@3 with Grok 2025.12. */

#include <dos.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <i86.h>

#define MAXLENGTH 40000

#define DRIVENO 1

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

/* PC-98 BIOS 1.22MB読み込み（INT 1Bh AH=22h） */
int read_lba98(unsigned int lba, char far *dst)
{
	union REGPACK regs;
//	int i,  a;
	int h = HEAD;
	int s = SECTOR;

	unsigned int cyl   = lba / (h * s);		   // 1シリンダ
	unsigned int head  = (lba / s) % h;//(lba % (h * s)) / s;
	unsigned int sect  =(lba % s) + 1;

	printf("read lba %d sect %d head %d cyl %d\n", lba, sect, head, cyl);

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


unsigned int cluster_to_lba(unsigned int c)
{
	return DATA_LBA + (c - 2) * SECTORS_PER_CLUSTER;
}

/* FAT12エントリ取得（キャッシュ付き） */
static unsigned int cached_sec = 0xFFFF;
static char fatbuf[SECTSIZE];

unsigned int next_cluster(unsigned int c)
{
	unsigned int offset = c + (c >> 1);	   // 
	unsigned int sec = 1 + (offset >> 10);	 // 

//	unsigned int sec = cluter_to_lba(c)

	unsigned int off = offset & 511;
	unsigned int val;

	printf("offset %d sec %d off %d\n", offset, sec, off);

	if (sec != cached_sec) {
		if (read_lba98(sec, fatbuf)) return 0xFFF;
		cached_sec = sec;
	}
	val = *(unsigned int*)(fatbuf + off);
	return (c & 1) ? ((val >> 4) & 0xFFF) : (val & 0xFFF);
}

/* ファイル検索（8.3大文字比較） */
int find_file(const char *name8_3, unsigned int *cluster, unsigned int *size)
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
		if (read_lba98(ROOTDIR_LBA + s, buf)) return 1;
//		read_lba98(ROOTDIR_LBA + s, buf);
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
				*cluster = *(unsigned short*)(buf + e + 26);
				*size	= *(unsigned int*)  (buf + e + 28);
//				printf("cluster %d size %d\n", *cluster, *size);
				return 0;
			}
		}
	}
	return 1;
}

/* ファイル読み込み本体 */
int load_file(const char *name, char far *dest, unsigned int *size)
{
	unsigned int cluster;
	char far *p = dest;
	unsigned int lba;
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

	while (size2 && cluster < 0xFF8) {
		lba = cluster_to_lba(cluster);
		for (i = 0; i < SECTORS_PER_CLUSTER; i++) {
			if (read_lba98(lba + i, p)) {
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
	}
	printf("OK\r\n");
	return  0;
}

/* メイン */
int main(int argc, char **argv)
{
	static char tmp[MAXLENGTH];
	unsigned int size; //, i
//	for(i = 0; i < MAXLENGTH; ++i){
//		tmp[i] = 0;
//	}

	if (argc < 2) {
		printf("Usage: FILE filename [seg:off]\r\n");
		printf("Ex: FILE AUTOEXEC.BAT\r\n");
		printf("    FILE GAME.EXE 8000:0\r\n");
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
		if(!load_file(argv[1], MK_FP(seg, off), &size)){
			printf("%s -> %04X:%04X\r\n", argv[1], seg, off);
		}
	}
	return 0;
}
