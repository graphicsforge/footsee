# NOTE: This requires ^Blender 2.68a

import os
import time

from mathutils import *

import bpy
import bmesh
import object_print3d_utils

blend_dir = "./blendscripts"
if blend_dir not in sys.path:
   sys.path.append(blend_dir)
import load

input_file = os.environ['INPUT_FILE']
out_file = os.environ['OUTPUT_FILE']
thickness = float(os.environ['THICKNESS'])

start_time = time.time()

render_object = load.from_file(input_file)

faces_error = object_print3d_utils.mesh_helpers.bmesh_check_thick_object(render_object, thickness)
print("{} thin faces detected".format(len(faces_error)))

bm_array = faces_error

bpy.ops.object.editmode_toggle()
bpy.ops.mesh.select_all(action='SELECT')
bpy.ops.mesh.select_mode(type='FACE')

bm = bmesh.from_edit_mesh(render_object.data)
elems = getattr(bm, 'faces')[:]

for i in bm_array:
    elems[i].select_set(False)

bpy.ops.mesh.delete(type='EDGE_FACE')

out_extension = os.path.splitext(out_file)[-1][1:]
if out_extension == 'stl':
    bpy.ops.export_mesh.stl(filepath=out_file)
else:
    raise Exception('unknown out_file extension: ' + out_extension)

elapsed_time = time.time()-start_time
print("Finished in %.2f seconds" % elapsed_time)
