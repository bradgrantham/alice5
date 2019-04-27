#include <string>
#include <vector>

#include "image.h"
#include "program.h"

struct ShaderToyImage
{
    int channelNumber;
    int id;
    SampledImage sampledImage;
};

struct ShaderSource
{
    std::string code;
    std::string filename;
    ShaderSource() {}
    ShaderSource(const std::string& code_, const std::string& filename_) :
        code(code_),
        filename(filename_)
    {}
};

struct ShaderToyRenderPass
{
    std::string name;
    std::vector<ShaderToyImage> inputs; // in channel order
    std::vector<ShaderToyImage> outputs;
    std::vector<ShaderSource> sources;
    Program pgm;
    void Render(void) {
        // set input images, uniforms, output images, call run()
    }
    ShaderToyRenderPass(const std::string& name_, std::vector<ShaderToyImage> inputs_, std::vector<ShaderToyImage> outputs_, const std::vector<ShaderSource>& sources_, const CommandLineParameters& params) :
        name(name_),
        inputs(inputs_),
        outputs(outputs_),
        sources(sources_),
        pgm(params.throwOnUnimplemented, params.beVerbose)
    {
    }
};

typedef std::shared_ptr<ShaderToyRenderPass> ShaderToyRenderPassPtr;


void getOrderedRenderPassesFromJSON(const std::string& filename, std::vector<ShaderToyRenderPassPtr>& renderPassesOrdered, const CommandLineParameters& params);

