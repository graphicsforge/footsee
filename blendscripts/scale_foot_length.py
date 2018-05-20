
import bpy
import os, sys
import time
import math
from mathutils import Matrix, Vector
import json


def warp_mesh(warp, obj):
    # TODO also warp armatures and bones
    if not hasattr(obj.data, 'vertices'):
        return
    for vert in obj.data.vertices:
        vert.co = warp*vert.co

try:
    inputs = json.loads(os.environ['INPUTS'])
    measurement = inputs["measurement"]
    target_distance = inputs["target_distance"]
except Exception as err:
    print("Error: invalid INPUTS {}".format(err), file=sys.stderr)
    exit(-1)

if bpy.ops.object.mode_set.poll():
    bpy.ops.object.mode_set(mode='OBJECT')

objs = bpy.context.scene.objects

if (len(objs) == 0):
    print("Error: no objects in scene! Did you open a proper blend file?")
    exit(-1)

# default to identity matrix
warp = Matrix([[1, 0, 0, 0],
        [0, 1, 0, 0],
        [0, 0, 1, 0],
        [0, 0, 0, 1]])

for obj in objs:
    if not obj.name == measurement:
        continue
    # if we have an object matching the name provided, scale the length
    scale = target_distance / math.sqrt(obj.dimensions.x*obj.dimensions.x + obj.dimensions.y*obj.dimensions.y + obj.dimensions.z*obj.dimensions.z)
    warp = Matrix([[scale, 0, 0, 0],
            [0, scale, 0, 0],
            [0, 0, scale, 0],
            [0, 0, 0, 1]])
    break

# scale everything
for obj in objs:
    warp_mesh(warp, obj)

bpy.ops.wm.save_mainfile(filepath=os.environ['OUTPUT'])

