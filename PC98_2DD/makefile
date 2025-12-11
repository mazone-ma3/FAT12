# file720.COM PC-98 makefile for OpenWatcom

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

all:file720.com

OBJFILES =  file720.obj

file720.com : $(OBJFILES)
#	wlink  system Dos option quiet name file720.com file { $(OBJFILES) }

file720.obj : file720.c
