#pragma once

#define UNREFERENCED_PARAMETER(x) EZ_IGNORE_UNUSED(x)

namespace DirectX
{
    enum TEX_COMPRESS_FLAGS : unsigned long
    {
        TEX_COMPRESS_DEFAULT            = 0,

        TEX_COMPRESS_RGB_DITHER         = 0x10000,
            // Enables dithering RGB colors for BC1-3 compression

        TEX_COMPRESS_A_DITHER           = 0x20000,
            // Enables dithering alpha for BC1-3 compression

        TEX_COMPRESS_DITHER             = 0x30000,
            // Enables both RGB and alpha dithering for BC1-3 compression

        TEX_COMPRESS_UNIFORM            = 0x40000,
            // Uniform color weighting for BC1-3 compression; by default uses perceptual weighting

        TEX_COMPRESS_BC7_USE_3SUBSETS   = 0x80000,
            // Enables exhaustive search for BC7 compress for mode 0 and 2; by default skips trying these modes

        TEX_COMPRESS_BC7_QUICK          = 0x100000,
            // Minimal modes (usually mode 6) for BC7 compression

        TEX_COMPRESS_SRGB_IN            = 0x1000000,
        TEX_COMPRESS_SRGB_OUT           = 0x2000000,
        TEX_COMPRESS_SRGB               = (TEX_COMPRESS_SRGB_IN | TEX_COMPRESS_SRGB_OUT),
            // if the input format type is IsSRGB(), then SRGB_IN is on by default
            // if the output format type is IsSRGB(), then SRGB_OUT is on by default

        TEX_COMPRESS_PARALLEL           = 0x10000000,
            // Compress is free to use multithreading to improve performance (by default it does not use multithreading)
    };
}