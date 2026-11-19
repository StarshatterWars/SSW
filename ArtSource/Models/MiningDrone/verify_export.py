import bpy,json
from mathutils import Vector
from pathlib import Path
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
bpy.ops.import_scene.fbx(filepath=r'D:\SSW\ArtSource\Models\MiningDrone\MiningDrone.fbx')
p=[o.matrix_world@Vector(c) for o in bpy.context.scene.objects if o.type=='MESH' for c in o.bound_box]
d=[max(v[i] for v in p)-min(v[i] for v in p) for i in range(3)]
r={'roundtrip_dimensions_m':d,'expected_ue_dimensions_cm':[x*100 for x in d],'axes':'+X forward +Z up','beam_socket_m':list(bpy.data.objects['SOCKET_MiningBeam'].matrix_world.translation)}
Path(r'D:\SSW\ArtSource\Models\MiningDrone\fbx_roundtrip.json').write_text(json.dumps(r,indent=2));print(json.dumps(r))
