#include <u.h>
#include <libc.h>
#include <draw.h>
#include <keyboard.h>
#include <memdraw.h>
#include <mouse.h>
#include <thread.h>
#include "pt.h"

#define MAX(a,b) ((a)>=(b)?(a):(b))

#define rchr(s) (be ? ((s)[0]<<8 | (s)[1]) : ((s)[1]<<8 | (s)[0]))

static const uchar mark[] = {0x00, 0x00, 0xc0, 0xe0, 0xf0};

static int
utf16to8(uchar *o, int osz, const uchar *s, int sz)
{
	int i, be, c, c2, wr, j;

	i = 0;
	be = 1;
	if(s[0] == 0xfe && s[1] == 0xff)
		i += 2;
	else if(s[0] == 0xff && s[1] == 0xfe){
		be = 0;
		i += 2;
	}

	for(; i < sz-1 && osz > 1;){
		c = rchr(&s[i]);
		i += 2;
		if(c >= 0xd800 && c <= 0xdbff && i < sz-1){
			c2 = rchr(&s[i]);
			if(c2 >= 0xdc00 && c2 <= 0xdfff){
				c = 0x10000 | (c - 0xd800)<<10 | (c2 - 0xdc00);
				i += 2;
			}else
				return -1;
		}else if(c >= 0xdc00 && c <= 0xdfff)
			return -1;

		if(c < 0x80)
			wr = 1;
		else if(c < 0x800)
			wr = 2;
		else if(c < 0x10000)
			wr = 3;
		else
			wr = 4;

		osz -= wr;
		if(osz < 1)
			break;

		o += wr;
		for(j = wr; j > 1; j--){
			*(--o) = (c & 0xbf) | 0x80;
			c >>= 6;
		}
		*(--o) = c | mark[wr];
		o += wr;
	}

	*o = 0;
	return i;
}

#define STB_TRUETYPE_IMPLEMENTATION
static void *
STBTT_malloc(int x, void *)
{
	return malloc(x);
}
static void
STBTT_free(void *p, void *)
{
	free(p);
}
#define STBTT_malloc STBTT_malloc
#define STBTT_free STBTT_free
#define STBTT_ifloor(x)    ((int)floor(x))
#define STBTT_iceil(x)     ((int)ceil(x))
#define STBTT_sqrt(x)      sqrt(x)
#define STBTT_pow(x,y)     pow(x,y)
#define STBTT_fmod(x,y)    fmod(x,y)
#define STBTT_cos(x)       cos(x)
#define STBTT_acos(x)      acos(x)
#define STBTT_fabs(x)      fabs(x)
#define STBTT_assert(x)    assert(x)
#define STBTT_strlen(x)    strlen(x)
#define STBTT_memcpy       memmove
#define STBTT_memset       memset
typedef usize size_t;
#define NULL nil
#define STBTT_RASTERIZER_VERSION 2 /* that one is definitely FASTER on amd64 */
#include "pt_private.h"

Sfont *
pt_font(uchar *data, float height)
{
	Sfont *f;

	f = mallocz(sizeof(*f), 1);
	if(stbtt_InitFont(&f->fi, data, stbtt_GetFontOffsetForIndex(data, 0)) == 0){
		werrstr("stbtt_InitFont: %r");
		goto err;
	}

	f->scale = stbtt_ScaleForPixelHeight(&f->fi, height);
	f->height = height;
	stbtt_GetFontVMetrics(&f->fi, &f->asc, &f->desc, &f->linegap);
	f->lineh = f->asc - f->desc + f->linegap;

	stbtt_GetFontBoundingBox(&f->fi, &f->bb.min.x, &f->bb.min.y, &f->bb.max.x, &f->bb.max.y);
	f->bb.max.x = ceil(f->scale*(f->bb.max.x - f->bb.min.x));
	f->bb.max.y = ceil(f->scale*(f->bb.max.y - f->bb.min.y));
	f->bb.min.x = 0;
	f->bb.min.y = 0;

	f->bb.max.x = (f->bb.max.x + 3) & ~3;
	f->bb.max.y = (f->bb.max.y + 3) & ~3;

	if(badrect(f->bb)){
		werrstr("invalid glyph bbox: %R", f->bb);
		goto err;
	}
	if((f->g = allocmemimage(f->bb, GREY8)) == nil)
		goto err;
	if((f->b = allocmemimage(f->bb, GREY8)) == nil)
		goto err;
	memfillcolor(f->b, DWhite);

	return f;
err:
	werrstr("pt_font: %r");
	pt_freefont(f);
	return nil;
}

void
pt_freefont(Sfont *f)
{
	if(f == nil)
		return;
	freememimage(f->g);
	freememimage(f->b);
	free(f);
}

static u32int *
bsearch(u32int c, u32int *t, int n)
{
	u32int *p;
	int m;

	while(n > 1){
		m = n/2;
		p = t + m;
		if(c >= p[0]) {
			t = p;
			n = n-m;
		} else
			n = m;
	}
	if(n && c >= t[0])
		return t;
	return 0;
}

static int
code2glyph(Sfont *f, u32int c)
{
	return stbtt_FindGlyphIndex(&f->fi, c);
}

Rectangle
pt_textrect(Sfont *f, char *s)
{
	int i, n;
	int glyph, prevglyph, adv, kern, leftb;
	int w, h, maxw;
	Rune r;

	w = maxw = 0;
	h = f->lineh;
	prevglyph = 0;
	for(i = 0; s[i]; i += n){
		n = chartorune(&r, s+i);
		glyph = code2glyph(f, r);
		if(r != '\n' && glyph > 0){
			stbtt_GetGlyphHMetrics(&f->fi, glyph, &adv, &leftb);
			kern = prevglyph ? stbtt_GetGlyphKernAdvance(&f->fi, prevglyph, glyph) : 0;
			w += adv + kern;
		}else
			glyph = 0;
		prevglyph = glyph;
		maxw = MAX(maxw, w);
		if(r == '\n'){
			h += f->lineh;
			w = 0;
		}
	}
	w = ceil(f->scale*maxw);
	h = ceil(f->scale*(h+1));

	return Rect(0, 0, w, h);
}

Image *
pt_textdraw(Sfont *f, char *s, Rectangle rect, Sdopts *opts)
{
	int i, base, n, x0, y0, prefilter;
	int glyph, prevglyph, adv, leftb;
	float x, shift, subx, suby;
	Rune r;
	Rectangle o;
	Memimage *mi;
	Image *image;
	uchar *p;

	if((mi = allocmemimage(rect, GREY8)) == nil)
		return nil;
	memfillcolor(mi, DWhite);
	prevglyph = 0;
	x = 0.0f;
	o = rect;
	base = 0;
	prefilter = opts != nil ? opts->prefilter : 0;
	if(prefilter > STBTT_MAX_OVERSAMPLE)
		prefilter = STBTT_MAX_OVERSAMPLE;

	for(i = 0; s[i]; i += n){
		n = chartorune(&r, s+i);
		glyph = code2glyph(f, r);

		if(prevglyph > 0 && glyph > 0 && r != '\n')
			x += stbtt_GetGlyphKernAdvance(&f->fi, prevglyph, glyph) * f->scale;
		shift = x - floor(x);

		if(r != '\n' && glyph > 0){
			memfillcolor(f->g, DBlack);

			if(prefilter > 0){
				stbtt_MakeGlyphBitmapSubpixelPrefilter(
					&f->fi,
					byteaddr(f->g, f->bb.min),
					Dx(f->bb), Dy(f->bb),
					bytesperline(f->bb, f->g->depth),
					f->scale, f->scale,
					shift, 0.0f,
					prefilter, prefilter,
					&subx, &suby,
					glyph
				);
			}else{
				stbtt_MakeGlyphBitmapSubpixel(
					&f->fi,
					byteaddr(f->g, f->bb.min),
					Dx(f->bb), Dy(f->bb),
					bytesperline(f->bb, f->g->depth),
					f->scale, f->scale,
					shift, 0.0f,
					glyph
				);
			}

			stbtt_GetGlyphBitmapBoxSubpixel(
				&f->fi,
				glyph,
				f->scale, f->scale,
				shift, 0.0f,
				&x0, &y0, nil, nil
			);
			o.min.x = floor(x) + x0;
			o.min.y = base + f->asc*f->scale + y0;
			memimagedraw(mi, o, f->b, ZP, f->g, ZP, DoutS);
			stbtt_GetGlyphHMetrics(&f->fi, glyph, &adv, &leftb);
			x += adv * f->scale;
		}else
			glyph = 0;
		prevglyph = glyph;
		if(r == '\n'){
			x = x0 = 0;
			prevglyph = 0;
			base += ceil(f->scale * f->lineh);
		}
	}

	n = 60 + Dx(rect)*Dy(rect);
	p = malloc(n);
	n = unloadmemimage(mi, rect, p, n);
	freememimage(mi);

	image = allocimage(display, rect, GREY8, 0, DNofill);
	if(opts != nil && opts->gamma != 1.0){
		float igamma = 1.0/opts->gamma;
		for(i = 0; i < n; i++){
			if(p[i] < 255){
				float g = pow(p[i], igamma);
				p[i] = g < 256 ? g : 255;
			}
		}
	}
	for(i = 0; i < n; i++)
		p[i] = 255 - p[i];
	loadimage(image, rect, p, n);
	free(p);

	return image;
}
