#include "stb_truetype.h"

struct Sfont {
	Memimage *g;
	Memimage *b;

	Rectangle bb;
	float scale;
	float height;
	int asc;
	int desc;
	int linegap;
	int lineh;

	stbtt_fontinfo fi;
};
