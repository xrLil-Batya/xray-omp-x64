// file: targasaver.cpp
#include "stdafx.h"
#pragma hdrstop

#include "tga.h"
/*
void	tga_save	(LPCSTR name, u32 w, u32 h, void* data, BOOL alpha )
{
	// Save
	TGAdesc		tga;
	tga.data	= data;
	tga.format	= alpha?IMG_32B:IMG_24B;
	tga.height	= h;
	tga.width	= w;
	tga.scanlenght=w*4;

	int		hf	= _open(name,O_CREAT | O_WRONLY | O_BINARY, S_IREAD | S_IWRITE);
	tga.maketga	(hf);
	_close		(hf);
}
*/

void TGAdesc::maketga( IWriter& fs ){
	R_ASSERT(data);
	R_ASSERT(width);
	R_ASSERT(height);

	tgaHeader hdr;
	ZeroMemory( &hdr, sizeof(hdr) );
	hdr.tgaImgType			= 2;
	hdr.tgaImgSpec.tgaXSize = u16(width);
	hdr.tgaImgSpec.tgaYSize = u16(height);

	if( format == IMG_24B ){
		hdr.tgaImgSpec.tgaDepth = 24;
		hdr.tgaImgSpec.tgaImgDesc = 32;			// flip
	}
	else{
		hdr.tgaImgSpec.tgaDepth = 32;
		hdr.tgaImgSpec.tgaImgDesc = 0x0f | 32;	// flip
	}

	fs.w(&hdr, sizeof(hdr) );

	if( format==IMG_24B ){
		BYTE ab_buffer[4]={0,0,0,0};
		int  real_sl = ((width*3)) & 3;
		int  ab_size = real_sl ? 4-real_sl : 0 ;
		for( int j=0; j<height; j++){
			BYTE *p = (LPBYTE)data + scanlenght*j;
			for( int i=0; i<width; i++){
				BYTE buffer[3] = {p[0],p[1],p[2]};
				fs.w(buffer, 3 );
				p+=4;
			}
			if(ab_size)
				fs.w(ab_buffer, ab_size );
		}
	}
	else{
		if (width*4 == scanlenght)	fs.w	(data,width*height*4);
		else {
			// bad pitch, it seems :(
			for( int j=0; j<height; j++){
				BYTE *p = (LPBYTE)data + scanlenght*j;
				for( int i=0; i<width; i++){
					BYTE buffer[4] = {p[0],p[1],p[2],p[3]};
					fs.w(buffer, 4 );
					p+=4;
				}
			}
		}
	}
}

/*
void TGAdesc::maketga( int hf ){
	R_ASSERT(data);
	R_ASSERT(width);
	R_ASSERT(height);

	tgaHeader hdr;
	ZeroMemory( &hdr, sizeof(hdr) );
	hdr.tgaImgType			= 2;
	hdr.tgaImgSpec.tgaXSize = u16(width);
	hdr.tgaImgSpec.tgaYSize = u16(height);

	if( format == IMG_24B ){
		hdr.tgaImgSpec.tgaDepth = 24;
		hdr.tgaImgSpec.tgaImgDesc = 32;			// flip
	}
	else{
		hdr.tgaImgSpec.tgaDepth = 32;
		hdr.tgaImgSpec.tgaImgDesc = 0x0f | 32;	// flip
	}

	_write(hf, &hdr, sizeof(hdr) );

	if( format==IMG_24B ){
		BYTE ab_buffer[4]={0,0,0,0};
		int  real_sl = ((width*3)) & 3;
		int  ab_size = real_sl ? 4-real_sl : 0 ;
		for( int j=0; j<height; j++){
			BYTE *p = (LPBYTE)data + scanlenght*j;
			for( int i=0; i<width; i++){
				BYTE buffer[3] = {p[0],p[1],p[2]};
				_write(hf, buffer, 3 );
				p+=4;
			}
			if(ab_size)
				_write(hf, ab_buffer, ab_size );
		}
	}
	else{
		_write	(hf,data,width*height*4);
	}
}
*/
#include "stdafx.h"

#include <cstdio>

using namespace XRay::Media;

Image::Image()
{
	format = ImageFormat::Unknown;
	channelCount = 0;
	width = height = 0;
	data = nullptr;
}

Image::~Image() {}

Image& Image::Create(u16 width, u16 height, void* data, ImageFormat format)
{
	this->width = width;
	this->height = height;
	this->data = data;
	this->format = format;
	channelCount = format == ImageFormat::RGB8 ? 3 : 4;
	return *this;
}

void Image::SaveTGA(const char* name, ImageFormat format, bool align)
{
	FILE* file = std::fopen(name, "wb");
	auto writerFunc = [&](void* data, size_t dataSize) { std::fwrite(data, dataSize, 1, file); };
	SaveTGA(writerFunc, format, align);
	std::fclose(file);
}

void Image::SaveTGA(IWriter& writer, bool align) { SaveTGA(writer, format, align); }
void Image::SaveTGA(IWriter& writer, ImageFormat format, bool align)
{
	auto writerFunc = [&](void* data, size_t dataSize) { writer.w(data, dataSize); };
	SaveTGA(writerFunc, format, align);
}

void Image::SaveTGA(const char* name, bool align) { SaveTGA(name, format, align); }
template <typename TWriter>
void Image::SaveTGA(TWriter& writerFunc, ImageFormat format, bool align)
{
	R_ASSERT(data);
	R_ASSERT(width);
	R_ASSERT(height);
	TGAHeader hdr = {};
	hdr.ImageType = 2; // uncompressed true-color image
	hdr.Width = width;
	hdr.Height = height;
	int scanLength = width * channelCount;
	switch (format)
	{
	case ImageFormat::RGB8:
	{
		hdr.BPP = 24;
		// XXX: generally should be set to zero
		hdr.ImageDesc = 32;
		writerFunc(&hdr, sizeof(hdr));
		int paddingBuf = 0;
		int paddingSize = align ? 4 - (width * 3 & 3) : 0;
		for (int j = 0; j < height; j++)
		{
			u8* p = (u8*)data + scanLength * j;
			for (int i = 0; i < width; i++)
			{
				u8 buffer[3] = { p[0], p[1], p[2] };
				writerFunc(buffer, sizeof(buffer));
				p += channelCount;
			}
			if (paddingSize)
				writerFunc(&paddingBuf, paddingSize);
		}
		break;
	}
	case ImageFormat::RGBA8:
	{
		hdr.BPP = 32;
		hdr.ImageDesc = 0x0f | 32;
		writerFunc(&hdr, sizeof(hdr));
		if (this->format == format)
			writerFunc(data, width * height * channelCount);
		else
		{
			for (int j = 0; j < height; j++)
			{
				u8* p = (u8*)data + scanLength * j;
				for (int i = 0; i < width; i++)
				{
					u8 buffer[4] = { p[0], p[1], p[2], 0xff };
					writerFunc(buffer, sizeof(buffer));
					p += channelCount;
				}
			}
		}
		break;
	}
	default: FATAL("Unsupported TGA image format");
	}
}