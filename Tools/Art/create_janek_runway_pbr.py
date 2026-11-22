"""Run with UnrealEditor-Cmd -run=pythonscript -script=<this file>.
Builds a dedicated airport PBR material; leaves the original atlas/material intact.
The atlas's right-hand strip is runway, so vector paint retains the original UVs.
All physical detail uses metre-sized world coordinates, not a stretched atlas.
"""
import unreal

PATH = "/Game/Assets/Ground/Airbases"
NAME = "M_JanekRunway_PBR"
lib = unreal.MaterialEditingLibrary
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
material = unreal.load_asset(PATH + "/" + NAME) if unreal.EditorAssetLibrary.does_asset_exist(PATH + "/" + NAME) else None
if material is None:
    material = asset_tools.create_asset(NAME, PATH, unreal.Material, unreal.MaterialFactoryNew())
material.set_editor_property("used_with_nanite", True)
lib.delete_all_material_expressions(material)

def custom_input(name):
    item = unreal.CustomInput()
    item.set_editor_property("input_name", name)
    return item

def node(cls, x, y):
    return lib.create_material_expression(material, cls, x, y)
def connect(a, output, b, pin):
    if not lib.connect_material_expressions(a, output, b, pin):
        raise RuntimeError("Could not connect " + pin)
def property_link(a, output, prop):
    if not lib.connect_material_property(a, output, prop):
        raise RuntimeError("Could not connect material property")

uv = node(unreal.MaterialExpressionTextureCoordinate, -1000, 0)
position = node(unreal.MaterialExpressionWorldPosition, -1000, 180)
local_position = node(unreal.MaterialExpressionTransformPosition, -820, 350)
local_position.set_editor_property("transform_source_type", unreal.MaterialPositionTransformSource.TRANSFORMPOSSOURCE_WORLD)
local_position.set_editor_property("transform_type", unreal.MaterialPositionTransformSource.TRANSFORMPOSSOURCE_LOCAL)
connect(position,"",local_position,"")
atlas = node(unreal.MaterialExpressionTextureSample, -1000, -220)
atlas.set_editor_property("texture", unreal.load_asset(PATH + "/T_Airbase_D"))
connect(uv, "", atlas, "UVs")
color = node(unreal.MaterialExpressionCustom, -550, -120)
color.set_editor_property("description", "Airport asphalt, concrete and anti-aliased vector runway paint")
color.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT4)
color.set_editor_property("inputs", [custom_input("UV"), custom_input("P"), custom_input("Atlas"),custom_input("Local")])
color.set_editor_property("code", r"""
struct RunwayLettering
{
    float Segment(float2 p,float2 a,float2 b,float w)
    {
        float2 v=b-a;
        float d=length(p-a-v*saturate(dot(p-a,v)/dot(v,v)));
        float aa=max(fwidth(d),0.002);
        return 1-smoothstep(w-aa,w+aa,d);
    }
    float Digit(float2 p,int n)
    {
        float paint=0;
        if(n!=1) paint=max(paint,Segment(p,float2(.12,.92),float2(.88,.92),.065));
        if(n!=1 && n!=7 && n!=0) paint=max(paint,Segment(p,float2(.12,.5),float2(.88,.5),.065));
        if(n!=1 && n!=7) paint=max(paint,Segment(p,float2(.12,.08),float2(.88,.08),.065));
        if(n==0 || n==9) paint=max(paint,Segment(p,float2(.12,.52),float2(.12,.9),.065));
        if(n==0 || n==2) paint=max(paint,Segment(p,float2(.12,.1),float2(.12,.48),.065));
        paint=max(paint,Segment(p,float2(.88,.52),float2(.88,.9),.065));
        if(n!=2) paint=max(paint,Segment(p,float2(.88,.1),float2(.88,.48),.065));
        return paint;
    }
    float Number(float2 p,float2 c,float2 forward,int left,int right)
    {
        float2 delta=p-c;
        float2 across=float2(-forward.y,forward.x);
        float2 text=float2(dot(delta,across)/12,dot(delta,forward)/25+.5);
        return max(Digit(float2(text.x+1.2,text.y),left),Digit(float2(text.x-.2,text.y),right));
    }
};
RunwayLettering lettering;
// Base1 deck coordinates are metres in the design and quarter-size in the mesh.
float numbers=lettering.Number(Local.xy,float2(0,550),float2(0,-1),2,7);
numbers=max(numbers,lettering.Number(Local.xy,float2(0,-550),float2(0,1),0,9));
numbers=max(numbers,lettering.Number(Local.xy,float2(263.6,455.6),normalize(float2(-1,-1)),3,1));
numbers=max(numbers,lettering.Number(Local.xy,float2(-648.6,-456.6),normalize(float2(1,1)),1,3));
float2 q = frac(UV);
float aa = max(fwidth(q.x), 0.000015);
float runway = smoothstep(0.830-aa,0.830+aa,q.x);
float across = saturate((q.x-0.830)/0.170);
float ax = max(fwidth(across), 0.0001);
float av = max(fwidth(q.y), 0.000015);
float center = 1-smoothstep(0.005-ax,0.005+ax,abs(across-0.5));
float dash = 1-smoothstep(0.44-av*40,0.44+av*40,frac(q.y*40));
float edge = 1-smoothstep(0.003-ax,0.003+ax,min(abs(across-0.06),abs(across-0.94)));
float threshold = (1-smoothstep(0.055-av,0.055+av,q.y))*smoothstep(0.015-av,0.015+av,q.y);
float bars = 1-smoothstep(0.45-ax*14,0.45+ax*14,frac(across*14));
float paint = max(saturate(center*dash+edge+threshold*bars)*runway,numbers*runway);
float footprint = max(length(ddx(P)),length(ddy(P)));
float detailFade = 1/(1+footprint*3);
float grain = sin(P.x*34.1+P.y*27.3)*sin(P.y*61.7-P.x*13.2)*detailFade;
float2 slab = abs(frac(P.xy/6)-0.5)*6;
float jointDistance = min(slab.x,slab.y);
float jointAA = max(fwidth(jointDistance),0.003);
float joints = 1-smoothstep(0.018-jointAA,0.018+jointAA,jointDistance);
float3 concrete = lerp(float3(0.22,0.225,0.23),Atlas,0.18)*(1-joints*0.17)+grain*0.008;
float tire = exp(-pow((across-0.39)/0.025,2))+exp(-pow((across-0.61)/0.025,2));
float3 asphalt = float3(0.055,0.060,0.067)+grain*0.005-tire*0.008;
float3 base = lerp(concrete,asphalt,runway);
base = lerp(base,float3(0.68,0.69,0.66),paint);
return float4(max(base,0.008),lerp(lerp(0.82,0.92,runway),0.64,paint));
""")
connect(uv,"",color,"UV");connect(position,"",color,"P");connect(atlas,"RGB",color,"Atlas");connect(local_position,"",color,"Local")
base = node(unreal.MaterialExpressionComponentMask, -200, -150)
base.set_editor_property("r",True);base.set_editor_property("g",True);base.set_editor_property("b",True)
connect(color,"",base,"");property_link(base,"",unreal.MaterialProperty.MP_BASE_COLOR)
rough = node(unreal.MaterialExpressionComponentMask, -200, 40)
rough.set_editor_property("r",False);rough.set_editor_property("a",True)
connect(color,"",rough,"");property_link(rough,"",unreal.MaterialProperty.MP_ROUGHNESS)
normal = node(unreal.MaterialExpressionCustom, -550, 220)
normal.set_editor_property("description","Fine aggregate normal, filtered with pixel footprint")
normal.set_editor_property("output_type",unreal.CustomMaterialOutputType.CMOT_FLOAT3)
normal.set_editor_property("inputs",[custom_input("P")])
normal.set_editor_property("code","float fade=1/(1+max(length(ddx(P)),length(ddy(P)))*5); return normalize(float3(cos(P.x*34.1+P.y*27.3)*0.055*fade,sin(P.y*61.7-P.x*13.2)*0.055*fade,1));")
connect(position,"",normal,"P");property_link(normal,"",unreal.MaterialProperty.MP_NORMAL)
for value,prop,y in [(0.0,unreal.MaterialProperty.MP_METALLIC,420),(0.28,unreal.MaterialProperty.MP_SPECULAR,520)]:
    constant=node(unreal.MaterialExpressionConstant,-200,y);constant.set_editor_property("r",value);property_link(constant,"",prop)
lib.layout_material_expressions(material)
lib.recompile_material(material)
unreal.EditorAssetLibrary.save_loaded_asset(material)
unreal.log("Janek dedicated runway PBR material saved: " + material.get_path_name())


