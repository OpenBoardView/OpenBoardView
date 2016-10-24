#include "TextureDDS.h"
#include <assert.h>

TextureDDS::TextureDDS(unsigned char *data) {
	this->data   = data;
	this->header = (DDSHeader *)data;
	assert(header->pf.fourCC == FOURCC_DXT5);
	this->width  = header->width;
	this->height = header->height;
}

bool TextureDDS::load() {
	const u32 bytes_per_block = 16;
	data                      = data + sizeof(DDSHeader);
	for (u32 i = 0; i < header->mipMapCount; i++) {
		u32 num_blocks = ((width + 3) >> 2) * ((height + 3) >> 2);
		u32 num_bytes  = num_blocks * bytes_per_block;
#ifdef _WIN32
		if (tex->LockRect(i, &tex_locked_rect, NULL, 0) != D3D_OK) return false;
		memcpy(tex_locked_rect.pBits, data, num_bytes);
		tex->UnlockRect(i);
#else
		glCompressedTexImage2D(GL_TEXTURE_2D, i, GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, width, height, 0, num_bytes, data);
#endif
		data += num_bytes;
		width >>= 1;
		height >>= 1;
		if (!width) width   = 1;
		if (!height) height = 1;
	}
	return true;
}

#ifdef _WIN32
bool TextureDDS::dx9Load(LPDIRECT3DDEVICE9 g_pd3dDevice) {
	if (g_pd3dDevice->CreateTexture(
	        width, height, header->mipMapCount, D3DUSAGE_DYNAMIC, D3DFMT_DXT5, D3DPOOL_DEFAULT, &tex, NULL) < 0)
		return false;
	if (!this->load()) return false;
	texture = (void *)tex;
	return true;
}
#else
bool TextureDDS::glLoad() {
	// Create one OpenGL texture
	glGenTextures(1, &textureID);
	// "Bind" the newly created texture : all future texture functions will modify
	// this texture
	glBindTexture(GL_TEXTURE_2D, textureID);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	if (!this->load()) return false;

	texture = (void *)textureID;
	// Unbind the texture
	glBindTexture(GL_TEXTURE_2D, 0);
	return true;
}
#endif

void *TextureDDS::get() {
	return texture;
}
