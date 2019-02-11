#include "xxhash.h"
#include <stdio.h>
#include <stdint.h>
#include <endian.h>
#include <malloc.h>
#include <inttypes.h>

#undef bswap16
#undef bswap32

static inline int16_t toLittle16(int16_t val) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#if __GNUC__
  return __builtin_bswap16(val);
#elif _WIN32
  return _byteswap_ushort(val);
#else
  return (val = (val << 8) | ((val >> 8) & 0xFF));
#endif
#else
  return val;
#endif
}

static inline int32_t toLittle32(int32_t val) {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#if __GNUC__
  return __builtin_bswap32(val);
#elif _WIN32
  return _byteswap_ulong(val);
#else
  val = (val & 0x0000FFFF) << 16 | (val & 0xFFFF0000) >> 16;
  val = (val & 0x00FF00FF) << 8 | (val & 0xFF00FF00) >> 8;
#endif
#endif
  return val;
}

typedef enum
{
    TF_I4,
    TF_I8,
    TF_IA4,
    TF_IA8,
    TF_IDX4,
    TF_IDX8,
    TF_RGB564,
    TF_RGB565,
    TF_RGB5A3,
    TF_RGBA8,
    TF_DXT1
} TXTRFormat;

typedef enum
{
    DTF_I4 = 0,
    DTF_I8 = 1,
    DTF_IA4 = 2,
    DTF_IA8 = 3,
    DTF_RGB565 = 4,
    DTF_RGB5A3 = 5,
    DTF_RGBA8 = 6,
    DTF_C4 = 8,
    DTF_C8 = 9,
    DTF_C14X2 = 10,
    DTF_CMPR = 14,
    DTF_INVALID = -1
} DOLTXTRFormat;

typedef struct
{
    TXTRFormat format;
    uint16_t width;
    uint16_t height;
    uint32_t mipmapCount;
} TXTRHeader;

DOLTXTRFormat retroToDol(TXTRFormat fmt)
{
    switch(fmt)
    {
    case TF_I4:  return DTF_I4;
    case TF_I8:  return DTF_I8;
    case TF_IA4: return DTF_IA4;
    case TF_IA8: return DTF_IA8;
    case TF_IDX4: return DTF_C4;
    case TF_IDX8: return DTF_C8;
    case TF_RGB565: return DTF_RGB565;
    case TF_RGB5A3: return DTF_RGB5A3;
    case TF_RGBA8: return DTF_RGBA8;
    case TF_DXT1: return DTF_CMPR;
    default:
        break;
    }
    return DTF_INVALID;
}

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        printf("Metroid Prime Dolphin Texture Hash Generator\n");
        printf("Usage: txtrhashgen <texture>\n");
        return 1;
    }
    FILE* f = fopen(argv[1], "rb");

    if (!f)
    {
        printf("Unable to locate file %s\n", argv[1]);
        return 1;
    }

    TXTRHeader header;
    fread(&header, 1, sizeof(TXTRHeader), f);
    header.format = (TXTRFormat)toLittle32(header.format);
    header.width = toLittle16(header.width);
    header.height = toLittle16(header.height);
    header.mipmapCount = toLittle32(header.mipmapCount);
    uint32_t textureSize = 0;
    DOLTXTRFormat dtFormat = retroToDol(header.format);
    if (dtFormat == DTF_INVALID)
    {
        fclose(f);
        printf("Unsupported format %d\n", header.format);
        return 1;
    }

    switch(header.format)
    {
    case TF_IDX8:
    case TF_IDX4:
        fclose(f);
        printf("Palletized textures are currently unsupported\n");
        return 1;
    case TF_I4:
    case TF_DXT1:
        textureSize = header.width * header.height / 2;
        break;
    case TF_RGB565:
    case TF_RGB5A3:
    case TF_IA8:
        textureSize = header.width * header.height * 2;
        break;
    case TF_RGBA8:
        textureSize = header.width * header.height * 4;
        break;
    default:
        textureSize = header.width * header.height;
        break;
    }

    void* data = malloc(textureSize);
    fread(data, 1, textureSize, f);
    fclose(f);

    uint64_t hash = XXH64(data, textureSize, 0);

    printf("tex1_%dx%d%s_%016" PRIx64 "_%d\n", header.width, header.height, header.mipmapCount > 1 ? "_m" : "", hash, dtFormat);
    return 0;
}
