#include "a.h"

uchar*
slurp(char *path)
{
	int fd;
	long r, n, s;
	uchar *buf;

	n = 0;
	s = 8192;
	buf = malloc(s);
	if(buf == nil)
		return nil;
	fd = open(path, OREAD);
	if(fd < 0)
		return nil;
	for(;;){
		r = read(fd, buf + n, s - n);
		if(r < 0)
			return nil;
		if(r == 0)
			break;
		n += r;
		if(n == s){
			s *= 1.5;
			buf = realloc(buf, s);
			if(buf == nil)
				return nil;
		}
	}
	buf[n] = 0;
	close(fd);
	return buf;
}

Sfont*
loadsfont(char *filename, float height)
{
	Sfont *sfont;
	uchar *data;

	data = slurp(filename);
	if(data == nil)
		sysfatal("loadsfont: %r");
	sfont = pt_font(data, height);
	if(sfont == nil)
		sysfatal("pt_font: %r");
	return sfont;
}

void*
emalloc(ulong n)
{
	void *p;

	p = malloc(n);
	if(p == nil)
		sysfatal("malloc: %r");
	return p;
}

Image*
ealloccol(ulong col)
{
	Image *i;

	i = allocimage(display, Rect(0, 0, 1, 1), screen->chan, 1, col);
	if(i == nil)
		sysfatal("allocimage: %r");
	return i;
}

