"""Run in UE Editor Python; creates only the SSW-owned radial shield-impact material.
Original legacy/pack materials and shield textures are never modified.
"""
import unreal

DIRECTORY='/Game/Effects/Shields'
NAME='M_SSW_ShieldImpact_Radial'
PATH=DIRECTORY+'/'+NAME
unreal.EditorAssetLibrary.make_directory(DIRECTORY)
material=unreal.load_asset(PATH)
if material is None:
    material=unreal.AssetToolsHelpers.get_asset_tools().create_asset(NAME,DIRECTORY,unreal.Material,unreal.MaterialFactoryNew())
if not isinstance(material,unreal.Material):
    raise RuntimeError('Owned shield-impact path is not a Material: '+PATH)
edit=unreal.MaterialEditingLibrary
edit.delete_all_material_expressions(material)
material.set_editor_property('blend_mode',unreal.BlendMode.BLEND_ADDITIVE)
material.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
material.set_editor_property('two_sided',True)
def node(cls,x,y):return edit.create_material_expression(material,cls,x,y)
def scalar(value,x,y):
    n=node(unreal.MaterialExpressionConstant,x,y);n.set_editor_property('r',value);return n
def wire(source,target,pin=''):
    if not edit.connect_material_expressions(source,'',target,pin):raise RuntimeError('Cannot connect material node '+str(target))
uv=node(unreal.MaterialExpressionTextureCoordinate,-1200,0)
center=node(unreal.MaterialExpressionConstant2Vector,-1200,150);center.set_editor_property('r',.5);center.set_editor_property('g',.5)
offset=node(unreal.MaterialExpressionSubtract,-1000,0);wire(uv,offset,'A');wire(center,offset,'B')
dot=node(unreal.MaterialExpressionDotProduct,-800,0);wire(offset,dot,'A');wire(offset,dot,'B')
distance=node(unreal.MaterialExpressionSquareRoot,-600,0);wire(dot,distance)
# Preserve the noisy center; fade continuously to zero at the inscribed UV circle.
start=node(unreal.MaterialExpressionSubtract,-400,0);wire(distance,start,'A');wire(scalar(.18,-600,160),start,'B')
width=node(unreal.MaterialExpressionDivide,-200,0);wire(start,width,'A');wire(scalar(.32,-400,160),width,'B')
clamp=node(unreal.MaterialExpressionSaturate,0,0);wire(width,clamp)
edge=node(unreal.MaterialExpressionOneMinus,180,0);wire(clamp,edge)
soft=node(unreal.MaterialExpressionMultiply,360,0);wire(edge,soft,'A');wire(edge,soft,'B')
fade=node(unreal.MaterialExpressionScalarParameter,180,220);fade.set_editor_property('parameter_name','Opacity');fade.set_editor_property('default_value',1.0)
mask=node(unreal.MaterialExpressionMultiply,540,0);wire(soft,mask,'A');wire(fade,mask,'B')
texture=node(unreal.MaterialExpressionTextureSampleParameter2D,180,-300);texture.set_editor_property('parameter_name','ShieldTexture')
source=unreal.load_asset('/Game/Models/Orion/T_ShieldImpact')
if not source:raise RuntimeError('Existing shield-impact texture not found; no substitute texture authored')
texture.set_editor_property('texture',source);wire(uv,texture,'UVs')
emissive=node(unreal.MaterialExpressionMultiply,750,-150);wire(texture,emissive,'A');wire(mask,emissive,'B')
if not edit.connect_material_property(emissive,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR):raise RuntimeError('Emissive connection failed')
if not edit.connect_material_property(mask,'',unreal.MaterialProperty.MP_OPACITY):raise RuntimeError('Opacity connection failed')
edit.recompile_material(material)
if not unreal.EditorAssetLibrary.save_loaded_asset(material,only_if_is_dirty=False):raise RuntimeError('Owned shield material could not be saved')
unreal.log('SSW radial shield material saved: '+PATH+'; center radius .18, outer radius .50, squared soft falloff; Opacity and ShieldTexture parameters retained.')
