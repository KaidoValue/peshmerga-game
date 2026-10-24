# -*- coding: utf-8 -*-
"""
Gibt der Landschaft ein Material: Gras auf flachen Stellen, steinige Erde
dazwischen, Fels an steilen Haengen.

Im Editor: Werkzeuge -> Python-Skript ausfuehren... -> Scripts/setup_landscape.py

Das Skript sucht unter /Game nach Oberflaechen-Texturen (z. B. Megascans aus
Fab). Ordner- oder Dateinamen mit grass/meadow/moss zaehlen als Gras,
soil/ground/dirt/gravel/mud als Erde und rock/cliff/stone als Fels. Fehlt
eine Sorte, nimmt es eine passende Farbe, damit der Boden trotzdem nicht
mehr grau ist. Nach dem Holen neuer Texturen einfach erneut ausfuehren.
"""
import unreal

MAT_DIR = "/Game/Materials"
MAT_NAME = "M_Qandil_Landschaft"

KINDS = {
    "gras": ("grass", "meadow", "moss", "lawn"),
    "erde": ("soil", "ground", "dirt", "gravel", "mud", "earth"),
    "fels": ("rock", "cliff", "stone", "boulder"),
}
# Ersatzfarben (linear), falls keine Textur gefunden wird
FALLBACK = {
    "gras": (0.10, 0.13, 0.05),
    "erde": (0.17, 0.13, 0.08),
    "fels": (0.26, 0.25, 0.23),
}
# Kachelgroesse in Landschafts-Quads (1 Quad = ca. 4 m)
TILING = {"gras": 1.0, "erde": 1.0, "fels": 2.5}

mel = unreal.MaterialEditingLibrary
assets = unreal.EditorAssetLibrary
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)


def log(msg):
    unreal.log("[Landschaft] " + msg)


# ------------------------------------------------------------------ Texturen finden

def class_name(data):
    try:
        return str(data.asset_class_path.asset_name)
    except Exception:
        return str(data.asset_class)


def is_color(name):
    n = name.lower()
    return n.endswith("_b") or n.endswith("_d") or any(k in n for k in ("basecolor", "base_color", "albedo", "diffuse"))


def is_normal(name):
    n = name.lower()
    return n.endswith("_n") or "normal" in n


registry = unreal.AssetRegistryHelpers.get_asset_registry()
found = {k: {} for k in KINDS}
for data in registry.get_assets_by_path("/Game", recursive=True):
    if class_name(data) != "Texture2D":
        continue
    path = str(data.package_name)
    if path.startswith(MAT_DIR):
        continue
    low = path.lower()
    kind = next((k for k, words in KINDS.items() if any(w in low for w in words)), None)
    if kind is None:
        continue
    name = str(data.asset_name)
    slot = "color" if is_color(name) else "normal" if is_normal(name) else None
    if slot and slot not in found[kind]:
        found[kind][slot] = path

for kind in KINDS:
    log("%s: Farbe=%s, Normal=%s" % (kind, found[kind].get("color", "-"), found[kind].get("normal", "-")))


# ------------------------------------------------------------------ Material bauen

path = MAT_DIR + "/" + MAT_NAME
if assets.does_asset_exist(path):
    mat = unreal.load_asset(path)
    mel.delete_all_material_expressions(mat)
else:
    mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        MAT_NAME, MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())


def node(cls, x, y, **props):
    e = mel.create_material_expression(mat, cls, x, y)
    for k, v in props.items():
        e.set_editor_property(k, v)
    return e


def link(a, b, pin, out=""):
    mel.connect_material_expressions(a, out, b, pin)


def const(v, x, y):
    return node(unreal.MaterialExpressionConstant, x, y, r=v)


def lerp(a, b, alpha, x, y):
    e = node(unreal.MaterialExpressionLinearInterpolate, x, y)
    link(a, e, "A")
    link(b, e, "B")
    link(alpha, e, "Alpha")
    return e


def sharpen(src, offset, gain, x, y):
    """saturate((src - offset) * gain)"""
    sub = node(unreal.MaterialExpressionSubtract, x, y)
    link(src, sub, "A")
    link(const(offset, x - 150, y + 60), sub, "B")
    mul = node(unreal.MaterialExpressionMultiply, x + 150, y)
    link(sub, mul, "A")
    link(const(gain, x, y + 60), mul, "B")
    sat = node(unreal.MaterialExpressionSaturate, x + 300, y)
    link(mul, sat, "")
    return sat


colors, normals = {}, {}
for i, kind in enumerate(KINDS):
    y = -900 + i * 500
    uv = node(unreal.MaterialExpressionLandscapeLayerCoords, -1700, y, mapping_scale=TILING[kind])
    tex = found[kind]
    if "color" in tex:
        c = node(unreal.MaterialExpressionTextureSample, -1400, y, texture=unreal.load_asset(tex["color"]))
        link(uv, c, "UVs")
        colors[kind] = c
    else:
        r, g, b = FALLBACK[kind]
        colors[kind] = node(unreal.MaterialExpressionConstant3Vector, -1400, y, constant=unreal.LinearColor(r, g, b, 1.0))
    if "normal" in tex:
        n = node(unreal.MaterialExpressionTextureSample, -1400, y + 220,
                 texture=unreal.load_asset(tex["normal"]), sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
        link(uv, n, "UVs")
        normals[kind] = n

# Flecken aus Erde im Gras (grosses Rauschen in Weltkoordinaten)
patch_noise = node(unreal.MaterialExpressionNoise, -1100, 700, scale=0.0004, levels=4, output_min=0.0, output_max=1.0)
patches = sharpen(patch_noise, 0.55, 5.0, -900, 700)

# Steilheit: Normalen-Z unter ca. 0.8 wird Fels
vnormal = node(unreal.MaterialExpressionVertexNormalWS, -1100, 1000)
up = node(unreal.MaterialExpressionComponentMask, -950, 1000, r=False, g=False, b=True, a=False)
link(vnormal, up, "")
flat = sharpen(up, 0.72, 7.0, -800, 1000)
steep = node(unreal.MaterialExpressionOneMinus, -350, 1000)
link(flat, steep, "")

ground = lerp(colors["gras"], colors["erde"], patches, -500, -600)
base = lerp(ground, colors["fels"], steep, -250, -400)

# leichte Helligkeitsschwankung gegen sichtbare Kachelung
tint_noise = node(unreal.MaterialExpressionNoise, -700, 1300, scale=0.0015, levels=3, output_min=0.8, output_max=1.15)
tinted = node(unreal.MaterialExpressionMultiply, -50, -400)
link(base, tinted, "A")
link(tint_noise, tinted, "B")
mel.connect_material_property(tinted, "", unreal.MaterialProperty.MP_BASE_COLOR)

mel.connect_material_property(const(0.9, -50, -200), "", unreal.MaterialProperty.MP_ROUGHNESS)

if normals:
    flat_n = node(unreal.MaterialExpressionConstant3Vector, -1400, 1500, constant=unreal.LinearColor(0, 0, 1, 1))
    n_gras = normals.get("gras", flat_n)
    n_erde = normals.get("erde", flat_n)
    n_fels = normals.get("fels", flat_n)
    n_ground = lerp(n_gras, n_erde, patches, -500, 300)
    n_base = lerp(n_ground, n_fels, steep, -250, 300)
    mel.connect_material_property(n_base, "", unreal.MaterialProperty.MP_NORMAL)

mel.layout_material_expressions(mat)
mel.recompile_material(mat)
assets.save_asset(path)
log("Material gespeichert: " + path)


# ------------------------------------------------------------------ zuweisen

count = 0
for a in actors.get_all_level_actors():
    if isinstance(a, unreal.LandscapeProxy):
        a.set_editor_property("landscape_material", mat)
        count += 1
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()

missing = [k for k in KINDS if "color" not in found[k]]
msg = "Material auf %d Landschaft(en) gesetzt." % count
if missing:
    msg += "\n\nKeine Textur gefunden fuer: %s.\nDort wird erstmal eine Farbe benutzt." % ", ".join(missing)
unreal.EditorDialog.show_message("Landschaft", msg, unreal.AppMsgType.OK)
