// HLE SKNS (and other YMZ280b-no-sound-cpu games) driver

#include "m1snd.h"

static void SKNS_Init(long srate);
static void SKNS_SendCmd(int cmda, int cmdb);

static unsigned int sk_readmem(unsigned int address);
static void sk_writemem(unsigned int address, unsigned int data);
static void skns_irq(int state);

#define ENABLE_ROCKN (1)

static struct YMZ280Binterface ymz280b_interface =
{
	1,
	{ 16934400 },
	{ RGN_SAMP1 },
	{ YM3012_VOL(100,MIXER_PAN_LEFT,100,MIXER_PAN_RIGHT) },
	{ skns_irq }
};

static M16502T dcrw = 
{
	sk_readmem,
	sk_readmem,
	sk_writemem,
};

M1_BOARD_START( skns )
	MDRV_NAME( "M1 HLE driver" )
	MDRV_HWDESC( "HLE, YMZ280B" )
	MDRV_DELAYS( 60, 60 )
	MDRV_INIT( SKNS_Init )
	MDRV_SEND( SKNS_SendCmd )

	MDRV_SOUND_ADD(YMZ280B, &ymz280b_interface)
M1_BOARD_END

// defines a single sample
typedef struct
{
	int fnum;
	int level;
	int pan;
	int start;
	int loopstart;
	int loopstop;
	int stop;

} SampleT;

// defines one sequence step in a song
typedef struct
{
	int flags;
	int sample;
} MusicT;

#define FL_LSTART 1
#define FL_LOOP	  2
#define FL_END	  3
#define FL_SLOOP  4

// song pointer flags
#define SP_SINGLE_LOOP	0x00001000
#define SP_SINGLE_SHOT	0x00002000
#define SP_SINGLE_SLOOP	0x00003000

// dummy song used by autogen
static MusicT dummysong[6];

// cyvern level 1 sequence data (song 3 in sound test)
static MusicT song3[] = 
{
	{ 0, 2 },
	{ 0, 3 },
	{ 0, 4 },
	{ FL_LSTART, 5 },
	{ 0, 6 },
	{ FL_LOOP, -1 },	
};

// cyvern level 1 without the intro (song 4 in sound test)
static MusicT song4[] = 
{
	{ 0, 3 },
	{ 0, 4 },
	{ FL_LSTART, 5 },
	{ 0, 6 },
	{ FL_LOOP, -1 },	
};

// cyvern level 2 (song 6 in sound test)
static MusicT song6[] = 
{
	{ 0, 8 },
	{ FL_LSTART, 9 },
	{ FL_LOOP, -1 },	
};

// cyvern "tour of all the songs" (good ending?)
static MusicT song11[] = 
{
	{ 0, 2 },
	{ 0, 3 },
	{ 0, 4 },
	{ 0, 5 },
	{ 0, 8 },
	{ 0, 9 },
	{ 0, 13 },
	{ 0, 16 },
	{ 0, 18 },
	{ 0, 15 },
	{ 0, 20 },
	{ FL_LSTART, 21 },
	{ FL_LOOP, -1 },	
};

// cyvern ending only (bad ending?)
static MusicT song13[] = 
{
	{ 0, 20 },
	{ FL_LSTART, 21 },
	{ FL_LOOP, -1 },	
};

// null song
static MusicT song14[] = 
{
	{ FL_END, -1 },
};

// test song
static MusicT song15[] = 
{
	{ 0, 137 },
	{ FL_END, -1 },
};

static MusicT *cyvsongs[] =
{
	song14,
	(MusicT *)(SP_SINGLE_SHOT | 0),
	(MusicT *)(SP_SINGLE_LOOP | 1),
	song3,
	song4,
	(MusicT *)(SP_SINGLE_LOOP | 7),
	song6,
	(MusicT *)(SP_SINGLE_LOOP | 10),
	(MusicT *)(SP_SINGLE_LOOP | 11),
	(MusicT *)(SP_SINGLE_LOOP | 12),
	(MusicT *)(SP_SINGLE_LOOP | 13),
	(MusicT *)(SP_SINGLE_LOOP | 14),
	(MusicT *)(SP_SINGLE_LOOP | 15),
	(MusicT *)(SP_SINGLE_LOOP | 16),
	(MusicT *)(SP_SINGLE_LOOP | 17),
	(MusicT *)(SP_SINGLE_LOOP | 18),
	(MusicT *)(SP_SINGLE_LOOP | 19),
	song11,
	(MusicT *)(SP_SINGLE_LOOP | 22),
	song13,
	song14,
};

static MusicT *guwangesongs[] =
{
	song14,
	(MusicT *)(SP_SINGLE_LOOP | 23),
	(MusicT *)(SP_SINGLE_LOOP | 24),
	(MusicT *)(SP_SINGLE_LOOP | 25),
	(MusicT *)(SP_SINGLE_LOOP | 26),
	(MusicT *)(SP_SINGLE_LOOP | 27),
	(MusicT *)(SP_SINGLE_LOOP | 28),
	(MusicT *)(SP_SINGLE_LOOP | 29),
	(MusicT *)(SP_SINGLE_SLOOP | 30),
	(MusicT *)(SP_SINGLE_LOOP | 31),
	(MusicT *)(SP_SINGLE_LOOP | 32), // 10
	(MusicT *)(SP_SINGLE_SHOT | 33), // 11
	(MusicT *)(SP_SINGLE_LOOP | 34),
	song14,
	song14,
};

// tetris p2

static MusicT *tetrisp2songs[] =
{
	song14,
	(MusicT *)(SP_SINGLE_LOOP | 35),
	(MusicT *)(SP_SINGLE_LOOP | 36),
	(MusicT *)(SP_SINGLE_LOOP | 37),
	(MusicT *)(SP_SINGLE_LOOP | 38),
	(MusicT *)(SP_SINGLE_LOOP | 39),
	(MusicT *)(SP_SINGLE_LOOP | 40),
	(MusicT *)(SP_SINGLE_LOOP | 41),
	(MusicT *)(SP_SINGLE_SHOT | 42),
	(MusicT *)(SP_SINGLE_LOOP | 43),
	(MusicT *)(SP_SINGLE_LOOP | 44),
	(MusicT *)(SP_SINGLE_LOOP | 45),
	(MusicT *)(SP_SINGLE_LOOP | 46),
	(MusicT *)(SP_SINGLE_LOOP | 47),
	(MusicT *)(SP_SINGLE_LOOP | 48),
	(MusicT *)(SP_SINGLE_LOOP | 49),
	(MusicT *)(SP_SINGLE_LOOP | 50),
	(MusicT *)(SP_SINGLE_LOOP | 51),
	(MusicT *)(SP_SINGLE_LOOP | 52),
	(MusicT *)(SP_SINGLE_LOOP | 53),
	(MusicT *)(SP_SINGLE_SHOT | 54),
	(MusicT *)(SP_SINGLE_SHOT | 55),
	(MusicT *)(SP_SINGLE_LOOP | 56),
	(MusicT *)(SP_SINGLE_SHOT | 57),
	song14,
};

static MusicT *sengekisongs[] =
{
	song14,
	(MusicT *)(SP_SINGLE_LOOP | 86),
	(MusicT *)(SP_SINGLE_LOOP | 87),
	(MusicT *)(SP_SINGLE_LOOP | 88),
	(MusicT *)(SP_SINGLE_LOOP | 89),
	(MusicT *)(SP_SINGLE_LOOP | 90),
	(MusicT *)(SP_SINGLE_LOOP | 91),
	(MusicT *)(SP_SINGLE_LOOP | 92),
	(MusicT *)(SP_SINGLE_LOOP | 93),
	(MusicT *)(SP_SINGLE_SHOT | 94),
	(MusicT *)(SP_SINGLE_SHOT | 95),
	(MusicT *)(SP_SINGLE_LOOP | 96),
	song14,
};

static MusicT *uopokosongs[] =
{
	song14,
	(MusicT *)(SP_SINGLE_SHOT | 97),
	(MusicT *)(SP_SINGLE_LOOP | 98),
	(MusicT *)(SP_SINGLE_LOOP | 99),
	(MusicT *)(SP_SINGLE_LOOP | 100),
	(MusicT *)(SP_SINGLE_LOOP | 101),
	(MusicT *)(SP_SINGLE_LOOP | 102),
	(MusicT *)(SP_SINGLE_LOOP | 103),
	(MusicT *)(SP_SINGLE_LOOP | 104),
	(MusicT *)(SP_SINGLE_LOOP | 105),
	(MusicT *)(SP_SINGLE_LOOP | 106),
	(MusicT *)(SP_SINGLE_LOOP | 107),
	(MusicT *)(SP_SINGLE_LOOP | 108),
	(MusicT *)(SP_SINGLE_LOOP | 109),
	(MusicT *)(SP_SINGLE_LOOP | 110),
	song14,
};

static MusicT *espradesongs[] =
{
	song14,
	(MusicT *)(SP_SINGLE_LOOP | 111),
	(MusicT *)(SP_SINGLE_LOOP | 112),
	(MusicT *)(SP_SINGLE_LOOP | 113),
	(MusicT *)(SP_SINGLE_LOOP | 114),
	(MusicT *)(SP_SINGLE_LOOP | 115),
	(MusicT *)(SP_SINGLE_LOOP | 116),
	(MusicT *)(SP_SINGLE_LOOP | 117),
	(MusicT *)(SP_SINGLE_LOOP | 118),
	(MusicT *)(SP_SINGLE_SLOOP | 119),
	(MusicT *)(SP_SINGLE_LOOP | 120),
	(MusicT *)(SP_SINGLE_LOOP | 121),
	(MusicT *)(SP_SINGLE_LOOP | 122),
	song14,
};

static MusicT *feveronsongs[] =
{
	song14,
	(MusicT *)(SP_SINGLE_LOOP | 123),
	(MusicT *)(SP_SINGLE_LOOP | 124),
	(MusicT *)(SP_SINGLE_LOOP | 125),
	(MusicT *)(SP_SINGLE_SHOT | 126),
	(MusicT *)(SP_SINGLE_SHOT | 127),
	(MusicT *)(SP_SINGLE_LOOP | 128),
	(MusicT *)(SP_SINGLE_LOOP | 129),
	(MusicT *)(SP_SINGLE_SHOT | 130),
	(MusicT *)(SP_SINGLE_LOOP | 131),
	(MusicT *)(SP_SINGLE_LOOP | 132),
	(MusicT *)(SP_SINGLE_LOOP | 133),
	(MusicT *)(SP_SINGLE_LOOP | 134),
	(MusicT *)(SP_SINGLE_LOOP | 135),
	song14,
};

static MusicT *testsongs[] =
{
	song15,
	song15,
	song15
};

// rockn 1
#if ENABLE_ROCKN
static MusicT *rockn1songs[] =
{
	song14,
//	(MusicT *)(SP_SINGLE_SHOT | 58 | 0x200),
	(MusicT *)(SP_SINGLE_SHOT | 59),
	(MusicT *)(SP_SINGLE_SHOT | 60 | 0x100),
	(MusicT *)(SP_SINGLE_SHOT | 61 | 0x200),
	(MusicT *)(SP_SINGLE_SHOT | 62 | 0x100),
	(MusicT *)(SP_SINGLE_SHOT | 63 | 0x200),
	(MusicT *)(SP_SINGLE_SHOT | 64 ),
	(MusicT *)(SP_SINGLE_SHOT | 65 | 0x200),
	(MusicT *)(SP_SINGLE_SHOT | 66 | 0x100),
	(MusicT *)(SP_SINGLE_SHOT | 67 ),
	(MusicT *)(SP_SINGLE_SHOT | 68 | 0x300),
	(MusicT *)(SP_SINGLE_SHOT | 69 | 0x100),
	(MusicT *)(SP_SINGLE_SHOT | 70 ),
	(MusicT *)(SP_SINGLE_SHOT | 71 | 0x100),
	(MusicT *)(SP_SINGLE_SHOT | 72 ),
	(MusicT *)(SP_SINGLE_SHOT | 73 | 0x200),
	(MusicT *)(SP_SINGLE_SHOT | 74 | 0x300),
	(MusicT *)(SP_SINGLE_SHOT | 75 | 0x400),
	(MusicT *)(SP_SINGLE_SHOT | 76 | 0x300),
	(MusicT *)(SP_SINGLE_SHOT | 77 | 0x400),
	(MusicT *)(SP_SINGLE_SHOT | 78 | 0x400),
	(MusicT *)(SP_SINGLE_SHOT | 79 | 0x300),
	(MusicT *)(SP_SINGLE_SHOT | 80 ),
	(MusicT *)(SP_SINGLE_SHOT | 81 ),
	(MusicT *)(SP_SINGLE_SHOT | 82 ),
	(MusicT *)(SP_SINGLE_SHOT | 83 ),
	(MusicT *)(SP_SINGLE_SHOT | 84 ),
	(MusicT *)(SP_SINGLE_SHOT | 85 ),
	song14,			
	song14,			
	song14,
};
#endif

static MusicT *cursong;
static int lp_start, current;
static int topsong;

// YMZ280B register dumps for each song, captured from MAME
static SampleT commands[] =
{
	// cyvern
	{ 0x81, 0xd0, 8, 0x1b3c4e, 0x1b3c4e, 0x23489e, 0x23489e },	// intro
	{ 0x81, 0x90, 8, 0x2348d0, 0x2348d0, 0x2544dc, 0x2544dc },	// select screen
	{ 0x81, 0xa0, 8, 0x5aa41a, 0x5aa41a, 0x5c8de2, 0x5c8de2 },	// level 1 (2)
	{ 0x81, 0xa0, 8, 0x25450e, 0x25450e, 0x276238, 0x276238 },	
	{ 0x81, 0xa0, 8, 0x27626a, 0x27626a, 0x280db6, 0x280db6 },	
	{ 0x81, 0xa0, 8, 0x62e908, 0x62e908, 0x6aa014, 0x6aa014 },	// (5)
	{ 0x81, 0xa0, 8, 0x6aa046, 0x6aa046, 0x6b4ae2, 0x6b4ae2 },	
	{ 0x81, 0xc0, 8, 0x280de8, 0x280de8, 0x2bb046, 0x2bb046 },	
	{ 0x81, 0xc0, 8, 0x6b4b14, 0x6b4b14, 0x6d49b6, 0x6d49b6 },	
	{ 0x81, 0xc0, 8, 0x2bb078, 0x2bb078, 0x2f9dd8, 0x2f9dd8 },	
	{ 0x81, 0xc0, 8, 0x2f9e0a, 0x2f9e0a, 0x367ed2, 0x367ed2 },	// (10)
	{ 0x81, 0xc0, 8, 0x367f04, 0x367f04, 0x3bff94, 0x3bff94 },	
	{ 0x81, 0xc0, 8, 0x3bffc6, 0x3bffc6, 0x404d78, 0x404d78 },	
	{ 0x81, 0xc0, 8, 0x404daa, 0x404daa, 0x447f86, 0x447f86 },	
	{ 0x81, 0xc0, 8, 0x447fb8, 0x447fb8, 0x4b3ac8, 0x4b3ac8 },	
	{ 0x81, 0xc0, 8, 0x4b3afa, 0x4b3afa, 0x512a58, 0x512a58 },	// (15)
	{ 0x81, 0xc0, 8, 0x512a8a, 0x512a8a, 0x56edee, 0x56edee },	
	{ 0x81, 0xc0, 8, 0x56ee20, 0x56ee20, 0x5aa3e8, 0x5aa3e8 },	
	{ 0x81, 0xc0, 8, 0x6d49e8, 0x6d49e8, 0x71b480, 0x71b480 },	
	{ 0x81, 0xc0, 8, 0x5c8e14, 0x5c8e14, 0x60e4ac, 0x60e4ac },	
	{ 0x81, 0xc0, 8, 0x7594c0, 0x7594c0, 0x79b7be, 0x79b7be },	// (20)
	{ 0x81, 0xc0, 8, 0x79b7f0, 0x79b7f0, 0x7b688c, 0x7b688c },	
	{ 0x81, 0xc0, 8, 0x60e50e, 0x60e50e, 0x62e8a6, 0x62e8a6 },	// (22)

	// guwange
        { 0x5d, 0x90, 0x8, 0x0, 0x2, 0x103be6, 0x103be8 },		// (23)
        { 0x5d, 0x70, 0x8, 0x103bea, 0x103bec, 0x132be6, 0x132be8 }, 	// (24)
        { 0x5d, 0xb0, 0x8, 0x132bea, 0x132bec, 0x187290, 0x187292 },	// (25)
        { 0x5d, 0xc0, 0x8, 0x187294, 0x187296, 0x1cb5ba, 0x1cb5bc },
        { 0x5d, 0xc0, 0x8, 0x1cb5be, 0x1d95a6, 0x211524, 0x211526 },
        { 0x5d, 0xb0, 0x8, 0x211528, 0x21152a, 0x36f58e, 0x36f590 },
        { 0x5d, 0xf0, 0x8, 0x36f592, 0x36f594, 0x3e91f8, 0x3e91fa },
        { 0x5d, 0xf0, 0x8, 0x3e91fc, 0x405f90, 0x43fae2, 0x43fae4 },	// (30)
        { 0x5d, 0xb0, 0x8, 0x43fae6, 0x43fae8, 0x45ef0c, 0x45ef0e },
        { 0x5d, 0xff, 0x8, 0x45ef10, 0x48c202, 0x540df6, 0x540df8 },
        { 0x5d, 0xa8, 0x8, 0x540dfa, 0x540dfa, 0x55b956, 0x55b956 },
        { 0x5d, 0xff, 0x8, 0x55b958, 0x5784f4, 0x5fab80, 0x5fab80 },	// (34)

	// tetris p2
        { 0x3f, 0x30, 0x8, 0x600000, 0x600002, 0x6506d2, 0x6506d2 },	// (35) (1)
        { 0x3f, 0x30, 0x8, 0x27f444, 0x27f446, 0x2d35e8, 0x2d35e8 },	// (36)
        { 0x3f, 0x30, 0x8, 0x2d35ea, 0x2d35ec, 0x2fb238, 0x2fb238 },
        { 0x3f, 0x30, 0x8, 0x2fb23a, 0x2fb23c, 0x309c90, 0x309c90 },
        { 0x3f, 0x30, 0x8, 0x14f0ec, 0x14f0ee, 0x1a410e, 0x1a410e },
        { 0x3f, 0x30, 0x8, 0x309c92, 0x309c94, 0x3948fc, 0x3948fc },    // (40)
        { 0x3f, 0x30, 0x8, 0x400000, 0x400002, 0x40c9c2, 0x40c9c2 },
        { 0x3f, 0x30, 0x8, 0x40c9c4, 0x40c9c6, 0x468e24, 0x468e24 },
        { 0x3f, 0x30, 0x8, 0x6506d4, 0x6506d6, 0x6af40e, 0x6af40e },
        { 0x3f, 0x30, 0x8, 0x6af410, 0x6af412, 0x7125cc, 0x7125cc },
        { 0x3f, 0x30, 0x8, 0x7125ce, 0x7125d0, 0x7851e2, 0x7851e2 },	// (45) (song c)
        { 0x3f, 0x30, 0x8, 0x7851e4, 0x7851e6, 0x7ffd30, 0x7ffd30 },
	{ 0x3f, 0x30, 0x8, 0x468e26, 0x468e28, 0x48226c, 0x48226c }, // (song e)
        { 0x3f, 0x30, 0x8, 0x48226e, 0x482270, 0x4b0486, 0x4b0486 },
        { 0x3f, 0x30, 0x8, 0x1a4110, 0x1a4112, 0x1e3d58, 0x1e3d58 },
        { 0x3f, 0x30, 0x8, 0x200000, 0x200002, 0x27f442, 0x27f442 },	// (50)
        { 0x3f, 0x30, 0x8, 0x4b0488, 0x4b048a, 0x4d2bfc, 0x4d2bfc },
        { 0x3f, 0x30, 0x8, 0x4d2bfe, 0x4d2c00, 0x517b54, 0x517b54 },
        { 0x3f, 0x30, 0x8, 0x517b56, 0x517b58, 0x5208a0, 0x5208a0 },
        { 0x3f, 0x30, 0x8, 0x5208a2, 0x5208a4, 0x52b762, 0x52b762 },
        { 0x3f, 0x30, 0x8, 0x52b764, 0x52b766, 0x534a48, 0x534a48 },	// (55)
        { 0x3f, 0x30, 0x8, 0x534a4a, 0x534a4c, 0x55cda0, 0x55cda0 },	// (56)
        { 0x3f, 0x30, 0x8, 0x55cda2, 0x55cda4, 0x5adaee, 0x5adaee },	// (57)

	// rock n tread 1
        { 0x7f, 0x0, 0x8, 0xe4302a, 0xe4302c, 0x10051a0, 0x10051a0 },	// (58)
        { 0x7f, 0x33, 0x8, 0x1378a0a, 0x1378a0c, 0x15a36c6, 0x15a36c6 },
        { 0x7f, 0x33, 0x8, 0x17091e4, 0x17091e6, 0x18adf1c, 0x18adf1c },
        { 0x7f, 0x33, 0x8, 0x125fd5a, 0x125fd5c, 0x141dc98, 0x141dc98 },	// (61)
        { 0x7f, 0x33, 0x8, 0x11d724e, 0x11d7250, 0x13a02c8, 0x13a02c8 },
        { 0x7f, 0x33, 0x8, 0xa6438c, 0xa6438e, 0xbf8d98, 0xbf8d98 },
        { 0x7f, 0x33, 0x8, 0xa8ef02, 0xa8ef04, 0xc66036, 0xc66036 },
        { 0x7f, 0x33, 0x8, 0x17a8568, 0x17a856a, 0x195f9d6, 0x195f9d6 },
        { 0x7f, 0x33, 0x8, 0xd40af2, 0xd40af4, 0xf74c60, 0xf74c60 },		// (65)
        { 0x7f, 0x33, 0x8, 0x18e388e, 0x18e3890, 0x1aa5328, 0x1aa5328 },
        { 0x7f, 0x33, 0x8, 0xd28f34, 0xd28f36, 0xeec73a, 0xeec73a },
        { 0x7f, 0x33, 0x8, 0x8e28ac, 0x8e28ae, 0xafc0ea, 0xafc0ea },
        { 0x7f, 0x33, 0x8, 0x1df4812, 0x1df4814, 0x1fad6be, 0x1fad6be },
        { 0x7f, 0x33, 0x8, 0x1bd23b0, 0x1bd23b2, 0x1df6ca4, 0x1df6ca4 },	// (70)
        { 0x7f, 0x33, 0x8, 0xf17a8a, 0xf17a8c, 0x10d7288, 0x10d7288 },	
        { 0x7f, 0x33, 0x8, 0x1d6f630, 0x1d6f632, 0x1f328b4, 0x1f328b4 },
        { 0x7f, 0x33, 0x8, 0x1bf453e, 0x1bf4540, 0x1de8ec0, 0x1de8ec0 },
        { 0x7f, 0x33, 0x8, 0xa6ca2e, 0xa6ca30, 0xc4cbba, 0xc4cbba },
        { 0x7f, 0x33, 0x8, 0x177f88a, 0x177f88c, 0x1979ff6, 0x1979ff6 },	// (75)
        { 0x7f, 0x33, 0x8, 0x15eebf2, 0x15eebf4, 0x17e3b18, 0x17e3b18 },
        { 0x7f, 0x33, 0x8, 0x102a364, 0x102a366, 0x12317ca, 0x12317ca },
        { 0x7f, 0x33, 0x8, 0x12bbe2c, 0x12bbe2e, 0x14b07aa, 0x14b07aa },
        { 0xff, 0x33, 0x8, 0x0, 0x2, 0x35d38, 0x35d38 },
        { 0xff, 0x33, 0x8, 0x35d3a, 0x35d3c, 0x3263ac, 0x3263ac },		// (80)
        { 0xff, 0x33, 0x8, 0x3263ae, 0x3263b0, 0x39c72c, 0x39c72c },	
        { 0xff, 0x33, 0x8, 0x39c72e, 0x39c730, 0x41ecfc, 0x41ecfc },
        { 0xff, 0x33, 0x8, 0x41ecfe, 0x41ed00, 0x45b8fc, 0x45b8fc },
        { 0xff, 0x33, 0x8, 0x45b8fe, 0x45b900, 0x5b8b3c, 0x5b8b3c },	// (84)

	// Sengeki Striker	
        { 0x81, 0xfd, 0x8, 0x6d994, 0x6d994, 0x122c54, 0x122c54 },	// (85)
        { 0x81, 0xfd, 0x8, 0x122c86, 0x122c86, 0x1c0b06, 0x1c0b06 },
        { 0x81, 0xfd, 0x8, 0x1c0b38, 0x1c0b38, 0x25e9f8, 0x25e9f8 },
        { 0x81, 0xfd, 0x8, 0x25ea2a, 0x25ea2a, 0x2fc6e2, 0x2fc6e2 },
        { 0x81, 0xfd, 0x8, 0x2fc714, 0x2fc714, 0x383424, 0x383424 },
        { 0x81, 0xfd, 0x8, 0x383456, 0x383456, 0x4283d6, 0x4283d6 },	// (90)
        { 0x81, 0xfd, 0x8, 0x428408, 0x428408, 0x4c6190, 0x4c6190 },
        { 0x81, 0xfd, 0x8, 0x4c61c2, 0x4c61c2, 0x55a312, 0x55a312 },
        { 0x81, 0xfd, 0x8, 0x55a344, 0x55a344, 0x56e144, 0x56e144 },
        { 0x81, 0xfd, 0x8, 0x56e176, 0x56e176, 0x581856, 0x581856 },
        { 0x81, 0xfd, 0x8, 0x581888, 0x581888, 0x722188, 0x722188 },	// (96)

	// Uo Poko
        { 0x7f, 0xdf, 8, 0x000000, 0x000000, 0x000000, 0x049ca2 },	// (97)
        { 0x7f, 0xff, 8, 0x049ca4, 0x049ca6, 0x074dca, 0x074dca },
        { 0x7f, 0xcf, 8, 0x074dcc, 0x074dce, 0x0bca08, 0x0bca08 },
        { 0x7f, 0x8f, 8, 0x0bca0a, 0x0bca0c, 0x108a1a, 0x108a1a },	// (100)        
        { 0x7f, 0xbf, 8, 0x108a1c, 0x108a1e, 0x131024, 0x131024 },	
        { 0x7f, 0xcf, 8, 0x131026, 0x131028, 0x174a26, 0x174a26 },
        { 0x7f, 0xef, 8, 0x174a28, 0x174a2a, 0x1cab7e, 0x1cab7e },
        { 0x7f, 0xcf, 8, 0x1cab80, 0x1cab82, 0x1f5c92, 0x1f5c92 },
        { 0x7f, 0x9f, 8, 0x1f5c94, 0x1f5c96, 0x22ba16, 0x22ba16 },	// (105) 
        { 0x7f, 0xcf, 8, 0x22ba18, 0x22ba1a, 0x24b300, 0x24b300 },	
        { 0x7f, 0x8f, 8, 0x24b302, 0x000000, 0x000000, 0x259780 },
        { 0x7f, 0x8f, 8, 0x259782, 0x259784, 0x2a694a, 0x2a694a },
        { 0x7f, 0xcf, 8, 0x2a694c, 0x2a694e, 0x2d1a8e, 0x2d1a8e },
        { 0x7f, 0xdf, 8, 0x2d1a90, 0x2d1a92, 0x2fa0d4, 0x2fa0d4 },	// (110)

	// esprade
        { 0x5d, 0xef, 8, 0x000000, 0x003086, 0x0dcc4c, 0x0dd044 },	// (111)
        { 0x5d, 0xff, 8, 0x0dd046, 0x113ca0, 0x200e68, 0x200eb4 },
        { 0x5d, 0x8f, 8, 0x200eb6, 0x201b68, 0x2d098a, 0x2d09de },
        { 0x5d, 0xd3, 8, 0x2d09e0, 0x2d3238, 0x308b2a, 0x309842 },
        { 0x5d, 0xcf, 8, 0x309844, 0x309846, 0x380676, 0x3806c0 },	// (115)
        { 0x5d, 0xeb, 8, 0x3806c2, 0x38071a, 0x3bc688, 0x3bc6c0 },
        { 0x5d, 0xe7, 8, 0x3bc6c2, 0x3be2b8, 0x4a4f9e, 0x4a5038 },
        { 0x5d, 0xcb, 8, 0x4a503a, 0x4b7f9a, 0x5564a4, 0x5564ee },
        { 0x5d, 0xc3, 8, 0x5564f0, 0x556fae, 0x5abfec, 0x5ac038 },
        { 0x5d, 0xff, 8, 0x5ac03a, 0x000000, 0x000000, 0x5c56b6 },	// (120)
        { 0x5d, 0xbf, 8, 0x5c56b8, 0x5c56ba, 0x5cc092, 0x5cc094 },
        { 0x5d, 0xf7, 8, 0x5cc096, 0x5dadf6, 0x68556e, 0x6855b8 },	// (122)

	// dfeveron
        { 0x5d, 0x78, 8, 0x000000, 0x000002, 0x0770c6, 0x0770c8 },	// (123)
        { 0x5d, 0x6a, 8, 0x0770ca, 0x0770cc, 0x1048d6, 0x1048d8 },
        { 0x5d, 0x69, 8, 0x1048da, 0x1048dc, 0x1624ce, 0x1624d0 },	// (125)
        { 0x5d, 0x4e, 8, 0x1624d2, 0x000000, 0x000000, 0x1753c8 },
        { 0x5d, 0x69, 8, 0x1753ca, 0x000000, 0x000000, 0x1c6ae8 },
        { 0x5d, 0x60, 8, 0x1c6aea, 0x1c6aec, 0x2052d6, 0x2052d8 },
        { 0x5d, 0x60, 8, 0x2052da, 0x2052dc, 0x26d52e, 0x26d530 },
        { 0x5d, 0x60, 8, 0x26d532, 0x000000, 0x000000, 0x28b5d4 },	// (130)
        { 0x5d, 0xa2, 8, 0x2904d4, 0x29378c, 0x335b76, 0x335be2 },
        { 0x5d, 0x88, 8, 0x335be4, 0x335c4a, 0x3e5808, 0x3e5888 },
        { 0x5d, 0x67, 8, 0x3e588a, 0x3e58fe, 0x4bdc84, 0x4bdc86 },
        { 0x5d, 0x73, 8, 0x4bdc88, 0x4bdcbc, 0x5b3684, 0x5b3686 },
        { 0x5d, 0x6f, 8, 0x5b3688, 0x5b36be, 0x654764, 0x6547ce },	// (135)

	// guwange revisited
        { 0x5d, 0xf0, 0x8, 0x405f90, 0x405f90, 0x43fae2, 0x43fae2 },	// (136)

	// test song for hell night
        { 0x7f, 0xf0, 0x8, 0x0, 0x0, 0x3ffffe, 0x3ffffe },	// (137)

	// rock n tread 2
	#if 0
bank 1
         { 0x7f, 0x0, 0x8, 0x11d724e, 0x11d7250, 0x13a02c8, 0x13a02c8 },
bank 0
         { 0x7f, 0x34, 0x8, 0x18e388e, 0x18e3890, 0x1aa5328, 0x1aa5328 },
bank 5
         { 0x7f, 0x34, 0x8, 0x1ba9f50, 0x1ba9f52, 0x1d9e8d2, 0x1d9e8d2 },
bank 1
         { 0x7f, 0x34, 0x8, 0xd40af2, 0xd40af4, 0xf74c60, 0xf74c60 },
bank 0
         { 0x7f, 0x34, 0x8, 0x1378a0a, 0x1378a0c, 0x15a36c6, 0x15a36c6 },
bank 2
         { 0x7f, 0x34, 0x8, 0x1e10a68, 0x1e10a6a, 0x1ff0bf4, 0x1ff0bf4 },
bank 2
         { 0x7f, 0x34, 0x8, 0x19a98cc, 0x19a98ce, 0x1ba4038, 0x1ba4038 },
bank 3
         { 0x7f, 0x34, 0x8, 0x1df6ebe, 0x1df6ec0, 0x1ffe324, 0x1ffe324 },
bank 5
         { 0x7f, 0x34, 0x8, 0xab189c, 0xab189e, 0xca444a, 0xca444a },
bank 3
         { 0x7f, 0x34, 0x8, 0xc2ca1c, 0xc2ca1e, 0xe1bfc6, 0xe1bfc6 },
bank 5
         { 0x7f, 0x34, 0x8, 0x1047950, 0x1047952, 0x1213af4, 0x1213af4 },
bank 5
         { 0x7f, 0x34, 0x8, 0x1446e94, 0x1446e96, 0x161196a, 0x161196a },
bank 6
         { 0x7f, 0x34, 0x8, 0x1ca88a2, 0x1ca88a4, 0x1ead422, 0x1ead422 },
bank 3
         { 0x7f, 0x34, 0x8, 0x123f6d2, 0x123f6d4, 0x1458a32, 0x1458a32 },
bank 8
         { 0x7f, 0x34, 0x8, 0x12c3ef2, 0x12c3ef4, 0x14d8d50, 0x14d8d50 },
bank 2
         { 0x7f, 0x34, 0x8, 0xb8b14a, 0xb8b14c, 0xd6f2d0, 0xd6f2d0 },
bank 3
         { 0x7f, 0x34, 0x8, 0x1816cda, 0x1816cdc, 0x1a19714, 0x1a19714 },
bank 5
         { 0x7f, 0x34, 0x8, 0x199abf2, 0x199abf4, 0x1b605ce, 0x1b605ce },
bank 8
         { 0x7f, 0x34, 0x8, 0x188c8b0, 0x188c8b2, 0x1a8fd78, 0x1a8fd78 },
bank 2
         { 0x7f, 0x34, 0x8, 0x1072f7c, 0x1072f7e, 0x1224a22, 0x1224a22 },
bank 2
         { 0x7f, 0x34, 0x8, 0x14faf4a, 0x14faf4c, 0x16c1aee, 0x16c1aee },
bank 6
         { 0x7f, 0x34, 0x8, 0xb11e0c, 0xb11e0e, 0xd169be, 0xd169be },
bank 6
         { 0x7f, 0x34, 0x8, 0xf9f908, 0xf9f90a, 0x11967e2, 0x11967e2 },
bank 6
         { 0x7f, 0x34, 0x8, 0x13e32f8, 0x13e32fa, 0x15ece4a, 0x15ece4a },
bank 6
         { 0x7f, 0x34, 0x8, 0x193f774, 0x193f776, 0x1b463ac, 0x1b463ac },
bank 8
         { 0x7f, 0x34, 0x8, 0xcb1ce0, 0xcb1ce2, 0xeb3e5a, 0xeb3e5a },
bank 4
         { 0x7f, 0x34, 0x8, 0xac1b18, 0xac1b1a, 0xc9344e, 0xc9344e },
bank 4
         { 0x7f, 0x34, 0x8, 0xf20c9e, 0xf20ca0, 0x1113e82, 0x1113e82 },
bank 4
         { 0x7f, 0x34, 0x8, 0x1500876, 0x1500878, 0x170cde6, 0x170cde6 },
bank 8
         { 0x7f, 0x34, 0x8, 0x1e05d2a, 0x1e05d2c, 0x1fefd50, 0x1fefd50 },
bank 0
         { 0x7f, 0x34, 0x8, 0x41d7e6, 0x41d7e8, 0x438680, 0x438680 },
bank 0
         { 0x7f, 0x34, 0x8, 0x43b4d8, 0x43b4da, 0x5b3810, 0x5b3810 },
bank 0
         { 0x7f, 0x34, 0x8, 0x5c2ace, 0x5c2ad0, 0x5fdc8c, 0x5fdc8c },
bank 0
         { 0x7f, 0x34, 0x8, 0x60a73a, 0x60a73c, 0x64cdca, 0x64cdca },
bank 0
         { 0x7f, 0x34, 0x8, 0x657338, 0x65733a, 0x675936, 0x675936 },
bank 0
         { 0x7f, 0x34, 0x8, 0x675938, 0x67593a, 0x724256, 0x724256 },	
	#endif

	// rock n' megasession
	#if 0
bank 4
         { 0x7f, 0x34, 0x8, 0x1500672, 0x1500674, 0x16c98cc, 0x16c98cc },
bank 6
         { 0x7f, 0x34, 0x8, 0xb84bc8, 0xb84bca, 0xd650ae, 0xd650ae },
bank 4
         { 0x7f, 0x34, 0x8, 0x1962d6e, 0x1962d70, 0x1b5e1b8, 0x1b5e1b8 },
bank 2
         { 0x7f, 0x34, 0x8, 0x1366db2, 0x1366db4, 0x1557c14, 0x1557c14 },
bank 6
         { 0x7f, 0x34, 0x8, 0x10ce77e, 0x10ce780, 0x130457c, 0x130457c },
bank 0
         { 0x7f, 0x34, 0x8, 0x116aaf6, 0x116aaf8, 0x137b370, 0x137b370 },
bank 1
         { 0x7f, 0x34, 0x8, 0x1000c9a, 0x1000c9c, 0x11fd198, 0x11fd198 },
bank 6
         { 0x7f, 0x34, 0x8, 0x15dfa70, 0x15dfa72, 0x17f6168, 0x17f6168 },
bank 3
         { 0x7f, 0x34, 0x8, 0x1448a92, 0x1448a94, 0x16398f4, 0x16398f4 },
bank 2
         { 0x7f, 0x34, 0x8, 0x18dea64, 0x18dea66, 0x1ae04ee, 0x1ae04ee },
bank 0
         { 0x7f, 0x34, 0x8, 0x17ad4e4, 0x17ad4e6, 0x19a2ae2, 0x19a2ae2 },
bank 0
         { 0x7f, 0x34, 0x8, 0x1d35164, 0x1d35166, 0x1f84962, 0x1f84962 },
bank 3
         { 0x7f, 0x34, 0x8, 0x18efdfc, 0x18efdfe, 0x1b1507a, 0x1b1507a },
bank 1
         { 0x7f, 0x34, 0x8, 0x15c987c, 0x15c987e, 0x17ecc7a, 0x17ecc7a },
bank 4
         { 0x7f, 0x34, 0x8, 0x1dc5706, 0x1dc5708, 0x1ff1e76, 0x1ff1e76 },
bank 2
         { 0x7f, 0x34, 0x8, 0x1dde068, 0x1dde06a, 0x1ff9866, 0x1ff9866 },
bank 5
         { 0x7f, 0x34, 0x8, 0x1801318, 0x180131a, 0x1a36b16, 0x1a36b16 },
bank 1
         { 0x7f, 0x34, 0x8, 0x1b7daaa, 0x1b7daac, 0x1dd2228, 0x1dd2228 },
bank 3
         { 0x7f, 0x34, 0x8, 0x1dc0590, 0x1dc0592, 0x1ff558e, 0x1ff558e },
bank 5
         { 0x7f, 0x34, 0x8, 0x1db366a, 0x1db366c, 0x1fe2d00, 0x1fe2d00 },
bank 3
         { 0x7f, 0x34, 0x8, 0xb11e0c, 0xb11e0e, 0xd169be, 0xd169be },
bank 3
         { 0x7f, 0x34, 0x8, 0xf9f908, 0xf9f90a, 0x11967e2, 0x11967e2 },
bank 2
         { 0x7f, 0x34, 0x8, 0xb52928, 0xb5292a, 0xd59560, 0xd59560 },
bank 5
         { 0x7f, 0x34, 0x8, 0xcb0810, 0xcb0812, 0xe7c9b4, 0xe7c9b4 },
bank 2
         { 0x7f, 0x34, 0x8, 0xf8c900, 0xf8c902, 0x11573d6, 0x11573d6 },
bank 1
         { 0x7f, 0x34, 0x8, 0xa2bc86, 0xa2bc88, 0xc2de00, 0xc2de00 },
bank 5
         { 0x7f, 0x34, 0x8, 0x123ac5c, 0x123ac5e, 0x143d696, 0x143d696 },
bank 0
         { 0x7f, 0x34, 0x8, 0xbb3b5e, 0xbb3b60, 0xdb7026, 0xdb7026 },
bank 4
         { 0x7f, 0x34, 0x8, 0xac1b18, 0xac1b1a, 0xc9344e, 0xc9344e },
bank 4
         { 0x7f, 0x34, 0x8, 0x1009400, 0x1009402, 0x11f3426, 0x11f3426 },
bank 0
         { 0x7f, 0x34, 0x8, 0x58258e, 0x582590, 0x593bbe, 0x593bbe },
bank 0
         { 0x7f, 0x34, 0x8, 0x59a7b4, 0x59a7b6, 0x69cafa, 0x69cafa },
bank 0
         { 0x7f, 0x34, 0x8, 0x6bb224, 0x6bb226, 0x6d5c5a, 0x6d5c5a },
bank 0
         { 0x7f, 0x34, 0x8, 0x6e2708, 0x6e270a, 0x7242ca, 0x7242ca },
bank 0
         { 0x7f, 0x34, 0x8, 0x72e838, 0x72e83a, 0x744028, 0x744028 },
bank 0
         { 0x7f, 0x34, 0x8, 0x74402a, 0x74402c, 0x7cdbf2, 0x7cdbf2 },
	#endif

	// rock n' tread 3
	#if 0
bank 0
         { 0x7f, 0x0, 0x8, 0x1dcd382, 0x1dcd384, 0x1f9cfb8, 0x1f9cfb8 },
bank 6
         { 0x7f, 0x34, 0x8, 0x1214d4e, 0x1214d50, 0x13ea9fc, 0x13ea9fc },
bank 0
         { 0x7f, 0x34, 0x8, 0x11d0584, 0x11d0586, 0x13e3784, 0x13e3784 },
bank 5
         { 0x7f, 0x34, 0x8, 0x10f98e0, 0x10f98e2, 0x130fec4, 0x130fec4 },
bank 6
         { 0x7f, 0x34, 0x8, 0xd7f40a, 0xd7f40c, 0xf74132, 0xf74132 },
bank 5
         { 0x7f, 0x34, 0x8, 0xbb3b0c, 0xbb3b0e, 0xdcdc6a, 0xdcdc6a },
bank 2
         { 0x7f, 0x34, 0x8, 0x18b8b28, 0x18b8b2a, 0x1ab9f22, 0x1ab9f22 },
bank 4
         { 0x7f, 0x34, 0x8, 0xb785f2, 0xb785f4, 0xd63fea, 0xd63fea },
bank 5
         { 0x7f, 0x34, 0x8, 0x1533302, 0x1533304, 0x1754394, 0x1754394 },
bank 5
         { 0x7f, 0x34, 0x8, 0x1c51c2c, 0x1c51c2e, 0x1e80f92, 0x1e80f92 },
bank 3
         { 0x7f, 0x34, 0x8, 0x1d744a2, 0x1d744a4, 0x1fd49a2, 0x1fd49a2 },
bank 1
         { 0x7f, 0x34, 0x8, 0x1895554, 0x1895556, 0x1ace2fe, 0x1ace2fe },
bank 4
         { 0x7f, 0x34, 0x8, 0x12084b6, 0x12084b8, 0x1474734, 0x1474734 },
bank 6
         { 0x7f, 0x34, 0x8, 0x15cc93e, 0x15cc940, 0x17d351e, 0x17d351e },
bank 2
         { 0x7f, 0x34, 0x8, 0x105c830, 0x105c832, 0x125e5e6, 0x125e5e6 },
bank 1
         { 0x7f, 0x34, 0x8, 0x1dbe7c0, 0x1dbe7c2, 0x1fd4eb8, 0x1fd4eb8 },
bank 0
         { 0x7f, 0x34, 0x8, 0x17abbee, 0x17abbf0, 0x19fb3ec, 0x19fb3ec },
bank 3
         { 0x7f, 0x34, 0x8, 0xc2e710, 0xc2e712, 0xe1dcba, 0xe1dcba },
bank 0
         { 0x7f, 0x34, 0x8, 0xc6193e, 0xc61940, 0xe68576, 0xe68576 },
bank 2
         { 0x7f, 0x34, 0x8, 0x1e22e3e, 0x1e22e40, 0x1fc7b76, 0x1fc7b76 },
bank 4
         { 0x7f, 0x34, 0x8, 0x1d7aeea, 0x1d7aeec, 0x1fcf668, 0x1fcf668 },
bank 1
         { 0x7f, 0x34, 0x8, 0x1395814, 0x1395816, 0x15ca812, 0x15ca812 },
bank 6
         { 0x7f, 0x34, 0x8, 0x972824, 0x972826, 0xb7499e, 0xb7499e },
bank 4
         { 0x7f, 0x34, 0x8, 0x1809bd4, 0x1809bd6, 0x19ea0ba, 0x19ea0ba },
bank 1
         { 0x7f, 0x34, 0x8, 0xebdbb4, 0xebdbb6, 0x10ea324, 0x10ea324 },
bank 2
         { 0x7f, 0x34, 0x8, 0x146dfe8, 0x146dfea, 0x165ee4a, 0x165ee4a },
bank 1
         { 0x7f, 0x34, 0x8, 0xa4cb14, 0xa4cb16, 0xc56666, 0xc56666 },
bank 2
         { 0x7f, 0x34, 0x8, 0xb147e8, 0xb147ea, 0xd3eea0, 0xd3eea0 },
bank 3
         { 0x7f, 0x34, 0x8, 0x116cfe6, 0x116cfe8, 0x139d7e4, 0x139d7e4 },
bank 3
         { 0x7f, 0x34, 0x8, 0x172e00c, 0x172e00e, 0x1989a9c, 0x1989a9c },
bank 0
         { 0x7f, 0x34, 0x8, 0x50c7d0, 0x50c7d2, 0x51de00, 0x51de00 },
bank 0
         { 0x7f, 0x34, 0x8, 0x51de02, 0x51de04, 0x5977be, 0x5977be },
bank 0
         { 0x7f, 0x34, 0x8, 0x5977c0, 0x5977c2, 0x5b21f6, 0x5b21f6 },
bank 0
         { 0x7f, 0x34, 0x8, 0x66ce2c, 0x66ce2e, 0x6a6e3e, 0x6a6e3e },
bank 0
         { 0x7f, 0x34, 0x8, 0x5b21f8, 0x5b21fa, 0x5c7aee, 0x5c7aee },
bank 0
         { 0x7f, 0x34, 0x8, 0x5c7af0, 0x5c7af2, 0x666236, 0x666236 },
	#endif

	// rock n' tread 4
	#if 0
bank 0
         { 0x7f, 0x0, 0x8, 0x8b4106, 0x8b4108, 0xac0bb0, 0xac0bb0 },
bank 0
         { 0x7f, 0x34, 0x8, 0xac0bb2, 0xac0bb4, 0xca3d2c, 0xca3d2c },
bank 0
         { 0x7f, 0x34, 0x8, 0xca3d2e, 0xca3d30, 0xec6ca8, 0xec6ca8 },
bank 0
         { 0x7f, 0x34, 0x8, 0xec6caa, 0xec6cac, 0x10f1af6, 0x10f1af6 },
bank 0
         { 0x7f, 0x34, 0x8, 0x10f1af8, 0x10f1afa, 0x12b75ca, 0x12b75ca },
bank 0
         { 0x7f, 0x34, 0x8, 0x12b75cc, 0x12b75ce, 0x14a2786, 0x14a2786 },
bank 0
         { 0x7f, 0x34, 0x8, 0x14a2788, 0x14a278a, 0x1674c0e, 0x1674c0e },
bank 0
         { 0x7f, 0x34, 0x8, 0x1674c10, 0x1674c12, 0x184803e, 0x184803e },
bank 0
         { 0x7f, 0x34, 0x8, 0x1848040, 0x1848042, 0x1a4ec50, 0x1a4ec50 },
bank 0
         { 0x7f, 0x34, 0x8, 0x1a4ec52, 0x1a4ec54, 0x1c9eeec, 0x1c9eeec },
bank 1
         { 0x7f, 0x34, 0x8, 0x800000, 0x800002, 0xa26908, 0xa26908 },
bank 0
         { 0x7f, 0x34, 0x8, 0x1c9eeee, 0x1c9eef0, 0x1ee9370, 0x1ee9370 },
bank 1
         { 0x7f, 0x34, 0x8, 0xa2690a, 0xa2690c, 0xc608fa, 0xc608fa },
bank 1
         { 0x7f, 0x34, 0x8, 0xc608fc, 0xc608fe, 0xeeb346, 0xeeb346 },
bank 2
         { 0x7f, 0x34, 0x8, 0x800000, 0x800002, 0xa555f6, 0xa555f6 },
bank 2
         { 0x7f, 0x34, 0x8, 0xa555f8, 0xa555fa, 0xc83836, 0xc83836 },
bank 1
         { 0x7f, 0x34, 0x8, 0xeeb348, 0xeeb34a, 0x1123508, 0x1123508 },
bank 2
         { 0x7f, 0x34, 0x8, 0xc83838, 0xc8383a, 0xf1af80, 0xf1af80 },
bank 1
         { 0x7f, 0x34, 0x8, 0x112350a, 0x112350c, 0x1340260, 0x1340260 },
bank 1
         { 0x7f, 0x34, 0x8, 0x1340262, 0x1340264, 0x15327d2, 0x15327d2 },
bank 1
         { 0x7f, 0x34, 0x8, 0x15327d4, 0x15327d6, 0x173481e, 0x173481e },
bank 2
         { 0x7f, 0x34, 0x8, 0xf1af82, 0xf1af84, 0x11ade28, 0x11ade28 },
bank 1
         { 0x7f, 0x34, 0x8, 0x1734820, 0x1734822, 0x195324e, 0x195324e },
bank 1
         { 0x7f, 0x34, 0x8, 0x1953250, 0x1953252, 0x1bd923e, 0x1bd923e },
bank 2
         { 0x7f, 0x34, 0x8, 0x11ade2a, 0x11ade2c, 0x14413f2, 0x14413f2 },
bank 1
         { 0x7f, 0x34, 0x8, 0x1bd9240, 0x1bd9242, 0x1df3786, 0x1df3786 },
bank 1
         { 0x7f, 0x34, 0x8, 0x1df3788, 0x1df378a, 0x1fee746, 0x1fee746 },
bank 2
         { 0x7f, 0x34, 0x8, 0x14413f4, 0x14413f6, 0x169a2c6, 0x169a2c6 },
bank 2
         { 0x7f, 0x34, 0x8, 0x169a2c8, 0x169a2ca, 0x1908d20, 0x1908d20 },
bank 2
         { 0x7f, 0x34, 0x8, 0x1908d22, 0x1908d24, 0x1b16d88, 0x1b16d88 },
bank 0
         { 0x7f, 0x34, 0x8, 0x548aa, 0x548ac, 0x6bf64, 0x6bf64 },
bank 0
         { 0x7f, 0x34, 0x8, 0x6edbc, 0x6edbe, 0x112dd6, 0x112dd6 },
bank 0
         { 0x7f, 0x34, 0x8, 0x131fec, 0x131fee, 0x17e0ba, 0x17e0ba },
bank 0
         { 0x7f, 0x34, 0x8, 0x18ab68, 0x18ab6a, 0x1dbea2, 0x1dbea2 },
bank 0
         { 0x7f, 0x34, 0x8, 0x1e6410, 0x1e6412, 0x21b424, 0x21b424 },
bank 0
         { 0x7f, 0x34, 0x8, 0x21b426, 0x21b428, 0x238ec2, 0x238ec2 },
	#endif
};

static unsigned int sk_readmem(unsigned int address)
{
	return prgrom[address];
}

static void sk_writemem(unsigned int address, unsigned int data)
{
}

void play_sample(int lflag, int cmda)
{
	YMZ280B_register_0_w(0, 0);
	YMZ280B_data_0_w(0, commands[cmda].fnum);
	YMZ280B_register_0_w(0, 1);
	YMZ280B_data_0_w(0, 0x20);
	YMZ280B_register_0_w(0, 2);
	YMZ280B_data_0_w(0, commands[cmda].level);
	YMZ280B_register_0_w(0, 3);
	YMZ280B_data_0_w(0, commands[cmda].pan);
	YMZ280B_register_0_w(0, 0x20);
	YMZ280B_data_0_w(0, commands[cmda].start>>17);
	YMZ280B_register_0_w(0, 0x40);
	YMZ280B_data_0_w(0, (commands[cmda].start&0x1fe00)>>9);
	YMZ280B_register_0_w(0, 0x60);
	YMZ280B_data_0_w(0, (commands[cmda].start&0x1fe)>>1);
	YMZ280B_register_0_w(0, 0x21);
	YMZ280B_data_0_w(0, commands[cmda].loopstart>>17);
	YMZ280B_register_0_w(0, 0x41);
	YMZ280B_data_0_w(0, (commands[cmda].loopstart&0x1fe00)>>9);
	YMZ280B_register_0_w(0, 0x61);
	YMZ280B_data_0_w(0, (commands[cmda].loopstart&0x1fe)>>1);
	YMZ280B_register_0_w(0, 0x22);
	YMZ280B_data_0_w(0, commands[cmda].loopstop>>17);
	YMZ280B_register_0_w(0, 0x42);
	YMZ280B_data_0_w(0, (commands[cmda].loopstop&0x1fe00)>>9);
	YMZ280B_register_0_w(0, 0x62);
	YMZ280B_data_0_w(0, (commands[cmda].loopstop&0x1fe)>>1);
	YMZ280B_register_0_w(0, 0x23);
	YMZ280B_data_0_w(0, commands[cmda].stop>>17);
	YMZ280B_register_0_w(0, 0x43);
	YMZ280B_data_0_w(0, (commands[cmda].stop&0x1fe00)>>9);
	YMZ280B_register_0_w(0, 0x63);
	YMZ280B_data_0_w(0, (commands[cmda].stop&0x1fe)>>1);
	YMZ280B_register_0_w(0, 1);
	if (lflag)
	{
		YMZ280B_data_0_w(0, (commands[cmda].fnum>>9) | 0xb0);	// on, looping
	}
	else
	{
		YMZ280B_data_0_w(0, (commands[cmda].fnum>>9) | 0xa0);	// on, no looping
	}
}

static void skns_irq(int state)
{
	int foo;
	
	// reading status clears the IRQ state
	foo = YMZ280B_status_0_r(0);

	if (state)
	{
		if (current >= 0)
		{
			if (cursong[current].flags == FL_SLOOP)
			{
				return;	// ymz will handle the looping itself
			}
		}

		// sample's come to an end, do the next
		YMZ280B_register_0_w(0, 1);
		YMZ280B_data_0_w(0, 0x20);

		current++;
		switch (cursong[current].flags)
		{
			case FL_LSTART:
				lp_start = current;
				break;

			case FL_LOOP:
				current = lp_start;
				break;

			case FL_END:
				return;	// don't play anything else, this is it
				break;
		}

		if (cursong[current].flags == FL_SLOOP)
		{
			play_sample(1, cursong[current].sample);
		}
		else
		{
			play_sample(0, cursong[current].sample);
		}
	}
}
 
static void SKNS_Init(long srate)
{
	prgrom = rom_getregion(RGN_CPU1);

	// m1's core can't work without at least 1 CPU, so
	// construct a phony 6502 program that does nothing.
	// point all vectors to 0x2000
	prgrom[0xfff0] = 0x00;
	prgrom[0xfff1] = 0x20;
	prgrom[0xfff2] = 0x00;
	prgrom[0xfff3] = 0x20;
	prgrom[0xfff4] = 0x00;
	prgrom[0xfff5] = 0x20;
	prgrom[0xfff6] = 0x00;
	prgrom[0xfff7] = 0x20;
	prgrom[0xfff8] = 0x00;
	prgrom[0xfff9] = 0x20;
	prgrom[0xfffa] = 0x00;
	prgrom[0xfffb] = 0x20;
	prgrom[0xfffc] = 0x00;
	prgrom[0xfffd] = 0x20;
	prgrom[0xfffe] = 0x00;
	prgrom[0xffff] = 0x20;

	prgrom[0x2000] = 0x4c;	// JMP 2000
	prgrom[0x2001] = 0x00;
	prgrom[0x2002] = 0x20;

	m1snd_add6502(100000, &dcrw);	// 0.1 MHz, why waste time on it :)

	switch (Machine->refcon)
	{
		case 1: //GAME_CYVERN:
			topsong = 0x14;
			break;

		case 2: //GAME_GUWANGE:
			topsong = 13;
			break;

		case 3: //GAME_TETRISP2:
			topsong = 24;
			break;

		case 4: //GAME_SENGEKISTRIKER:
			topsong = 11;
			break;

		case 5: //GAME_UOPOKO:
			topsong = 14;
			break;

		case 6: //GAME_ESPRADE:
			topsong = 13;
			break;

		case 7: //GAME_DFEVERON:
			topsong = 14;
			break;

		#if ENABLE_ROCKN
		case 8: //GAME_ROCKN1:
			topsong = 29;
			break;
		#endif

		case 9:
			topsong = 2;
			break;
	}
}

static void SKNS_SendCmd(int cmda, int cmdb)
{
	// enable all irqs
	YMZ280B_register_0_w(0, 0xfe);
	YMZ280B_data_0_w(0, 0xff);

	// hit the key enable
	YMZ280B_register_0_w(0, 0xff);
	YMZ280B_data_0_w(0, 0xd0);

	if (cmda == 0xffff)
	{
		YMZ280B_register_0_w(0, 1);
		YMZ280B_data_0_w(0, 0x20);
		return;
	}

	if (cmda >= topsong) cmda = topsong;

	// start a song, the irq handler will take it from here
	switch (Machine->refcon)
	{
		case 1: //GAME_CYVERN:
			cursong = cyvsongs[cmda];
			break;

		case 2: //GAME_GUWANGE:
			cursong = guwangesongs[cmda];
			break;

		case 3: //GAME_TETRISP2:
			cursong = tetrisp2songs[cmda];
			break;

		case 4: //GAME_SENGEKISTRIKER:
			cursong = sengekisongs[cmda];
			break;

		case 5: //GAME_UOPOKO:
			cursong = uopokosongs[cmda];
			break;

		case 6: //GAME_ESPRADE:
			cursong = espradesongs[cmda];
			break;

		case 7: //GAME_DFEVERON:
			cursong = feveronsongs[cmda];
			break;

		#if ENABLE_ROCKN
		case 8: //GAME_ROCKN1:
			{
				unsigned char *SNDROM = memory_region(RGN_SAMP1);
				int bank;

				cursong = rockn1songs[cmda];

				bank = (size_t)cursong & 0x00000f00;
				bank >>= 8;

				memcpy(&SNDROM[0x0400000], &SNDROM[0x1000000 + (0x0c00000 * bank)], 0x0c00000);
			}
			break;
		#endif

		case 9:
			cursong = testsongs[cmda];
			break;
	}
	lp_start = 0;
	current = -1;

	// check if it's a construct-on-the-fly
	// this isn't foolproof, but it's unlikely on any architecture
	// that we'll load into the lowest 64k of RAM.
	if (((size_t)cursong & 0xffff0000) == 0x00000000)
	{
		// loop entire sample, ignore YMZ loop points
		if (((size_t)cursong & 0xf000) == SP_SINGLE_LOOP)
		{
			dummysong[0].flags = FL_LSTART;
			dummysong[0].sample = ((size_t)cursong & 0xff);
			dummysong[1].flags = FL_LOOP;
			dummysong[1].sample = -1;
		}

		// single loop using YMZ loop points
		if (((size_t)cursong & 0xf000) == SP_SINGLE_SLOOP)
		{
			dummysong[0].flags = FL_SLOOP;
			dummysong[0].sample = ((size_t)cursong & 0xff);
			dummysong[1].flags = FL_END;
			dummysong[1].sample = -1;
		}

		// single shot
		if (((size_t)cursong & 0xf000) == SP_SINGLE_SHOT)
		{
			dummysong[0].flags = 0;
			dummysong[0].sample = ((size_t)cursong & 0xff);
			dummysong[1].flags = FL_END;
			dummysong[1].sample = -1;
		}

		cursong = &dummysong[0];
	}

	// kick off the song
	skns_irq(1);
}