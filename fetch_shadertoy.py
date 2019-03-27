
# Fetch shaders from ShaderToy.

import sys
import json
import urllib2
import subprocess

def fetch_json(url):
    return json.load(urllib2.urlopen(url))

def analyze_shader(shader_id, shadertoy_key):
    shader = fetch_json("https://www.shadertoy.com/api/v1/shaders/%s?key=%s" % (shader_id, shadertoy_key))
    print "https://www.shadertoy.com/view/%s (%s)" % (shader_id, shader["Shader"]["info"]["name"])
    renderpass = shader["Shader"]["renderpass"]
    if len(renderpass) != 1:
        print "    Skipping because number of passes is %d" % len(renderpass)
        return False
    renderpass = renderpass[0]
    inputs = renderpass["inputs"]
    if len(inputs) != 0:
        print "    Skipping because number of inputs is %d" % len(inputs)
        return False
    code = renderpass["code"]

    p = subprocess.Popen(["./shade", "-v", "-"], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    stdoutdata, stderrdata = p.communicate(code)

    if p.returncode != 0:
        print stdoutdata, stderrdata
        return False

    unimplemented = set()
    for line in stdoutdata.split("\n"):
        if "unimplemented opcode" in line:
            unimplemented.add(line)

    print "    Needs %d more instructions" % len(unimplemented)

def main(shadertoy_key):
    recent_shaders = fetch_json("https://www.shadertoy.com/api/v1/shaders?sort=newest&num=100&key=%s" % shadertoy_key)
    for shader_id in recent_shaders["Results"]:
        analyze_shader(shader_id, shadertoy_key)

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print "Usage: %s SHADERTOY_KEY" % sys.argv[0]
        sys.exit(1)

    shadertoy_key = sys.argv[1]

    # analyze_shader("tsjXDh", shadertoy_key)
    analyze_shader("WsSXWz", shadertoy_key)
    # main(shadertoy_key)
