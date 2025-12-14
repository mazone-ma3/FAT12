/* FILE.X (X68K 1.22MB専用版) */
/* 使い方:	FILE AUTOEXEC.BAT	  → 画面表示 */
/*			FILE KAIJI.SYS 0   → 0000にロード */

/* 実験用 */
/* By m@3 with Grok 2025.12. */

//#include <dos.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
//#include <i86.h>
#include <x68k/iocs.h>

#define MAXLENGTH 40000

#define DRIVENO 1

#define AH 0x76
#define DA_UA 0x90


/* X68K 1.22MB用パラメータ */
#define ROOTDIR_LBA	5	// ルートディレクトリ開始LBA
#define ROOTDIR_NUM	6
#define DATA_LBA	11	// データ開始LBA
#define SECTORS_PER_CLUSTER	1

#define HEAD		2
#define SECTOR		8
#define SECTSIZE	1024

char buf[SECTSIZE];

/* X68000 IOCS 1.22MB読み込み */
int read_lba68(unsigned int lba, char *dst)
{
	int h = HEAD;
	int s = SECTOR;

	unsigned int cyl   = lba / (h * s);		   // 1シリンダ
	unsigned int head  = (lba / s) % h;//(lba % (h * s)) / s;
	unsigned int sect  =(lba % s) + 1;

	int a, i;

//	printf("read lba %d sect %d head %d cyl %d\n", lba, sect, head, cyl);

	a = _iocs_b_read ((0x91 << 8) | 0x70, (sect | (head << 8) | (cyl << 16) | (3 << 24)), SECTSIZE, dst);
//	printf("code %d\n", a);

/*	for(i = 0 ; i < 1024; ++i){
		a = dst[i];
		if(a < 0x20)
			a = 0x20;
		printf("%c", a);
	}*/
	if(a > 0)
		return  0;
	else
		return 1;
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
	unsigned int sec = 1 + (offset >> 10);	 /* SECTSIZE 1024 */
	unsigned int off = offset & 511;
	unsigned int val;

	if (sec != cached_sec) {
		if (read_lba68(sec, fatbuf)) return 0xFFF;
		cached_sec = sec;
	}
//	val = *(unsigned short*)(fatbuf + off);
	val = *(unsigned char*)(fatbuf + off) |
			(*(unsigned char*)(fatbuf + off + 1) << 8);

	return (c & 1) ? (val >> 4) : (val & 0xFFF);
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
		if (read_lba68(ROOTDIR_LBA + s, buf)) return 1;
//		read_lba68(ROOTDIR_LBA + s, buf);
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
							(*(unsigned char*)(buf + e + 27)  << 8);
//				*size	= *(unsigned int*)  (buf + e + 28);
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
	unsigned int cluster;
	char *p = dest;
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
			if (read_lba68(lba + i, p)) {
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
			p += SECTSIZE;
			if (size2 <= SECTSIZE) { size2 = 0; break; }
			size2 -= SECTSIZE;
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
		printf("Usage: FILE filename [off]\r\n");
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
		unsigned int off;
		sscanf(argv[2], "%x", &off);
		if(!load_file(argv[1], (char *)off, &size)){
			printf("%s -> %08X\r\n", argv[1], off);
		}
	}
	return 0;
}
