typedef struct Sdopts Sdopts;
typedef struct Sfont Sfont;
#pragma incomplete Sfont

struct Sdopts {
	int prefilter;
	float gamma;
};

struct Sfontinfo {
	char *family;
	char *subfamily;
	
};

Sfont *pt_font(uchar *data, float height);
void pt_freefont(Sfont *f);
Rectangle pt_textrect(Sfont *f, char *s);
Image *pt_textdraw(Sfont *f, char *s, Rectangle rect, Sdopts *opts);
