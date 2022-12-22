/* slide margin */
int		margin 			= 32;

/* box (quote or code) internal padding */
int		padding 		= 12;

/* slide title font size */
float	ftitlesz		= 120.0;

/* text font size */
float	ftextsz			= 96.0;

/* fixed font size */
float	ffixedsz		= 72.0;

/* color definitions */
ulong	coldefs[Ncols] 	= {
	[Cbg]		= 0xFFFFFFFF,	/* slide background */
	[Cfg]		= 0x000000FF,	/* text color */
	[Cqbg]		= 0xFAFAFAFF,	/* quotes background */
	[Cqbord]	= 0xCCCCCCFF,	/* quotes border */
	[Ccbg] 		= 0xFFFFEAFF,	/* code background */
	[Ccbord]	= 0x99994CFF,	/* code border */
};
