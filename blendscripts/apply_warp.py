
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
    warp = Matrix(inputs["warp"])
except Exception as err:
    print("Error: invalid INPUTS {}".format(err), file=sys.stderr)
    exit(-1)

if bpy.ops.object.mode_set.poll():
    bpy.ops.object.mode_set(mode='OBJECT')

objs = bpy.context.scene.objects

if (len(objs) == 0):
    print("Error: no objects in scene! Did you open a proper blend file?")
    exit(-1)


for obj in objs:
    print("warping {}".format(obj.name))
    warp_mesh(warp, obj)

bpy.ops.wm.save_mainfile(filepath=os.environ['OUTPUT'])

