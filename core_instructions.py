import json
import sys

spirv = json.loads(sys.stdin.read())

for insn in spirv["instructions"]:
    if ("capabilities" not in insn) or ("Shader" in insn):
        print insn["opname"]
