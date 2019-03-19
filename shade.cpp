#include <iostream>
#include <cstdio>
#include <fstream>

#include <StandAlone/ResourceLimits.h>
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

    const char* strings[1] = { text.c_str() };
    shader->setStringsWithLengthsAndNames(strings, NULL, &argv[1], 1);

    shader->setEnvInput(glslang::EShSourceGlsl, EShLangFragment, glslang::EShClientVulkan, glslang::EShTargetVulkan_1_0);

    shader->setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_1);

    shader->setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_3);

    EShMessages messages = (EShMessages)(EShMsgDefault | EShMsgSpvRules | EShMsgVulkanRules | EShMsgDebugInfo);

    glslang::TShader::ForbidIncluder includer;
    TBuiltInResource resources;

    resources = glslang::DefaultTBuiltInResource;

    ShInitialize();

    if (!shader->parse(&resources, 110, false, messages, includer)) {
        std::cerr << "compile failed\n";
        exit(EXIT_FAILURE);
    }

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
