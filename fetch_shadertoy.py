
# Fetch shaders from ShaderToy.

import sys
import json
import urllib.request
import subprocess

def fetch_json(url):
    # If you get a certificate error here on Mac, run this:
    #    % /Applications/Python\ 3.6/Install\ Certificates.command
    return json.load(urllib.request.urlopen(url))

def analyze_shader(shader_id, shadertoy_key):
    shader = fetch_json("https://www.shadertoy.com/api/v1/shaders/%s?key=%s" % (shader_id, shadertoy_key))
    print("https://www.shadertoy.com/view/%s (%s)" % (shader_id, shader["Shader"]["info"]["name"]))
    renderpass = shader["Shader"]["renderpass"]
    if len(renderpass) != 1:
        print("    Skipping because number of passes is %d" % len(renderpass))
        return False
    renderpass = renderpass[0]
    inputs = renderpass["inputs"]
    if len(inputs) != 0:
        print("    Skipping because number of inputs is %d" % len(inputs))
        return False
    code = renderpass["code"]

    p = subprocess.Popen(["./shade", "-n", "-"], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdoutdata, stderrdata = p.communicate(code.encode("utf-8"))

    stdoutdata = stdoutdata.decode("utf-8")
    stderrdata = stderrdata.decode("utf-8")

    unimplemented = set()
    for line in stdoutdata.split("\n"):
        if "unimplemented opcode" in line:
            unimplemented.add(line)

    print("    Needs %d more instructions:" % len(unimplemented))
    for inst in unimplemented:
        print("        " + inst)

def main(shadertoy_key):
    recent_shaders = fetch_json("https://www.shadertoy.com/api/v1/shaders?sort=newest&num=100&key=%s" % shadertoy_key)
    for shader_id in recent_shaders["Results"]:
        analyze_shader(shader_id, shadertoy_key)

if __name__ == "__main__":
     if sys.version_info.major != 3:
        sys.stderr.write("Must run with Python 3.\n")
        sys.exit(1)

    if len(sys.argv) != 2:
        print("Usage: %s SHADERTOY_KEY" % sys.argv[0])
        sys.exit(1)

    shadertoy_key = sys.argv[1]

    # analyze_shader("tsjXDh", shadertoy_key)
    # analyze_shader("WsSXWz", shadertoy_key)
    main(shadertoy_key)
