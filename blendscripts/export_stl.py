
import bpy
import os, sys
import time
import json

blend_dir = "./blendscripts"
if blend_dir not in sys.path:
   sys.path.append(blend_dir)
import load

try:
    inputs = json.loads(os.environ['INPUTS'])
    model_name = inputs["model_name"]
except Exception as err:
    print("Error: invalid INPUTS {}".format(err), file=sys.stderr)
    exit(-1)

# outputs
out_file = os.environ['OUTPUT']

if bpy.ops.object.mode_set.poll():
    bpy.ops.object.mode_set(mode='OBJECT')

objs = bpy.context.scene.objects

if (len(objs) == 0):
    print("Error: no objects in scene! Did you open a proper blend file?")
    exit(-1)

for obj in objs:
    if not obj.name == model_name:
        obj.select = False
        continue
    bpy.context.scene.objects.active = obj
    obj.select = True
 
bpy.ops.export_mesh.stl(filepath=out_file, use_mesh_modifiers=True)
