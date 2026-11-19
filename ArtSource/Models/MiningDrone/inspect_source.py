import bpy,json
from pathlib import Path
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
bpy.ops.import_scene.fbx(filepath=r'D:\SSW\ArtSource\Assets\Drone\targetdrone.fbx')
info=[]
for o in bpy.context.scene.objects:
 if o.type=='MESH':
  o.data.calc_loop_triangles()
  info.append({'name':o.name,'bounds':[list(o.matrix_world @ __import__('mathutils').Vector(c)) for c in o.bound_box],'dimensions':list(o.dimensions),'rotation':list(o.rotation_euler),'vertices':len(o.data.vertices),'triangles':len(o.data.loop_triangles),'materials':[m.name if m else None for m in o.data.materials]})
Path(r'D:\SSW\ArtSource\Models\MiningDrone\source_inspection.json').write_text(json.dumps(info,indent=2))
print(json.dumps(info))
