# FILE.COM PC-98 makefile for OpenWatcom

#マクロ
CC= wcl

#CFLAGS = -mt -bt=dos -0 -ox

CFLAGS = -mt -lr -0 -s -wx -j  -zq

# Cソ－スからobj作成

.SUFFIXES: .obj .c
.c.obj:
	$(CC) $< $(CFLAGS)
# -c


#生成手順

all:file.com

OBJFILES =  file.obj

file.com : $(OBJFILES)
#	wlink  system Dos option quiet name file.com file { $(OBJFILES) }

file.obj : file.c
