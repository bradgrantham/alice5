#include <cstdio>
#include <fstream>

const int imageWidth = 256;
const int imageHeight = 256;
unsigned char imageBuffer[imageHeight][imageWidth][3];

namespace glslang {
    typedef void* TShader;
};

struct fvec4 {
    float v[4];
    float& operator[](int b) {
        return v[b];
    }
};

void eval(glslang::TShader *shader, float u, float v, fvec4& color)
{
    color[0] = u;
    color[1] = v;
    color[2] = 0.5f;
    color[3] = 1.0f;
}

int main(int argc, char **argv)
{
    glslang::TShader *shader = nullptr;

#if 0
    shader = new glslang::TShader(glslang::EShLangFragment);

    shader->setStringsWithLengthsAndNames(text, NULL, filenames, count);

    shader->setEnvInput(glslang::EShSourceGlsl, glslang::EShLangFragment, client, version);
    shader->setEnvClient(client, clientVersion);
    shader->setEnvTarget(language, targetVersion);

    if (!shader->parse(&resources, defaultVersion, false, messages, includer)) {
        /* compile failed */
    }
#endif

    for(int y = 0; y < imageHeight; y++)
        for(int x = 0; x < imageWidth; x++) {
            fvec4 color;
            float u = (x + .5) / imageWidth;
            float v = (y + .5) / imageHeight;
            eval(shader, u, v, color);
            for(int c = 0; c < 3; c++) {
                imageBuffer[y][x][c] = color[c] * 255;
            }
        }

    std::ofstream imageFile("shaded.ppm", std::ios::out | std::ios::binary);
    imageFile << "P6 " << imageWidth << " " << imageHeight << " 255\n";
    imageFile.write(reinterpret_cast<const char *>(imageBuffer), sizeof(imageBuffer));
}
