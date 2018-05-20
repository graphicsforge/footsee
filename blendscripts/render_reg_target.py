# NOTE: This requires ^Blender 2.68a

import bpy
import os, sys
import time
import math
from mathutils import *

blend_dir = "./blendscripts"
if blend_dir not in sys.path:
   sys.path.append(blend_dir)
import load

def render_thumb(image_path, gl=False, anim=False):
    if gl:
        if anim:
            bpy.data.scenes['Scene'].render.filepath = "/tmp/"+ob.name+"#"
            bpy.ops.render.opengl(animation=True)
        else:
            bpy.ops.render.opengl(write_still=True)
            bpy.data.images['Render Result'].save_render(filepath=image_path)
    else:
        if anim:
            bpy.data.scenes['Scene'].render.filepath = "/tmp/"+ob.name+"#"
            bpy.ops.render.render(animation=True)
        else:
            bpy.ops.render.render(write_still=False)
            bpy.data.images['Render Result'].save_render(filepath=image_path)

# inputs
input_file = os.environ['INPUT_FILE']
try:
    camera_rotation = float(os.environ['ROTATION'])
except:
    camera_rotation = 0.0

# outputs
out_file = os.environ['OUTPUT_FILE']

# start
start_time = time.time()

render_object = load.from_file(input_file)

out_extension = os.path.splitext(out_file)[-1][1:]
if out_extension == 'png':
    # set world color to white
    bpy.data.worlds["World"].horizon_color.r = 1
    bpy.data.worlds["World"].horizon_color.g = 1
    bpy.data.worlds["World"].horizon_color.b = 1

    # set output resolution
    bpy.data.scenes["Scene"].render.resolution_x = 1280
    bpy.data.scenes["Scene"].render.resolution_y = 720
    bpy.data.scenes["Scene"].render.resolution_percentage = 100
    if camera_rotation != 0:
        bpy.data.scenes["Scene"].render.resolution_x = 720
        bpy.data.scenes["Scene"].render.resolution_y = 1280

    camera_distance = 100
    bpy.ops.object.camera_add(location=(math.sin(camera_rotation)*camera_distance, 0, math.cos(camera_rotation)*camera_distance), rotation=(0.0, camera_rotation, 0.0))
    for obj in bpy.context.scene.objects:
        if obj.type == 'CAMERA':
            obj.data.type = 'ORTHO'
            obj.data.ortho_scale = 500
            obj.data.clip_start = 60
            obj.data.clip_end = 150
            bpy.context.scene.camera = obj

    render_thumb(out_file)
elif out_extension == 'stl':
    bpy.ops.export_mesh.stl(filepath=out_file)
else:
    raise Exception('unknown out_file extension: ' + out_extension)

# end profiling
elapsed_time = time.time()-start_time
print("Finished in %.2f seconds" % elapsed_time)
