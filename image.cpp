#include "image.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

ImagePtr loadImage(std::string filename, bool flipInY)
{
    int textureWidth, textureHeight, textureComponents;
    unsigned char *textureData;

    if((textureData = stbi_load(filename.c_str(), &textureWidth, &textureHeight, &textureComponents, 4)) == NULL) {
        std::cerr << "couldn't read image from " << filename << '\n';
        exit(EXIT_FAILURE);
    }

    ImagePtr image(new Image(Image::FORMAT_R8G8B8A8_UNORM, Image::DIM_2D, textureWidth, textureHeight));

    if(!image) {
        std::cerr << "couldn't create Image object\n";
        exit(EXIT_FAILURE);
    }

    unsigned char* s = textureData;
    unsigned char* d = image->storage;
    int rowsize = textureWidth * Image::getPixelSize(image->format);

    if(!flipInY) {
        for(int row = 0; row < textureHeight; row++) {
            std::copy(s + rowsize * row, s + rowsize * (row + 1), d + rowsize * row);
        }
    } else {
        for(int row = 0; row < textureHeight; row++) {
            std::copy(s + rowsize * row, s + rowsize * (row + 1), d + rowsize * (textureHeight - row - 1));
        }
    }

    stbi_image_free(textureData);

    return image;
}
