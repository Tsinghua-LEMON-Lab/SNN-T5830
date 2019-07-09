PRO=SNN_lib.so

SRCS=ATLio.c  DataLoader.c  Network.c  Utils.c

ARCH		= ${ATFSSITEARCH}
CC		= ${ATFSSITECC} 
ATFS_CFLAGS	= -g

OBJS		= ${SRCS:%.c=%.o}

ATFS_INCS	= -I${ATFSROOT}/${ARCH}/${ATFSSITEOS}/${ATFSATL}/include \
		  -I${ATFSROOT}/${ARCH}/${ATFSSITEOS}/${ATFSSYS}/include \
		  -I${ATFSROOT}/${ARCH}/${ATFSSITEOS}/${ATFSTBUS}/include \
		  -I${ATFSROOT}/${ARCH}/${ATFSSITEOS}/${ATFSVTC}/include

ATFS_LIBS	= -L${ATFSROOT}/${ARCH}/${ATFSOS}/${ATFSCOMP}/${ATFSTMODEL}/lib  \
		  -L${ATFSROOT}/${ARCH}/${ATFSOS}/${ATFSSYS}/lib                 \
		  -L${ATFSROOT}/${ARCH}/${ATFSOS}/${ATFSDIAG}/${ATFSTMODEL}/lib  \
		  -L${ATFSROOT}/${ARCH}/${ATFSOS}/${ATFSTBUS}/lib                \
		  -L${ATFSROOT}/${ARCH}/${ATFSOS}/${ATFSVTC}/${ATFSTMODEL}/lib   \
		  -L${ATFSROOT}/${ARCH}/${ATFSOS}/ATFSnamazu/lib                 \
		  -latfsmcicl                                    \
		  -latfssock                                     \
		  -latfsvtc                                      \
		  -latfstb                                       \
		  -latfssc                                       \
		  -latfshn                                       \
		  -latfsrc                                       \
		  -latfsflow                                     \
		  -latfsfm                                       \
		  -latfspf                                       \
		  -latfscommon                                   \
		  -latfsem                                       \
		  -latfsew                                       \
		  -lnsl                                          \
		  -lm                                            \
		  -ldl                                           \
		  -lrt

all:	clean make

${PRO}:	${OBJS}
	${CC} ${ATFS_CFLAGS} ${ATFS_LIBS} ${OBJS} -shared -o $@ 

%.o:%.c
	${CC} ${ATFS_CFLAGS} -fpic ${ATFS_INCS} -c  $<

make:	${PRO}

clean:
	rm -f ${PRO} ${OBJS} *.o 

