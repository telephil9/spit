</$objtype/mkfile

BIN=/$objtype/bin
TARG=spit
LIB=libpt/libpt.a$O
HFILES=a.h style.h
OFILES=spit.$O utils.$O

</sys/src/cmd/mkone

CFLAGS=-FTVw -Ilibpt -p

$LIB:V:
	cd libpt
	mk

clean nuke:V:
	@{ cd libpt; mk $target }
	rm -f *.[$OS] [$OS].out $TARG
