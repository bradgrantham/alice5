#include <map>
#include <set>
#include <functional>

#include "shadertoy.h"
#include "basic_types.h"
#include "image.h"
#include "json.hpp"

#ifdef USE_CPP17_FILESYSTEM
// filesystem still not available in XCode 2019/04/04
#include <filesystem>
#else
#include <libgen.h>
#endif

using json = nlohmann::json;

void sortInDependencyOrder(
    const std::vector<ShaderToyRenderPassPtr>& passes,
    const std::map<int, ShaderToyRenderPassPtr>& channelIdsToPasses,
    const std::map<std::string, ShaderToyRenderPassPtr>& namesToPasses,
    std::vector<ShaderToyRenderPassPtr>& passesOrdered)
{
    std::set<ShaderToyRenderPassPtr> passesToVisit(passes.begin(), passes.end());

    std::function<void(ShaderToyRenderPassPtr)> visit;

    visit = [&visit, &passesToVisit, &channelIdsToPasses, &passesOrdered](ShaderToyRenderPassPtr pass) {
        if(passesToVisit.count(pass) > 0) {
            passesToVisit.erase(pass);
            for(ShaderToyImage& input: pass->inputs) {
                if(channelIdsToPasses.find(input.id) != channelIdsToPasses.end()) {
                    visit(channelIdsToPasses.at(input.id));
                }
            }
            passesOrdered.push_back(pass);
        }
    };

    visit(namesToPasses.at("Image"));
}

std::string getFilepathAdjacentToPath(const std::string& filename, std::string adjacent)
{
    std::string result;

#ifdef USE_CPP17_FILESYSTEM

    // filesystem still not available in XCode 2019/04/04
    std::filesystem::path adjacent_path(adjacent);
    std::filesystem::path adjacent_dirname = adjacent_path.parent_path();
    std::filesystem::path code_path(filename);

    if(code_path.is_relative()) {
        std::filesystem::path full_path(adjacent_dirname + code_path);
        result = full_path;
    }

#else

    if(filename[0] != '/') {
        // Assume relative path, get directory from JSON filename
        char *adjacent_copy = strdup(adjacent.c_str());;
        result = std::string(dirname(adjacent_copy)) + "/" + filename;
        free(adjacent_copy);
    }

#endif

    return result;
}

void getOrderedRenderPassesFromJSON(const std::string& filename, std::vector<ShaderToyRenderPassPtr>& renderPassesOrdered, const CommandLineParameters& params)
{
    json j = json::parse(readFileContents(filename));

    std::vector<ShaderToyRenderPassPtr> renderPasses;

    ShaderSource common_code;

    // Go through the render passes and load code in from files as necessary
    auto& renderPassesInJSON = j["Shader"]["renderpass"];

    for (auto& pass: renderPassesInJSON) {

        // If special key "codefile" is present, use that file as
        // the shader code instead of the value of key "code".

        if(pass.find("codefile") != pass.end()) {
            std::string code_filename = pass["codefile"];

            code_filename = getFilepathAdjacentToPath(code_filename, filename);

            pass["full_code_filename"] = code_filename;
            pass["code"] = readFileContents(code_filename);
        }

	// If this is the "common" pass, it includes text which is
	// preprended to all other passes.

        if(pass["type"] == "common") {
            common_code.code = pass["code"];
            if(pass.find("codefile") != pass.end()) {
                common_code.filename = pass["codefile"];
            } else {
                common_code.filename = pass["name"].get<std::string>() + " JSON inline";
            }
        } 
    }

    std::map<int, ShaderToyRenderPassPtr> channelIdsToPasses;
    std::map<std::string, ShaderToyRenderPassPtr> namesToPasses;

    for (auto& pass: renderPassesInJSON) {
        std::vector<ShaderSource> sources;

        if(pass["type"] == "common") {
            /* Don't make a renderpass for "common", it's just common preamble code for all passes */
            continue;
        }
        if((pass["type"] != "buffer") && (pass["type"] != "image")) {
            throw std::runtime_error("pass type \"" + pass["type"].get<std::string>() + "\" is not supported");
        }

        ShaderSource asset_preamble;
        ShaderSource shader;

        std::vector<ShaderToyImage> inputs;

        for(auto& input: pass["inputs"]) {
            int channelNumber = input["channel"].get<int>();
            int channelId = input["id"].get<int>();

            const std::string& src = input["src"].get<std::string>();

            bool isABuffer = (src.find("/media/previz/buffer") != std::string::npos);
            bool isKeyboard = (input["ctype"].get<std::string>() == "keyboard");

            if(isABuffer) {

                /* will hook up to output from source pass */
                if(false) printf("pass \"%s\", channel number %d and id %d\n", pass["name"].get<std::string>().c_str(), channelNumber, channelId);
                Sampler sampler; /*  = makeSamplerFromJSON(input); */
                inputs.push_back({channelNumber, channelId, {nullptr, sampler}});
                asset_preamble.code += "layout (binding = " + std::to_string(channelNumber + 1) + ") uniform sampler2D iChannel" + std::to_string(channelNumber) + ";\n";

            } else if(isKeyboard) {

                ImagePtr fakeKeyboard(new Image(Image::FORMAT_R32G32B32A32_SFLOAT, Image::DIM_2D, 256, 3));
                for(int j = 0; j < 3; j++) {
                    for(int i = 0; i < 256; i++) {
                        fakeKeyboard->set(i, j, {0, 0, 0, 0});
                    }
                }
                Sampler sampler;
                inputs.push_back({channelNumber, channelId, {fakeKeyboard, sampler}});
                asset_preamble.code += "layout (binding = " + std::to_string(channelNumber + 1) + ") uniform sampler2D iChannel" + std::to_string(channelNumber) + ";\n";

            } else if(input["ctype"].get<std::string>() != "texture") {

                throw std::runtime_error("unsupported input type " + input["ctype"].get<std::string>());

            } else if(input.find("locally_saved") == input.end()) {

                throw std::runtime_error("downloading assets is not supported; didn't find \"locally_saved\" key for an asset");

            } else {

                std::string asset_filename = getFilepathAdjacentToPath(input["locally_saved"].get<std::string>(), filename);

                bool flipInY = ((input.find("vflip") != input.end()) && (input["vflip"].get<std::string>() == std::string("true")));

                ImagePtr image = loadImage(asset_filename, flipInY);
                if(!image) {
                    std::cerr << "image load failed\n";
                    exit(EXIT_FAILURE);
                }

                Sampler sampler; /*  = makeSamplerFromJSON(input); */
                inputs.push_back({channelNumber, channelId, {image, sampler}});
                asset_preamble.code += "layout (binding = " + std::to_string(channelNumber + 1) + ") uniform sampler2D iChannel" + std::to_string(channelNumber) + ";\n";
            }
        }

        if(asset_preamble.code != "") {
            asset_preamble.filename = "BuiltIn Asset Preamble";
            sources.push_back(asset_preamble);
        }

        if(common_code.code != "") {
            sources.push_back(common_code);
        }

        if(pass.find("full_code_filename") != pass.end()) {
            // If we find "full_code_filename", that's the
            // fully-qualified absolute on-disk path we loaded.  Set
            // that for debugging.
            shader.filename = pass["full_code_filename"];
        } else if(pass.find("codefile") != pass.end()) {
            // If we find only "codefile", that's the on-disk path
            // the JSON specified.  Set that for debugging.
            shader.filename = pass["codefile"];
        } else {
            // If we find neither "codefile" nor "full_code_filename",
            // then the shader came out of the renderpass "code", so
            // at least use the pass name to aid debugging.
            shader.filename = std::string("JSON inline shader from pass ") + pass["name"].get<std::string>();
        }

        shader.code = pass["code"];
        sources.push_back(shader);

        int channelId = pass["outputs"][0]["id"].get<int>();
        Image::Format format = (pass["name"].get<std::string>() == "Image") ? Image::FORMAT_R8G8B8_UNORM : Image::FORMAT_R32G32B32A32_SFLOAT;
        ImagePtr image(new Image(format, Image::DIM_2D, params.outputWidth, params.outputHeight));
        ShaderToyImage output {0, channelId, { image, Sampler {}}};
        ShaderToyRenderPassPtr rpass(new ShaderToyRenderPass(
            pass["name"],
            {inputs},
            {output},
            sources,
            params));

        channelIdsToPasses[channelId] = rpass;
        namesToPasses[pass["name"]] = rpass;
        renderPasses.push_back(rpass);
    }

    // Hook up outputs to inputs
    for(auto& pass: renderPasses) {
        for(size_t i = 0; i < pass->inputs.size(); i++) {
            ShaderToyImage& input = pass->inputs[i];
            if(!input.sampledImage.image) {
                int channelId = input.id;
                ShaderToyImage& source = channelIdsToPasses.at(channelId)->outputs[0];
                pass->inputs[i].sampledImage = source.sampledImage;
            }
        }
    }
    
    // and sort in dependency order
    sortInDependencyOrder(renderPasses, channelIdsToPasses, namesToPasses, renderPassesOrdered);
}
