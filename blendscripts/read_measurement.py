

import bpy
import os, sys
import time
import math
from mathutils import Matrix, Vector
import json


try:
    inputs = json.loads(os.environ['INPUTS'])
    measurement = inputs["measurement"]
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
    if not obj.name == measurement:
        continue
    length = math.sqrt(obj.dimensions.x*obj.dimensions.x + obj.dimensions.y*obj.dimensions.y + obj.dimensions.z*obj.dimensions.z)
    print(json.dumps({measurement: length}))
    break

