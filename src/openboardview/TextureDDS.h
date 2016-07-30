#ifdef _WIN32
#include <d3d9.h>
typedef unsigned long u32;
#else
#include <glad/glad.h>
typedef unsigned int u32;
#endif

struct DDSPixelFormat {
	u32 size;
	u32 flags;
	u32 fourCC;
	u32 rgbBitCount;
	u32 rBitMask;
	u32 gBitMask;
	u32 bBitMask;
	u32 aBitMask;
};

struct DDSHeader {
	u32 magic;
	u32 size;
	u32 flags;
	u32 height;
	u32 width;
	u32 pitchOrLinearSize;
	u32 depth;
	u32 mipMapCount;
	u32 reserved1[11];
	DDSPixelFormat pf;
	u32 caps;
	u32 caps2;
	u32 caps3;
	u32 caps4;
	u32 reserved2;
};

#define FOURCC_DXT1 0x31545844 // Equivalent to "DXT1" in ASCII
#define FOURCC_DXT3 0x33545844 // Equivalent to "DXT3" in ASCII
#define FOURCC_DXT5 0x35545844 // Equivalent to "DXT5" in ASCII

class TextureDDS {
  public:
	TextureDDS(unsigned char *data);
#ifdef _WIN32
	bool dx9Load(LPDIRECT3DDEVICE9 g_pd3dDevice);
#else
	bool glLoad();
#endif
	void *get();

  private:
	unsigned char *data;
	u32 width;
	u32 height;
	DDSHeader *header;
#ifdef _WIN32
	LPDIRECT3DTEXTURE9 tex = NULL;
	D3DLOCKED_RECT tex_locked_rect;
#else
	GLuint textureID;
#endif
	void *texture;
	bool load();
};
