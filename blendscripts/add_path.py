

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
    svg_path = inputs["svg_path"]
    model_name = inputs["model_name"]
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
    if obj.name==model_name:
       target_model = obj 
    if obj.name=="text":
        text_position = obj.data.vertices[0].co
    obj.select = False
 
bpy.ops.import_curve.svg(filepath=svg_path)
# combine all curves (that svg could have multiple paths)
for obj in objs:
    obj.select = obj.type=="CURVE"
    if obj.type=="CURVE":
        bpy.context.scene.objects.active = obj
        path_obj = obj
path_obj.name = "imported_path"
bpy.ops.object.join()
bpy.ops.object.convert(target='MESH')
target_width = 50.0
scale_factor = target_width/path_obj.dimensions.x
bpy.ops.transform.resize(value=(-scale_factor,scale_factor,scale_factor))
bpy.ops.object.transform_apply(location=True,scale=True)
bpy.ops.transform.translate(value=(target_width/2.0,0.0,-10.0))
bpy.ops.transform.translate(value=(text_position))
bpy.ops.object.transform_apply(location=True,scale=True)

for vert in path_obj.data.vertices:
    vert.normal = (0,0,1)
# engrave inwards
bpy.ops.object.mode_set(mode='EDIT')
bpy.ops.mesh.select_all(action='SELECT')
bpy.ops.mesh.extrude_region_move(TRANSFORM_OT_translate={"value":(0,0,2),"constraint_axis":(False,False,True)})
bpy.ops.object.vertex_group_assign_new()
#bpy.ops.mesh.select_all(action='SELECT')
#bpy.ops.mesh.print3d_clean_non_manifold()
bpy.ops.object.mode_set(mode='OBJECT')

bpy.ops.object.modifier_add(type='SHRINKWRAP')
path_obj.modifiers[0].target = target_model
path_obj.modifiers[0].vertex_group = 'Group'
bpy.ops.object.modifier_apply(modifier=path_obj.modifiers[0].name,apply_as='DATA')
bpy.ops.transform.translate(value=(0,0,1.0))

# boolean it
bpy.context.scene.objects.active = target_model
bpy.ops.object.modifier_add(type='BOOLEAN')
engrave = target_model.modifiers[len(target_model.modifiers)-1]
engrave.object=path_obj

path_obj.hide=True

bpy.ops.wm.save_mainfile(filepath=os.environ['OUTPUT'])
