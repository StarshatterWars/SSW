"""Non-destructive mining-drone adaptation. Blender4.1 background; optional --render for previews."""
import bpy,math,json,sys,shutil
from pathlib import Path
from mathutils import Vector
ROOT=Path(r'D:\SSW'); OUT=ROOT/'ArtSource/Models/MiningDrone';OUT.mkdir(parents=True,exist_ok=True)
SOURCE=ROOT/'ArtSource/Assets/Drone/targetdrone.fbx'
bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
bpy.ops.import_scene.fbx(filepath=str(SOURCE))
hull=next(o for o in bpy.context.scene.objects if o.type=='MESH');hull.name='MiningDrone_Hull'
bpy.context.view_layer.objects.active=hull;hull.select_set(True)
bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
# Retain original UVs/material assignments; repoint existing texture nodes to copied source files.
texdir=OUT/'Textures';texdir.mkdir(exist_ok=True)
for name in ['Black','Metal','Plate']:
 shutil.copy2(ROOT/'ArtSource/Assets/Drone'/f'{name}.png',texdir/f'{name}.png')
 mat=bpy.data.materials.get(name)
 if mat:
  mat.use_nodes=True;bsdf=mat.node_tree.nodes.get('Principled BSDF')
  image=bpy.data.images.load(str(texdir/f'{name}.png'),check_existing=True)
  nodes=[n for n in mat.node_tree.nodes if n.type=='TEX_IMAGE']
  node=nodes[0] if nodes else mat.node_tree.nodes.new('ShaderNodeTexImage');node.image=image
  mat.node_tree.links.new(node.outputs['Color'],bsdf.inputs['Base Color']);bsdf.inputs['Metallic'].default_value=.65;bsdf.inputs['Roughness'].default_value=.43
  for stale in list(mat.node_tree.nodes):
   if stale.type=='TEX_IMAGE' and stale!=node:mat.node_tree.nodes.remove(stale)
center=sum((Vector(c) for c in hull.bound_box),Vector())/8
for v in hull.data.vertices:v.co-=center
hull.rotation_euler.z=-math.pi/2;scale=2.8/8.134843826293945;hull.scale=(scale,)*3
bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
# Weld the source FBX's UV-split duplicate vertices without changing loop UVs.
import bmesh
bm=bmesh.new();bm.from_mesh(hull.data);bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=.00001);bm.to_mesh(hull.data);bm.free()
def material(name,color,metal=.5,rough=.4,emission=0):
 m=bpy.data.materials.new(name);m.diffuse_color=(*color,1);m.use_nodes=True;p=m.node_tree.nodes.get('Principled BSDF');p.inputs['Base Color'].default_value=(*color,1);p.inputs['Metallic'].default_value=metal;p.inputs['Roughness'].default_value=rough
 if emission:p.inputs['Emission Color'].default_value=(*color,1);p.inputs['Emission Strength'].default_value=emission
 return m
steel=material('MiningDrone_Titanium',(.15,.19,.22),.8,.3);dark=material('MiningDrone_Ceramic',(.025,.037,.045),.2,.6);amber=material('MiningDrone_SafetyAmber',(.85,.41,.045),.35,.4);cyan=material('MiningDrone_Emitter',(.04,.75,.9),.3,.2,2.5)
def cube(name,loc,dim,mat,bevel=.025):
 bpy.ops.mesh.primitive_cube_add(size=1,location=loc);o=bpy.context.object;o.name=name;o.dimensions=dim;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True);o.data.materials.append(mat)
 if bevel:m=o.modifiers.new('MachinedEdges','BEVEL');m.width=bevel;m.segments=2;o.modifiers.new('WeightedNormals','WEIGHTED_NORMAL')
 return o
def cyl(name,loc,radius,depth,mat,vertices=16):
 bpy.ops.mesh.primitive_cylinder_add(vertices=vertices,radius=radius,depth=depth,location=loc,rotation=(0,math.pi/2,0));o=bpy.context.object;o.name=name;o.data.materials.append(mat);m=o.modifiers.new('EdgeBreak','BEVEL');m.width=.012;m.segments=2;o.modifiers.new('WeightedNormals','WEIGHTED_NORMAL');return o
# Offset cutter modules visually distinguish this industrial variant from the original target drone.
for side in [-1,1]:
 cube(f'CutterShoulder_{side}',(.35,side*.58,-.08),(.55,.18,.23),steel)
 cube(f'CutterBoom_{side}',(.85,side*.79,-.10),(1.15,.18,.18),amber)
 cube(f'CutterRail_{side}',(1.0,side*.79,-.205),(.75,.12,.06),dark,.01)
 cyl(f'CutterHousing_{side}',(1.43,side*.79,-.10),.16,.40,steel)
 cyl(f'CutterCollar_{side}',(1.64,side*.79,-.10),.18,.08,amber)
 cyl(f'CutterLens_{side}',(1.69,side*.79,-.10),.115,.025,cyan)
 cyl(f'RearThruster_{side}',(-1.30,side*.42,.05),.14,.32,dark)
 cyl(f'RearNozzle_{side}',(-1.47,side*.42,.05),.10,.025,cyan)
 cube(f'CargoClamp_{side}',(-.25,side*.57,-.31),(.65,.13,.12),steel)
cube('CutterCrossbrace',(.35,0,-.08),(.35,1.65,.18),steel)
cube('TailCrossbrace',(-1.13,0,.05),(.32,1.0,.14),steel)
for side in [-1,1]:cube(f'CargoHanger_{side}',(-.25,side*.50,-.15),(.25,.14,.38),steel,.012)
cube('TopPanelMount',(-.05,0,.25),(.30,.25,.22),dark,.012)
# Central beam emitter is the runtime mining-beam attachment, no baked world beam.
cube('BeamBridge',(1.10,0,.02),(.50,.42,.25),dark)
cyl('MainEmitterHousing',(1.39,0,.02),.17,.30,steel)
cyl('MainEmitterLens',(1.555,0,.02),.12,.025,cyan)
cube('TopServicePanel',(-.05,0,.34),(.65,.4,.075),amber,.012)
def socket(name,loc,direction):
 o=bpy.data.objects.new(name,None);bpy.context.collection.objects.link(o);o.location=loc;o.empty_display_type='ARROWS';o.empty_display_size=.18;o['direction_local']=direction;return o
socket('SOCKET_MiningBeam',(1.58,0,.02),[1,0,0])
socket('SOCKET_CutterLeft',(1.72,-.79,-.10),[1,0,0]);socket('SOCKET_CutterRight',(1.72,.79,-.10),[1,0,0])
socket('SOCKET_EngineLeft',(-1.50,-.42,.05),[-1,0,0]);socket('SOCKET_EngineRight',(-1.50,.42,.05),[-1,0,0])
scene=bpy.context.scene;scene.unit_settings.system='METRIC';scene.unit_settings.scale_length=1
scene['asset_name']='SSW Mining Drone';scene['forward_axis']='+X';scene['up_axis']='+Z';scene['source_fbx']=str(SOURCE)
meshes=[o for o in scene.objects if o.type=='MESH']
# Apply bevels then triangulate exported geometry so counts and preview match.
for o in meshes:
 bpy.context.view_layer.objects.active=o
 for mod in list(o.modifiers):
  try:bpy.ops.object.modifier_apply(modifier=mod.name)
  except RuntimeError:pass
 tri=o.modifiers.new('ExportTriangulation','TRIANGULATE');bpy.ops.object.modifier_apply(modifier=tri.name)
 o.data.calc_loop_triangles()
points=[o.matrix_world@Vector(c) for o in meshes for c in o.bound_box];mins=[min(p[i] for p in points) for i in range(3)];maxs=[max(p[i] for p in points) for i in range(3)]
meta={'source':str(SOURCE),'prototype':True,'units':'meters','forward':'+X','up':'+Z','dimensions_m':[maxs[i]-mins[i] for i in range(3)],'bounds_min_m':mins,'bounds_max_m':maxs,'triangles':sum(len(o.data.loop_triangles) for o in meshes),'vertices':sum(len(o.data.vertices) for o in meshes),'materials':sorted({m.name for o in meshes for m in o.data.materials if m}),'sockets':{o.name:{'location_m':list(o.location),'forward':list(o['direction_local'])} for o in scene.objects if o.type=='EMPTY'}}
(OUT/'MiningDrone.metadata.json').write_text(json.dumps(meta,indent=2))
bpy.ops.object.select_all(action='DESELECT')
for o in scene.objects:o.select_set(o.type in {'MESH','EMPTY'})
bpy.ops.export_scene.fbx(filepath=str(OUT/'MiningDrone.fbx'),use_selection=True,object_types={'MESH','EMPTY'},global_scale=1,apply_unit_scale=True,axis_forward='X',axis_up='Z',use_mesh_modifiers=True,mesh_smooth_type='FACE',add_leaf_bones=False,path_mode='COPY',embed_textures=True)
# Numeric geometry check uses floating-object mode for this flying asset.
sys.path.insert(0,r'C:\Users\Carlo\.codex\skills\blender-modeler\scripts')
import check_geometry
report=check_geometry.run(spec_path=str(OUT/'spec.json'))
(OUT/'geometry_check.json').write_text(json.dumps(report,indent=2,default=str))
# Pack Blender textures; source file stays untouched.

for image in list(bpy.data.images):
 if image.users==0:bpy.data.images.remove(image)
bpy.ops.file.pack_all();bpy.ops.wm.save_as_mainfile(filepath=str(OUT/'MiningDrone.blend'))
print('MINING_DRONE_METADATA',json.dumps(meta))
if '--render' in sys.argv:
 scene.render.engine='BLENDER_EEVEE';scene.render.resolution_x=480;scene.render.resolution_y=360;scene.render.resolution_percentage=100
 scene.world.color=(.08,.08,.08);scene.view_settings.view_transform='AgX';scene.render.image_settings.file_format='PNG';scene.render.film_transparent=False
 for loc,energy,size in [((3,-4,6),950,5),((-3,2,3),700,4),((0,4,1),450,3)]:
  bpy.ops.object.light_add(type='AREA',location=loc);o=bpy.context.object;o.data.energy=energy;o.data.shape='DISK';o.data.size=size;o.rotation_euler=(Vector((0,0,0))-o.location).to_track_quat('-Z','Y').to_euler()
 bpy.ops.object.camera_add();cam=bpy.context.object;scene.camera=cam;cam.data.type='ORTHO';cam.data.ortho_scale=4.7
 for name,loc in [('Perspective',(4,-5,3)),('Top',(0,0,7)),('Side',(0,-7,0)),('Front',(7,0,0))]:
  cam.location=loc;cam.rotation_euler=(Vector((.1,0,0))-cam.location).to_track_quat('-Z','Y').to_euler();scene.render.filepath=str(OUT/f'Preview_{name}.png');bpy.ops.render.render(write_still=True)
