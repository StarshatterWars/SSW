/*  Project Starshatter Wars
    Fractal Dev Studios
    Copyright (C) 2025-2026. All Rights Reserved.

    SUBSYSTEM:    nGenEx.lib
    FILE:         PCX.cpp
    AUTHOR:       Carlos Bott

    ORIGINAL AUTHOR AND STUDIO
    ==========================
    John DiCamillo / Destroyer Studios LLC

    OVERVIEW
    ========
    PCX image file loader
*/

#include "PCX.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Unreal logging:
#include "Logging/LogMacros.h"

// +--------------------------------------------------------------------+

#define MAX_SIZE (4*1024*1024)

enum { BYTEMODE, RUNMODE };

// +--------------------------------------------------------------------+

PcxImage::PcxImage()
{
    // PCX.h initializes members with defaults, but keep explicit safety:
    bitmap = nullptr;
    himap = nullptr;
    imagebytes = 0;
    width = 0;
    height = 0;
}

PcxImage::PcxImage(short w, short h, unsigned char* bits, unsigned char* colors)
{
    bitmap = nullptr;
    himap = nullptr;
    imagebytes = 0;
    width = 0;
    height = 0;

    hdr.manufacturer = 10;
    hdr.version = 5;
    hdr.encoding = 1;
    hdr.bits_per_pixel = 8;
    hdr.xmin = 0;
    hdr.xmax = (short)(w - 1);
    hdr.ymin = 0;
    hdr.ymax = (short)(h - 1);
    hdr.hres = 0x48;
    hdr.vres = 0x48;

    hdr.palette16[0] = (unsigned char)0x00;
    hdr.palette16[1] = (unsigned char)0x00;
    hdr.palette16[2] = (unsigned char)0x00;
    hdr.palette16[3] = (unsigned char)0x80;
    hdr.palette16[4] = (unsigned char)0x00;
    hdr.palette16[5] = (unsigned char)0x00;
    hdr.palette16[6] = (unsigned char)0x00;
    hdr.palette16[7] = (unsigned char)0x80;
    hdr.palette16[8] = (unsigned char)0x00;
    hdr.palette16[9] = (unsigned char)0x80;
    hdr.palette16[10] = (unsigned char)0x80;
    hdr.palette16[11] = (unsigned char)0x00;
    hdr.palette16[12] = (unsigned char)0x00;
    hdr.palette16[13] = (unsigned char)0x00;
    hdr.palette16[14] = (unsigned char)0x80;
    hdr.palette16[15] = (unsigned char)0x80;

    hdr.palette16[16] = (unsigned char)0x00;
    hdr.palette16[17] = (unsigned char)0x80;
    hdr.palette16[18] = (unsigned char)0x00;
    hdr.palette16[19] = (unsigned char)0x80;
    hdr.palette16[20] = (unsigned char)0x80;
    hdr.palette16[21] = (unsigned char)0xC0;
    hdr.palette16[22] = (unsigned char)0xC0;
    hdr.palette16[23] = (unsigned char)0xC0;
    hdr.palette16[24] = (unsigned char)0xC0;
    hdr.palette16[25] = (unsigned char)0xDC;
    hdr.palette16[26] = (unsigned char)0xC0;
    hdr.palette16[27] = (unsigned char)0xA6;
    hdr.palette16[28] = (unsigned char)0xCA;
    hdr.palette16[29] = (unsigned char)0xF0;
    hdr.palette16[30] = (unsigned char)0x33;
    hdr.palette16[31] = (unsigned char)0x2B;

    hdr.palette16[32] = (unsigned char)0x1F;
    hdr.palette16[33] = (unsigned char)0x2B;
    hdr.palette16[34] = (unsigned char)0x23;
    hdr.palette16[35] = (unsigned char)0x1B;
    hdr.palette16[36] = (unsigned char)0x5F;
    hdr.palette16[37] = (unsigned char)0x5F;
    hdr.palette16[38] = (unsigned char)0x5F;
    hdr.palette16[39] = (unsigned char)0x2F;
    hdr.palette16[40] = (unsigned char)0x2F;
    hdr.palette16[41] = (unsigned char)0x2F;
    hdr.palette16[42] = (unsigned char)0x27;
    hdr.palette16[43] = (unsigned char)0x27;
    hdr.palette16[44] = (unsigned char)0x27;
    hdr.palette16[45] = (unsigned char)0x1F;
    hdr.palette16[46] = (unsigned char)0x1F;
    hdr.palette16[47] = (unsigned char)0x1F;

    hdr.reserved = 0;
    hdr.color_planes = 1;
    hdr.bytes_per_line = w;
    hdr.palette_type = 1;

    for (unsigned int i = 0; i < 58; i++)
        hdr.filler[i] = 0;

    width = (unsigned short)w;
    height = (unsigned short)h;
    imagebytes = (unsigned long)width * (unsigned long)height;

    bitmap = new unsigned char[imagebytes];

    if (bitmap) {
        for (unsigned long i = 0; i < imagebytes; i++)
            bitmap[i] = bits[i];

        unsigned char* p = pal;
        for (int i = 0; i < 256; i++) {
            *p++ = *colors++;
            *p++ = *colors++;
            *p++ = *colors++;
            colors++; // skip alpha/unused byte
        }
    }
}

PcxImage::PcxImage(short w, short h, unsigned long* hibits)
{
    bitmap = nullptr;
    himap = nullptr;
    imagebytes = 0;
    width = 0;
    height = 0;

    hdr.manufacturer = 10;
    hdr.version = 5;
    hdr.encoding = 1;
    hdr.bits_per_pixel = 8;
    hdr.xmin = 0;
    hdr.xmax = (short)(w - 1);
    hdr.ymin = 0;
    hdr.ymax = (short)(h - 1);
    hdr.hres = 0x48;
    hdr.vres = 0x48;

    hdr.palette16[0] = (unsigned char)0x00;
    hdr.palette16[1] = (unsigned char)0x00;
    hdr.palette16[2] = (unsigned char)0x00;
    hdr.palette16[3] = (unsigned char)0x80;
    hdr.palette16[4] = (unsigned char)0x00;
    hdr.palette16[5] = (unsigned char)0x00;
    hdr.palette16[6] = (unsigned char)0x00;
    hdr.palette16[7] = (unsigned char)0x80;
    hdr.palette16[8] = (unsigned char)0x00;
    hdr.palette16[9] = (unsigned char)0x80;
    hdr.palette16[10] = (unsigned char)0x80;
    hdr.palette16[11] = (unsigned char)0x00;
    hdr.palette16[12] = (unsigned char)0x00;
    hdr.palette16[13] = (unsigned char)0x00;
    hdr.palette16[14] = (unsigned char)0x80;
    hdr.palette16[15] = (unsigned char)0x80;

    hdr.palette16[16] = (unsigned char)0x00;
    hdr.palette16[17] = (unsigned char)0x80;
    hdr.palette16[18] = (unsigned char)0x00;
    hdr.palette16[19] = (unsigned char)0x80;
    hdr.palette16[20] = (unsigned char)0x80;
    hdr.palette16[21] = (unsigned char)0xC0;
    hdr.palette16[22] = (unsigned char)0xC0;
    hdr.palette16[23] = (unsigned char)0xC0;
    hdr.palette16[24] = (unsigned char)0xC0;
    hdr.palette16[25] = (unsigned char)0xDC;
    hdr.palette16[26] = (unsigned char)0xC0;
    hdr.palette16[27] = (unsigned char)0xA6;
    hdr.palette16[28] = (unsigned char)0xCA;
    hdr.palette16[29] = (unsigned char)0xF0;
    hdr.palette16[30] = (unsigned char)0x33;
    hdr.palette16[31] = (unsigned char)0x2B;

    hdr.palette16[32] = (unsigned char)0x1F;
    hdr.palette16[33] = (unsigned char)0x2B;
    hdr.palette16[34] = (unsigned char)0x23;
    hdr.palette16[35] = (unsigned char)0x1B;
    hdr.palette16[36] = (unsigned char)0x5F;
    hdr.palette16[37] = (unsigned char)0x5F;
    hdr.palette16[38] = (unsigned char)0x5F;
    hdr.palette16[39] = (unsigned char)0x2F;
    hdr.palette16[40] = (unsigned char)0x2F;
    hdr.palette16[41] = (unsigned char)0x2F;
    hdr.palette16[42] = (unsigned char)0x27;
    hdr.palette16[43] = (unsigned char)0x27;
    hdr.palette16[44] = (unsigned char)0x27;
    hdr.palette16[45] = (unsigned char)0x1F;
    hdr.palette16[46] = (unsigned char)0x1F;
    hdr.palette16[47] = (unsigned char)0x1F;

    hdr.reserved = 0;
    hdr.color_planes = 3;
    hdr.bytes_per_line = w;
    hdr.palette_type = 1;

    for (unsigned int i = 0; i < 58; i++)
        hdr.filler[i] = 0;

    width = (unsigned short)w;
    height = (unsigned short)h;
    imagebytes = (unsigned long)width * (unsigned long)height;

    himap = new unsigned long[imagebytes];

    if (himap) {
        for (unsigned long i = 0; i < imagebytes; i++)
            himap[i] = hibits[i];
    }
}

PcxImage::~PcxImage()
{
    delete[] bitmap;
    bitmap = nullptr;

    delete[] himap;
    himap = nullptr;

    imagebytes = 0;
    width = 0;
    height = 0;
}

// +--------------------------------------------------------------------+

int PcxImage::Load(const char* fname)
{
    unsigned long i;
    short mode = BYTEMODE;
    short bytecount = 0;

    unsigned char abyte = 0;
    unsigned char* p = nullptr;

    FILE* f = nullptr;

#if defined(_MSC_VER)
    fopen_s(&f, fname, "rb");
#else
    f = fopen(fname, "rb");
#endif
    if (!f) {
        return PCX_NOFILE;
    }

    fread(&hdr, sizeof(PcxHeader), 1, f);

    // indexed (256 color) PCX
    if (hdr.color_planes == 1) {
        width = (unsigned short)(1 + hdr.xmax - hdr.xmin);
        height = (unsigned short)(1 + hdr.ymax - hdr.ymin);
        imagebytes = (unsigned long)width * (unsigned long)height;

        if (imagebytes > MAX_SIZE) {
            fclose(f);
            return PCX_TOOBIG;
        }

        // palette at end
        fseek(f, -768L, SEEK_END);
        fread(pal, 768, 1, f);

        // pixel data begins after header
        fseek(f, (long)sizeof(PcxHeader), SEEK_SET);

        delete[] himap;  himap = nullptr;
        delete[] bitmap; bitmap = nullptr;

        himap = new unsigned long[imagebytes];
        if (!himap) {
            fclose(f);
            return PCX_NOMEM;
        }

        // force alpha to 255
        memset(himap, 0xff, imagebytes * 4);

        unsigned long* pix = himap;
        for (i = 0; i < imagebytes; i++) {
            if (mode == BYTEMODE) {
                abyte = (unsigned char)fgetc(f);

                if (abyte > 0xbf) {
                    bytecount = (short)(abyte & 0x3f);
                    abyte = (unsigned char)fgetc(f);
                    if (--bytecount > 0)
                        mode = RUNMODE;
                }
            }
            else if (--bytecount == 0) {
                mode = BYTEMODE;
            }

            *pix++ = 0xff000000 |
                (pal[3 * abyte] << 16) |
                (pal[3 * abyte + 1] << 8) |
                (pal[3 * abyte + 2]);
        }
    }

    // true color PCX (24-bit = 3 planes)
    else {
        width = (unsigned short)(1 + hdr.xmax - hdr.xmin);
        height = (unsigned short)(1 + hdr.ymax - hdr.ymin);
        imagebytes = (unsigned long)width * (unsigned long)height;

        if (imagebytes > MAX_SIZE) {
            fclose(f);
            return PCX_TOOBIG;
        }

        delete[] himap;  himap = nullptr;
        delete[] bitmap; bitmap = nullptr;

        himap = new unsigned long[imagebytes];
        if (!himap) {
            fclose(f);
            return PCX_NOMEM;
        }

        // force alpha to 255
        memset(himap, 0xff, imagebytes * 4);

        for (int row = 0; row < (int)height; row++) {
            // RED, GREEN, BLUE planes
            for (int plane = 2; plane >= 0; plane--) {
                // p points into BGRA dword array as bytes:
                p = ((unsigned char*)himap) + (int)width * row * 4 + plane;

                for (int col = 0; col < (int)width; col++) {
                    if (mode == BYTEMODE) {
                        abyte = (unsigned char)fgetc(f);

                        if (abyte > 0xbf) {
                            bytecount = (short)(abyte & 0x3f);
                            abyte = (unsigned char)fgetc(f);
                            if (--bytecount > 0)
                                mode = RUNMODE;
                        }
                    }
                    else if (--bytecount == 0) {
                        mode = BYTEMODE;
                    }

                    *p = abyte;
                    p += 4;
                }
            }
        }
    }

    fclose(f);
    return PCX_OK;
}

// +--------------------------------------------------------------------+
// Buffer load (memory)
// +--------------------------------------------------------------------+

int PcxImage::LoadBuffer(unsigned char* buf, int len)
{
    if (!buf || len < (int)sizeof(PcxHeader))
        return PCX_NOFILE;

    unsigned long i;
    short mode = BYTEMODE;
    short bytecount = 0;

    unsigned char abyte = 0;
    unsigned char* p = nullptr;

    unsigned char* fp = buf;

    memcpy(&hdr, fp, sizeof(PcxHeader));
    fp += sizeof(PcxHeader);

    // indexed (256 color) PCX
    if (hdr.color_planes == 1) {
        width = (unsigned short)(1 + hdr.xmax - hdr.xmin);
        height = (unsigned short)(1 + hdr.ymax - hdr.ymin);
        imagebytes = (unsigned long)width * (unsigned long)height;

        if (imagebytes > MAX_SIZE)
            return PCX_TOOBIG;

        if (len < (int)sizeof(PcxHeader) + 768)
            return PCX_NOFILE;

        // palette at end:
        memcpy(pal, buf + len - 768, 768);

        delete[] himap;  himap = nullptr;
        delete[] bitmap; bitmap = nullptr;

        himap = new unsigned long[imagebytes];
        if (!himap)
            return PCX_NOMEM;

        // force alpha to 255
        memset(himap, 0xff, imagebytes * 4);

        unsigned long* pix = himap;
        for (i = 0; i < imagebytes; i++) {
            if (mode == BYTEMODE) {
                if (fp >= buf + len)
                    return PCX_NOFILE;

                abyte = *fp++;

                if (abyte > 0xbf) {
                    bytecount = (short)(abyte & 0x3f);

                    if (fp >= buf + len)
                        return PCX_NOFILE;

                    abyte = *fp++;
                    if (--bytecount > 0)
                        mode = RUNMODE;
                }
            }
            else if (--bytecount == 0) {
                mode = BYTEMODE;
            }

            *pix++ = 0xff000000 |
                (pal[3 * abyte] << 16) |
                (pal[3 * abyte + 1] << 8) |
                (pal[3 * abyte + 2]);
        }
    }

    // true color PCX (24-bit = 3 planes)
    else {
        width = (unsigned short)(1 + hdr.xmax - hdr.xmin);
        height = (unsigned short)(1 + hdr.ymax - hdr.ymin);
        imagebytes = (unsigned long)width * (unsigned long)height;

        if (imagebytes > MAX_SIZE)
            return PCX_TOOBIG;

        delete[] himap;  himap = nullptr;
        delete[] bitmap; bitmap = nullptr;

        himap = new unsigned long[imagebytes];
        if (!himap)
            return PCX_NOMEM;

        // force alpha to 255
        memset(himap, 0xff, imagebytes * 4);

        for (int row = 0; row < (int)height; row++) {
            for (int plane = 2; plane >= 0; plane--) {
                p = ((unsigned char*)himap) + (int)width * row * 4 + plane;

                for (int col = 0; col < (int)width; col++) {
                    if (mode == BYTEMODE) {
                        if (fp >= buf + len)
                            return PCX_NOFILE;

                        abyte = *fp++;

                        if (abyte > 0xbf) {
                            bytecount = (short)(abyte & 0x3f);

                            if (fp >= buf + len)
                                return PCX_NOFILE;

                            abyte = *fp++;
                            if (--bytecount > 0)
                                mode = RUNMODE;
                        }
                    }
                    else if (--bytecount == 0) {
                        mode = BYTEMODE;
                    }

                    *p = abyte;
                    p += 4;
                }
            }
        }
    }

    return PCX_OK;
}

// +--------------------------------------------------------------------+

int PcxImage::Save(const char* sfilename)
{
    FILE* f = nullptr;

#if defined(_MSC_VER)
    fopen_s(&f, sfilename, "wb");
#else
    f = fopen(filename, "wb");
#endif
    if (!f)
        return PCX_NOFILE;

    fwrite(&hdr, sizeof(PcxHeader), 1, f);

    if (hdr.color_planes == 1) {
        if (!bitmap) {
            fclose(f);
            return PCX_NOFILE;
        }

        unsigned char* pBits = bitmap;
        unsigned long total = imagebytes;
        unsigned long row = 0;
        unsigned char palette_marker = 12;

        while (total) {
            unsigned char* start = pBits;
            unsigned char count = 0;

            while (*start == *pBits && count < 0x3f && row < width) {
                pBits++;
                count++;
                row++;
            }

            if (count > 1 || *start > 0xbf) {
                unsigned char b[2];
                b[0] = (unsigned char)(0xc0 | count);
                b[1] = *start;
                fwrite(b, 2, 1, f);
            }
            else {
                fwrite(start, 1, 1, f);
            }

            total -= count;

            if (row == width)
                row = 0;
        }

        fwrite(&palette_marker, 1, 1, f);
        fwrite(pal, 768, 1, f);
    }

    // 24-bit PCX (3 planes)
    else {
        if (!himap) {
            fclose(f);
            return PCX_NOFILE;
        }

        for (int row = 0; row < (int)height; row++) {
            for (int plane = 2; plane >= 0; plane--) {
                unsigned long col = 0;
                unsigned char* pBits = ((unsigned char*)himap) + (int)width * row * 4 + plane;

                while (col < width) {
                    unsigned char* start = pBits;
                    unsigned char count = 0;

                    while (*start == *pBits && count < 0x3f && col < width) {
                        pBits += 4;
                        count++;
                        col++;
                    }

                    if (count > 1 || *start > 0xbf) {
                        unsigned char b[2];
                        b[0] = (unsigned char)(0xc0 | count);
                        b[1] = *start;
                        fwrite(b, 2, 1, f);
                    }
                    else {
                        fwrite(start, 1, 1, f);
                    }
                }
            }
        }
    }

    fclose(f);
    return PCX_OK;
}
