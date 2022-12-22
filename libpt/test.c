#include <u.h>
#include <libc.h>
#include <draw.h>
#include <keyboard.h>
#include <memdraw.h>
#include <mouse.h>
#include <thread.h>
#include "pt.h"

#define MAX(a,b) ((a)>=(b)?(a):(b))

static Sfont *fo;
static Image *image;
static int invert, prefilter;
static Point offset = {8, 8};
static Sdopts opts = {
	.prefilter = 0,
	.gamma = 1.0,
};
static char info[128];
int mainstacksize = 32768;
static Image *fg;

static void
redraw(void)
{
	Rectangle r;

	lockdisplay(display);

	draw(screen, screen->r, display->white, nil, ZP);

	r = rectaddpt(screen->r, offset);
	r.max = screen->r.max;
	draw(screen, r, fg, image, ZP);

	r = screen->r;
	r.min.y = r.max.y - font->height - 4;
	r.min.x += 4;
	stringbg(screen, r.min, invert ? display->white : display->black, ZP, font, info, invert ? display->black : display->white, ZP);

	flushimage(display, 1);
	unlockdisplay(display);
}

static void *
loadall(char *path)
{
	uchar *d;
	uvlong sz;
	Dir *st;
	int f, r, n;

	d = nil;
	sz = 0;
	if((f = open(path, OREAD)) < 0 || (st = dirfstat(f)) == nil)
		goto end;
	sz = st->length;
	free(st);
	if((d = malloc(sz+1)) == nil)
		goto end;

	for(r = 0; r < sz; r += n){
		if((n = read(f, d+r, sz-r)) <= 0){
			free(d);
			d = nil;
			goto end;
		}
	}

end:
	if(d != nil)
		d[sz] = 0;
	close(f);
	return d;
}

void
threadmain(int argc, char **argv)
{
	Mousectl *mctl;
	Keyboardctl *kctl;
	uchar *data;
	char *s;
	Rune key;
	Mouse m;
	Alt a[] = {
		{ nil, &m, CHANRCV },
		{ nil, nil, CHANRCV },
		{ nil, &key, CHANRCV },
		{ nil, nil, CHANEND },
	};
	int oldheight, height;
	Point oldxy, oldoffset;
	int oldb;
	Rectangle rect;

	quotefmtinstall();

	if(argc < 2)
		sysfatal("usage");
	if((data = loadall(argv[1])) == nil)
		sysfatal("%r");
	s = "Hello there, 9fans. ljil|.";
	if(argc > 2 && (s = loadall(argv[2])) == nil)
		s = argv[2];

	threadsetname("libpt");
	if(initdraw(nil, nil, "libpt") < 0)
		sysfatal("initdraw: %r");
	fg = allocimage(display, Rect(0,0,1,1), screen->chan, 1, DBlack);
	display->locking = 1;
	unlockdisplay(display);
	if((mctl = initmouse(nil, screen)) == nil)
		sysfatal("initmouse: %r");
	if((kctl = initkeyboard(nil)) == nil)
		sysfatal("initkeyboard: %r");

	a[0].c = mctl->c;
	a[1].c = mctl->resizec;
	a[2].c = kctl->c;
	memimageinit();
	height = 32.0;
	oldheight = height;
	oldxy = ZP;

	goto changed;

	for(;;){
		oldb = m.buttons;
		redraw();

		switch(alt(a)){
		case 0: /* mouse */
			if(m.buttons != 0){
				if(oldb == 0){
					oldxy = m.xy;
					oldoffset = offset;
					oldheight = height;
				}else if(m.buttons == 4){
					height = oldheight + (m.xy.y - oldxy.y)/20.0;
					if(height < 4)
						height = 4;
					goto changed;
				}else if(m.buttons == 2){
					offset = addpt(oldoffset, subpt(m.xy, oldxy));
				}
			}
			break;

		case 1: /* resize */
			getwindow(display, Refnone);
			redraw();
			break;

		case 2: /* keyboard */
			switch(key){
			case 'q':
			case Kdel:
				goto end;
			case '-':
				height -= height < 6 ? 0 : 1;
				goto changed;
			case '+':
				height += 1;
				goto changed;
			case 'i':
				invert = !invert;
				goto changed;
			case Kleft:
				opts.prefilter--;
				if(prefilter < 0)
					prefilter = 0;
				goto changed;
			case Kright:
				opts.prefilter++;
				goto changed;
			case Kup:
				opts.gamma += 0.01;
				goto changed;
			case Kdown:
				opts.gamma -= 0.01;
				if(opts.gamma < 0.4)
					opts.gamma = 0.4;
				goto changed;
				break;
			case '\n':
				opts.gamma = 1.0;
				goto changed;
			}
		}
		continue;

changed:
		pt_freefont(fo);
		if((fo = pt_font(data, height)) == nil)
			sysfatal("%r");
		rect = pt_textrect(fo, s);
		freeimage(image);
		image = pt_textdraw(fo, s, rect, &opts);
	}

end:
	threadexitsall(nil);
}
