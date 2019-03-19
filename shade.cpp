#include <iostream>
#include <cstdio>
#include <fstream>

#include <glslang/Public/ShaderLang.h>

const int imageWidth = 256;
const int imageHeight = 256;
unsigned char imageBuffer[imageHeight][imageWidth][3];

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

std::string readFileContents(std::string shaderFileName)
{
    std::ifstream shaderFile(shaderFileName.c_str(), std::ios::in | std::ios::binary | std::ios::ate);
    std::ifstream::pos_type size = shaderFile.tellg();
    shaderFile.seekg(0, std::ios::beg);

    std::string text(size, '\0');
    shaderFile.read(&text[0], size);

    return text;
}

int main(int argc, char **argv)
{
    if(argc < 2) {
        std::cerr << "usage: " << argv[0] << " fragshader.frag\n";
        exit(EXIT_FAILURE);
    }

    std::string text = readFileContents(argv[1]);

    glslang::TShader *shader = new glslang::TShader(EShLangFragment);

#if 0
    const char* strings[1];
    strings[0] = text.c_str();
    shader->setStringsWithLengthsAndNames(strings, NULL, &argv[1], 1);


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
