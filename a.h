#include <u.h>
#include <libc.h>
#include <draw.h>
#include <mouse.h>
#include <keyboard.h>
#include <thread.h>
#include <memdraw.h>
#include <bio.h>
#include <pt.h>

typedef struct Lines Lines;

enum 
{
	Maxslides = 128,
	Maxlines  = 16 
};

struct Lines
{
	char *lines[Maxlines];
	uint nlines;
};

enum
{
	Cbg,
	Cfg,
	Cqbg,
	Cqbord,
	Ccbg,
	Ccbord,
	Ncols,
};

Sfont*	loadsfont(char*, float);
void*	emalloc(ulong);
Image*	ealloccol(ulong);
