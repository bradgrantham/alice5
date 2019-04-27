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

    unsigned char *getPixelAddress(int i, int j, int k, int l) const
    {
        return storage + (l * depth * width * height + k * width * height + j * width + i) * pixelSize;
    }
    unsigned char *getPixelAddress(int i, int j, int k) const
    {
        return storage + (k * width * height + j * width + i) * pixelSize;
    }
    unsigned char *getPixelAddress(int i, int j) const
    {
        return storage + (j * width + i) * pixelSize;
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

    // There's probably a clever C++ way to do this with variadic templates...
    // void setPixel(int i, int j, int k, int l, const v4float& v) {}
    // void setPixel(int i, int j, int k,  const v4float& v) {}
    void set(int i, int j, const v4float& v)
    {
        assert((i >= 0) || (j >= 0) || (i < width) || (j < height));
        assert(dim == DIM_2D);
        set(getPixelAddress(i, j), v);
    }
    // void get(int i, int j, int k, int l, v4float& v) const {} 
    // void get(int i, int j, int k, v4float& v) const {}
    void get(int i, int j, v4float& v) const
    {
        assert((i >= 0) || (j >= 0) || (i < width) || (j < height));
        assert(dim == DIM_2D);
        get(getPixelAddress(i, j), v);
    }

    // implement first only rgb, rgba and f32 and ub8
    // Read(filename, format); // XXX should construct an image with this and use move semantics
    // Write(filename);

    void writePpm(std::ostream &os) {
        os << "P6 " << width << " " << height << " 255\n";
        os.write(reinterpret_cast<char *>(getPixelAddress(0, 0)), 3*width*height);
    }

private:
    void get(const unsigned char *pixel, v4float& v) const
    {
        switch(format) {
            case FORMAT_R32G32B32A32_SFLOAT: {
                for(int c = 0; c < 4; c++) {
                    v[c] = reinterpret_cast<const float*>(pixel)[c];
                }
                break;
            }
                
            case FORMAT_R8G8B8A8_UNORM: {
                for(int c = 0; c < 4; c++) {
                    v[c] = pixel[c] / 255.99;
                }
                break;
            }

            case FORMAT_R8G8B8_UNORM: {
                for(int c = 0; c < 3; c++) {
                    v[c] = pixel[c] / 255.99;
                }
                v[3] = 1.0;
                break;
            }
            default: {
                throw std::runtime_error("get() : unimplemented image format " + std::to_string(format));
                break; // not reached
            }
        }
    }

    void set(unsigned char *pixel, const v4float& v)
    {
        switch(format) {
            case FORMAT_R32G32B32A32_SFLOAT: {
                for(int c = 0; c < 4; c++) {
                    reinterpret_cast<float*>(pixel)[c] = v[c];
                }
                break;
            }

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

typedef std::shared_ptr<Image> ImagePtr;

ImagePtr loadImage(std::string filename, bool flipInY);

struct Sampler
{
    enum AddressMode {
        REPEAT = 0,
        CLAMP_TO_EDGE = 2,
    } uAddressMode, vAddressMode;
    enum FilterMode {
        NEAREST = 0,
        LINEAR = 1,
    } filterMode;
    enum MipMapMode {
        MIPMAP_NEAREST = 0,
        MIPMAP_LINEAR = 1,
    } mipMapMode;
    bool isSRGB;

    Sampler() :
        uAddressMode(REPEAT),
        vAddressMode(REPEAT),
        filterMode(LINEAR),
        mipMapMode(MIPMAP_NEAREST),
        isSRGB(false)
        {}

    Sampler(AddressMode uMode, AddressMode vMode, FilterMode filterMode_, MipMapMode mipMapMode_, bool isSRGB_) :
        uAddressMode(uMode),
        vAddressMode(vMode),
        filterMode(filterMode_),
        mipMapMode(mipMapMode_),
        isSRGB(isSRGB_)
    {
    }

    void sample(ImagePtr, const v4float& coordinates, float lod, Register& result);
};

struct SampledImage
{
    ImagePtr image;
    Sampler sampler;
};


#endif /* IMAGE_H */
