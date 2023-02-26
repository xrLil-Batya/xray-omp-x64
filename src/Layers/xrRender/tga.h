// file: targasaver.h

#ifndef _INCDEF_TARGASAVER_H_
#define _INCDEF_TARGASAVER_H_

#pragma pack(push,1)
struct tgaImgSpecHeader{
	u16		tgaXOrigin;
	u16		tgaYOrigin;
	u16		tgaXSize;
	u16		tgaYSize;
	BYTE	tgaDepth;
	BYTE	tgaImgDesc;
};
struct tgaHeader{
	BYTE	tgaIDL;
	BYTE	tgaMapType;
	BYTE	tgaImgType;
	BYTE	tgaClrMapSpec[5];
	tgaImgSpecHeader tgaImgSpec;
};
#pragma pack(pop)


#define IMG_24B 0
#define IMG_32B 1

class TGAdesc
{
public:
	int format;
	int scanlenght;
	int width,height;
	void *data;
public:
	TGAdesc()	{ data = 0; };
	~TGAdesc()	{};

	void maketga( IWriter &fs );
//	void maketga( int hf );
};

void	tga_save	(LPCSTR name, u32 w, u32 h, void* data, BOOL alpha );

#endif /*_INCDEF_TARGASAVER_H_*/

#include "xrCore/xrCore.h"
#include "xrCore/FS.h"

namespace XRay
{
	namespace Media
	{
		enum class ImageFormat : u32
		{
			Unknown = 0,
			RGB8 = 1,
			RGBA8 = 2,
		};

		class XRCORE_API Image
		{
		private:
#pragma pack(push, 1)
			struct TGAHeader
			{
				u8 DescSize;
				u8 MapType;
				u8 ImageType;
				u16 MapStart;
				u16 MapEntries;
				u8 MapBits;
				u16 XOffset;
				u16 YOffset;
				u16 Width;
				u16 Height;
				u8 BPP;
				u8 ImageDesc;
			};
#pragma pack(pop)

			ImageFormat format;
			int channelCount;
			u16 width, height;
			void* data;

		public:
			Image();
			~Image();

			Image& Create(u16 width, u16 height, void* data, ImageFormat format);
			void SaveTGA(IWriter& writer, bool align);
			void SaveTGA(IWriter& writer, ImageFormat format, bool align);
			void SaveTGA(const char* name, bool align);
			void SaveTGA(const char* name, ImageFormat format, bool align);

		private:
			template <typename TWriter>
			void SaveTGA(TWriter& writer, ImageFormat format, bool align);
		};
	}
}