
import bpy
import os

# imports a model and selects it
def from_file(file_path):
    extension = os.path.splitext(file_path)[-1][1:]
    print('Loading %s with extension %s' % (file_path, extension))
    
    if extension == 'blend':
        bpy.ops.wm.open_mainfile(filepath=file_path)
        bpy.ops.object.select_all(action='SELECT')
        return bpy.context.scene.objects[0]
    elif extension == 'stl':
        bpy.ops.import_mesh.stl(filepath=file_path)
    elif extension == 'obj':
        bpy.ops.import_scene.obj(filepath=file_path)
    elif extension == 'ply':
        bpy.ops.import_mesh.ply(filepath=file_path)
    elif extension == '3ds':
        bpy.ops.import_scene.autodesk_3ds(filepath=file_path, constrain_size=0)
    elif extension == 'obj':
        bpy.ops.import_scene.obj(filepath=file_path)
    elif extension == 'dae':
        bpy.ops.wm.collada_import(filepath=file_path)
    else:
        raise Exception('unknown extension: ' + extension)
    
    bpy.context.scene.objects.active = bpy.context.selected_objects[0]
    bpy.ops.object.join()
    ob = bpy.context.selected_objects[0]

    bpy.ops.object.select_all(action='DESELECT')
    ob.select = True

    return ob

