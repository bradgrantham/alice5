#ifndef IMAGE_H
#define IMAGE_H

#include <cstdint>
#include "basic_types.h"

struct Image
{
    enum Format {
        // Matching Vulkan 1.0
        UNDEFINED = 0,
        FORMAT_R8G8B8_UNORM = 23,
        FORMAT_R8G8B8A8_UNORM = 37,
        FORMAT_R32G32B32_SFLOAT = 106,
        FORMAT_R32G32B32A32_SFLOAT = 109
    };

    static size_t getPixelSize(Format fmt)
    {
        switch(fmt) {
            case FORMAT_R8G8B8_UNORM: return 3; break;
            case FORMAT_R8G8B8A8_UNORM: return 4; break;
            case FORMAT_R32G32B32_SFLOAT: return 12; break;
            case FORMAT_R32G32B32A32_SFLOAT: return 16; break;

            default:
            case UNDEFINED:
                throw std::runtime_error("unimplemented image format " + std::to_string(fmt));
                break;
        };
    };

    enum Dim {
        // Matching Vulkan 1.0
        DIM_1D = 0,
        DIM_2D = 1,
        DIM_3D = 2,
        DIM_CUBE = 3
    };

    Format format;
    size_t pixelSize;
    Dim dim;
    uint32_t width, height, depth, slices;
    unsigned char *storage;

    unsigned char *getPixelAddress(int s, int t, int r, int q)
    {
        return storage + (q * depth * width * height + r * width * height + t * width + s) * pixelSize;
    }
    unsigned char *getPixelAddress(int s, int t, int r)
    {
        return storage + (r * width * height + t * width + s) * pixelSize;
    }
    unsigned char *getPixelAddress(int s, int t)
    {
        return storage + (t * width + s) * pixelSize;
    }

    Image() :
        format(UNDEFINED),
        pixelSize(0),
        dim(DIM_2D),
        storage(nullptr)
    {}
    Image(Format format_, Dim dim_, uint32_t w_) {}
    Image(Format format_, Dim dim_, uint32_t w_, uint32_t h_, uint32_t d_) {}
    Image(Format format_, Dim dim_, uint32_t w_, uint32_t h_) :
        format(format_),
        pixelSize(getPixelSize(format_)),
        dim(dim_),
        width(w_),
        height(h_),
        depth(1),
        slices(1),
        storage(new unsigned char [width * height * depth * slices * pixelSize])
    {
        assert(dim == DIM_2D);
    }
    ~Image()
    {
        delete[] storage;
    }

#if 0
    static get(const unsigned char *pixel, const v4float& v)
    {
        switch(format) {
            case FORMAT_R8G8U8_UNORM: {
                for(int c = 0; c < 3; c++) {
                    v[0] = pixel[c] / 255.99;
                }
                v[3] = 1.0;
                break;
            }
            default: {
                throw std::runtime_error("get() : unimplemented image format " + std::to_string(fmt));
                break; // not reached
            }
        }
    }
#endif

    // There's probably a clever C++ way to do this with variadic templates...
    // void setPixel(int s, int t, int r, int q, const v4float& v) {}
    // void setPixel(int s, int t, int r,  const v4float& v) {}
    void set(int s, int t, const v4float& v)
    {
        assert((s >= 0) || (t >= 0) || (s < width) || (t < height));
        assert(dim == DIM_2D);
        set(getPixelAddress(s, t), v);
    }
    // void get(int s, int t, int r, int q, v4float& v) {}
    // void get(int s, int t, int r, v4float& v) {}
    // void get(int s, int t, v4float& v) {}

    // implement first only rgb, rgba and f32 and ub8
    // Read(filename, format); // XXX should construct an image with this and use move semantics
    // Write(filename);

private:
    void set(unsigned char *pixel, const v4float& v)
    {
        switch(format) {
            case FORMAT_R8G8B8_UNORM: {
                for(int c = 0; c < 3; c++) {
                    // ShaderToy clamps the color.
                    pixel[c] = std::clamp(int(v[c]*255.99), 0, 255);
                }
                break;
            }
            default: {
                throw std::runtime_error("set() : unimplemented image format " + std::to_string(format));
                break; // not reached
            }
        }
    }
};

#endif /* IMAGE_H */
