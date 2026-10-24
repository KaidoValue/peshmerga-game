# -*- coding: utf-8 -*-
"""
Baut Mission 01 "Feuer ueber Qandil" in die geoeffnete Karte.

Voraussetzung: Die Landschaft Qandil_Berfin ist schon importiert (siehe README).
Im Editor: Datei -> Python-Skript ausfuehren... -> Scripts/setup_m01.py

Das Skript
  - importiert die Dialoge (Deutsch/Sorani) als Datentabellen nach /Game/Data,
  - legt Himmel, Sonne, Nebel und Wolken an, falls sie fehlen,
  - setzt Lager, Huetten, Figuren, Hoehle, Felsdaecher, Drohne, Wache und
    alle Trigger als Platzhalter auf das echte Gelaende,
  - verbindet alles mit dem PGMissionDirector und speichert die Karte.

Es kann mehrfach ausgefuehrt werden: Alles, was es selbst gesetzt hat
(Tag "PG_M01"), wird vorher entfernt.

Koordinaten in Metern wie im Browser-Prototyp: x nach Osten, z nach Norden,
Ursprung = Mitte der Landschaft (Lager Berfin).
"""
import os
import unreal

TAG = "PG_M01"
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()

CUBE = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
CYLINDER = unreal.load_asset("/Engine/BasicShapes/Cylinder.Cylinder")
SPHERE = unreal.load_asset("/Engine/BasicShapes/Sphere.Sphere")


def log(msg):
    unreal.log("[Mission01] " + msg)


# ------------------------------------------------------------------ Gelaende

landscape = None
for a in actors.get_all_level_actors():
    if isinstance(a, unreal.LandscapeProxy):
        landscape = a
        break
if landscape is None:
    unreal.EditorDialog.show_message(
        "Mission 01",
        "Keine Landschaft gefunden.\n\nBitte zuerst RawData/Heightmaps/Qandil_Berfin.png "
        "als Landschaft importieren (siehe README), dann das Skript erneut ausfuehren.",
        unreal.AppMsgType.OK)
    raise SystemExit

origin, extent = landscape.get_actor_bounds(False)
CX, CY = origin.x, origin.y
TOP = origin.z + extent.z + 5000.0
BOTTOM = origin.z - extent.z - 5000.0


def hit_location(hit):
    for name in ("impact_point", "location"):
        try:
            return hit.get_editor_property(name)
        except Exception:
            pass
    return hit.to_tuple()[4]


def ground(x, z):
    """Hoehe des Gelaendes bei x/z (Meter) als Weltposition in cm."""
    wx, wy = CX + x * 100.0, CY - z * 100.0
    hit = unreal.SystemLibrary.line_trace_single(
        world, unreal.Vector(wx, wy, TOP), unreal.Vector(wx, wy, BOTTOM),
        unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, True, [], unreal.DrawDebugTrace.NONE, True)
    if hit:
        p = hit_location(hit)
        return unreal.Vector(wx, wy, p.z)
    log("Kein Boden bei %.0f/%.0f gefunden" % (x, z))
    return unreal.Vector(wx, wy, origin.z)


def at(x, z, up=0.0):
    p = ground(x, z)
    return unreal.Vector(p.x, p.y, p.z + up)


def rot(yaw=0.0, pitch=0.0, roll=0.0):
    return unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw)


# ------------------------------------------------------------------ Aufraeumen

for a in actors.get_all_level_actors():
    if TAG in [str(t) for t in a.tags]:
        actors.destroy_actor(a)


def spawn(cls, loc, rotation=None, folder="Mission01", label=None):
    a = actors.spawn_actor_from_class(cls, loc, rotation or rot())
    a.set_editor_property("tags", [unreal.Name(TAG)])
    a.set_folder_path(folder)
    if label:
        a.set_actor_label(label)
    return a


def block(label, x, z, size, up=None, yaw=0.0, mesh=CUBE, folder="Mission01/Lager", movable=False, pitch=0.0):
    """Platzhalter-Mesh. size in Metern (Breite, Tiefe, Hoehe)."""
    sx, sy, sz = size
    a = spawn(unreal.StaticMeshActor, at(x, z, (sz * 50.0) if up is None else up), rot(yaw, pitch), folder, label)
    comp = a.get_editor_property("static_mesh_component")
    if movable:
        comp.set_mobility(unreal.ComponentMobility.MOVABLE)
    comp.set_static_mesh(mesh)
    a.set_actor_scale3d(unreal.Vector(sx, sy, sz))
    return a


def person(label, x, z, yaw=0.0, folder="Mission01/Figuren"):
    return block(label, x, z, (0.5, 0.5, 1.8), mesh=CYLINDER, yaw=yaw, folder=folder, movable=True)


def marker(label, x, z, up=100.0, yaw=0.0):
    return spawn(unreal.TargetPoint, at(x, z, up), rot(yaw), "Mission01/Punkte", label)


def interactable(label, x, z, kind, steps, mesh, size, up=None, yaw=0.0, pitch=0.0, folder="Mission01/Figuren"):
    sx, sy, sz = size
    a = spawn(unreal.PGInteractableActor, at(x, z, 0.0), rot(yaw), folder, label)
    a.set_editor_property("kind", kind)
    a.set_editor_property("active_steps", steps)
    m = a.get_editor_property("mesh")
    m.set_static_mesh(mesh)
    m.set_relative_scale3d(unreal.Vector(sx, sy, sz))
    m.set_relative_location(unreal.Vector(0, 0, (sz * 50.0) if up is None else up), False, False)
    m.set_relative_rotation(rot(0.0, pitch), False, False)
    return a


def trigger(label, cls, x, z, ext, trigger_id=None, up=None, yaw=0.0):
    ex, ey, ez = ext
    a = spawn(cls, at(x, z, ez * 100.0 if up is None else up), rot(yaw), "Mission01/Trigger", label)
    a.get_editor_property("box").set_box_extent(unreal.Vector(ex * 100.0, ey * 100.0, ez * 100.0), True)
    if trigger_id:
        a.set_editor_property("trigger_id", unreal.Name(trigger_id))
    return a


# ------------------------------------------------------------------ Datentabellen

def import_table(csv_name, asset_name, struct_path):
    path = os.path.join(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()), "RawData", "Dialogue", csv_name)
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", path)
    task.set_editor_property("destination_path", "/Game/Data")
    task.set_editor_property("destination_name", asset_name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    factory = unreal.CSVImportFactory()
    settings = factory.get_editor_property("automated_import_settings")
    settings.set_editor_property("import_row_struct", unreal.load_object(None, struct_path))
    settings.set_editor_property("import_type", unreal.CSVImportType.ECSV_DATA_TABLE)
    factory.set_editor_property("automated_import_settings", settings)
    task.set_editor_property("factory", factory)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    return unreal.load_asset("/Game/Data/%s.%s" % (asset_name, asset_name))


dialogue = import_table("DT_M01_Dialogue.csv", "DT_M01_Dialogue", "/Script/PeshmergaGame.PGDialogueLine")
uitext = import_table("DT_M01_UIText.csv", "DT_M01_UIText", "/Script/PeshmergaGame.PGUIText")
log("Datentabellen: %s, %s" % (dialogue, uitext))

# ------------------------------------------------------------------ Himmel und Licht

existing = actors.get_all_level_actors()


def find(cls):
    for a in existing:
        if isinstance(a, cls):
            return a
    return None


sun = find(unreal.DirectionalLight)
if sun is None:
    sun = spawn(unreal.DirectionalLight, unreal.Vector(CX, CY, TOP), rot(120.0, -32.0), "Licht", "Sonne")
    lc = sun.get_editor_property("light_component")
    lc.set_editor_property("atmosphere_sun_light", True)
    lc.set_editor_property("intensity", 9.0)
    lc.set_mobility(unreal.ComponentMobility.MOVABLE)
if find(unreal.SkyLight) is None:
    sky = spawn(unreal.SkyLight, unreal.Vector(CX, CY, TOP), rot(), "Licht", "Himmelslicht")
    sc = sky.get_editor_property("light_component")
    sc.set_mobility(unreal.ComponentMobility.MOVABLE)
    sc.set_editor_property("real_time_capture", True)
if find(unreal.SkyAtmosphere) is None:
    spawn(unreal.SkyAtmosphere, unreal.Vector(CX, CY, 0), rot(), "Licht", "Atmosphaere")
if find(unreal.VolumetricCloud) is None:
    spawn(unreal.VolumetricCloud, unreal.Vector(CX, CY, 0), rot(), "Licht", "Wolken")
if find(unreal.ExponentialHeightFog) is None:
    fog = spawn(unreal.ExponentialHeightFog, at(0, 0, 0), rot(), "Licht", "Hoehennebel")
    fog.get_editor_property("component").set_editor_property("enable_volumetric_fog", True)
# Die Licht-Actors sollen beim erneuten Ausfuehren bleiben.
for a in actors.get_all_level_actors():
    if str(a.get_folder_path()) == "Licht":
        a.set_editor_property("tags", [])

# ------------------------------------------------------------------ Lager Berfin

huts = [(-12, -10, 5, 4, 23), (13, -6, 6, 4.5, -17), (-4, 14, 5.5, 4, 191), (16, 12, 5, 4, 151)]
for i, (x, z, w, d, yaw) in enumerate(huts):
    block("Huette_%d" % (i + 1), x, z, (w, d, 2.4), yaw=yaw)
block("Feuerstelle", 3, -2, (1.2, 1.2, 0.25), mesh=CYLINDER)
fire = spawn(unreal.PointLight, at(3, -2, 60), rot(), "Mission01/Lager", "Feuerlicht")
fl = fire.get_editor_property("light_component")
fl.set_editor_property("light_color", unreal.Color(r=255, g=138, b=58, a=255))
fl.set_editor_property("intensity", 2500.0)
fl.set_editor_property("attenuation_radius", 900.0)
block("Kiste_Funk", -7.4, 10.1, (0.8, 0.5, 0.5))

# Hoehle im westlichen Hang: einfacher Felsbogen als Platzhalter
block("Hoehle_Dach", -31, 2, (7, 6, 1.2), up=330, yaw=8)
block("Hoehle_Wand_N", -31, 5, (7, 1, 3.2), yaw=8)
block("Hoehle_Wand_S", -31, -1, (7, 1, 3.2), yaw=8)
block("Hoehle_Rueckwand", -34.5, 2, (1, 6, 3.2), yaw=8)

# ------------------------------------------------------------------ Figuren

spring = interactable("Quelle", 17, 22, unreal.PGInteractKind.WATER, [unreal.PGMissionStep.WATER], CYLINDER, (3, 3, 0.2), folder="Mission01/Lager")
dara = interactable("Dara", 4.5, -4.2, unreal.PGInteractKind.TALK, [unreal.PGMissionStep.TEA, unreal.PGMissionStep.RETURN], CYLINDER, (0.5, 0.5, 1.75), yaw=-137)
azad = interactable("Azad", -7, 9, unreal.PGInteractKind.TALK, [unreal.PGMissionStep.RADIO], CYLINDER, (0.5, 0.5, 1.8), yaw=149)
radio = interactable("Funkgeraet", -7.4, 10.1, unreal.PGInteractKind.RADIO, [unreal.PGMissionStep.RADIO], CUBE, (0.4, 0.3, 0.25), up=63)

hide = [
    person("Cemal", 9, -14, yaw=17),
    person("Baran", 11, -12),
    person("Kawa", 12.5, -13),
    person("Kaempfer_1", -14, 3, yaw=69),
    person("Kaempfer_2", 8, 6, yaw=-115),
]

wounded = []
for i, (x, z) in enumerate([(-10, -6.5), (10.5, -2.5), (-1, 10.5)]):
    wounded.append(interactable("Verwundeter_%d" % (i + 1), x, z, unreal.PGInteractKind.WOUNDED,
                                [unreal.PGMissionStep.WOUNDED], CYLINDER, (0.5, 0.5, 1.75), up=25, pitch=90, yaw=i * 70))

player_start = spawn(unreal.PlayerStart, at(0, -7, 110), rot(-90.0), "Mission01", "Start_Rojhat")

# ------------------------------------------------------------------ Angriff und Hoehle

impacts = [marker("Einschlag_%d" % (i + 1), x, z, up=150) for i, (x, z, _, _, _) in enumerate(huts[:3])]
cave_marker = marker("Hoehle", -29, 2)
drops = [marker("Hoehle_Platz_%d" % (i + 1), -32.5, 0.5 + i * 1.5, up=30) for i in range(3)]
trigger("Trigger_Hoehle", unreal.PGMissionTrigger, -30.5, 2, (4, 4, 2.5), "Cave")

# ------------------------------------------------------------------ Flucht durch die Schlucht

# Talweg nach Sueden (aus den Hoehendaten des Prototyps)
path = [(0, -40), (14.3, -90), (16, -140), (16, -190), (16, -240), (16, -290), (29.3, -340)]
HUT_A = (24.0, -372.0)
HUT_B = (39.0, -376.0)
shelters = [(5.8, -72), (22, -118), (10, -165), (22, -212), (10, -258), (22, -305), (23.4, -342)]

for i, (x, z) in enumerate(shelters):
    yaw = (i * 37) % 60 - 30
    block("Felsdach_%d" % (i + 1), x, z, (7, 5.5, 0.9), up=260, yaw=yaw, folder="Mission01/Schlucht")
    block("Felsdach_%d_Stuetze_A" % (i + 1), x - 2.8, z, (1.6, 1.6, 2.2), folder="Mission01/Schlucht")
    block("Felsdach_%d_Stuetze_B" % (i + 1), x + 2.8, z, (1.6, 1.6, 2.2), folder="Mission01/Schlucht")
    trigger("Versteck_%d" % (i + 1), unreal.PGShelterVolume, x, z, (3.4, 2.6, 2.0), yaw=yaw)

escape_cp = marker("Fluchtpunkt_Hoehlenausgang", -26, -1, yaw=90)

drone_start = ground(path[0][0], -55)
drone = spawn(unreal.PGReconDrone, unreal.Vector(drone_start.x, drone_start.y, drone_start.z + 9000), rot(), "Mission01/Schlucht", "Drohne")
drone.get_editor_property("body").set_static_mesh(SPHERE)
drone.get_editor_property("body").set_relative_scale3d(unreal.Vector(0.8, 0.8, 0.3))
drone.set_editor_property("patrol_points", [
    unreal.Vector((x - path[0][0]) * 100.0, -(z - (-55)) * 100.0, 0.0) for (x, z) in path[1:] + list(reversed(path[1:-1]))
])

# ------------------------------------------------------------------ Hirtenhuetten und Nacht

hutA = block("Hirtenhuette_A", HUT_A[0], HUT_A[1], (4.5, 4, 2.4), folder="Mission01/Huetten")
hutB = block("Hirtenhuette_B", HUT_B[0], HUT_B[1], (4.5, 4, 2.4), folder="Mission01/Huetten")
hut_marker = marker("Hirtenhuette", HUT_A[0], HUT_A[1])
trigger("Trigger_Huette", unreal.PGMissionTrigger, HUT_A[0], HUT_A[1], (9, 9, 4), "Hut")
night_start = marker("Nacht_Start", HUT_A[0] + 1.5, HUT_A[1] + 4.5, up=110, yaw=90)
night_dara = marker("Nacht_Dara", HUT_A[0] - 1, HUT_A[1] + 3, up=0)
listen = marker("Lauschplatz", HUT_B[0], HUT_B[1] - 4.2)
trigger("Trigger_Lauschen", unreal.PGMissionTrigger, HUT_B[0], HUT_B[1] - 4.2, (2.5, 1.8, 2), "Listen")

lamp = spawn(unreal.PointLight, at(HUT_B[0] + 1.2, HUT_B[1] + 3, 160), rot(), "Mission01/Huetten", "Licht_Huette_B")
ll = lamp.get_editor_property("light_component")
ll.set_editor_property("light_color", unreal.Color(r=255, g=154, b=64, a=255))
ll.set_editor_property("intensity", 800.0)
ll.set_editor_property("attenuation_radius", 1200.0)

guard_pos = at(HUT_A[0] - 9, HUT_A[1] + 8.5, 100)
guard = spawn(unreal.PGGuard, guard_pos, rot(0.0), "Mission01/Huetten", "Wache")
guard.set_editor_property("patrol_points", [unreal.Vector(0, 0, 0), unreal.Vector(2800, 0, 0)])
guard_body = block("Wache_Koerper", HUT_A[0] - 9, HUT_A[1] + 8.5, (0.5, 0.5, 1.8), mesh=CYLINDER, folder="Mission01/Huetten", movable=True)
guard_body.get_editor_property("static_mesh_component").set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
guard_body.set_actor_location(unreal.Vector(guard_pos.x, guard_pos.y, guard_pos.z - 10), False, False)
guard_body.attach_to_actor(guard, "", unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD, unreal.AttachmentRule.KEEP_WORLD, False)

# ------------------------------------------------------------------ Director

director = spawn(unreal.PGMissionDirector, at(0, 0, 300), rot(), "Mission01", "Missionsablauf")
for name, value in [
    ("dialogue_table", dialogue), ("ui_text_table", uitext),
    ("spring", spring), ("dara", dara), ("azad", azad), ("radio", radio),
    ("hide_after_attack", hide), ("impact_points", impacts), ("cave_marker", cave_marker),
    ("wounded", wounded), ("cave_drop_points", drops),
    ("drone", drone), ("escape_checkpoint", escape_cp), ("hut_marker", hut_marker),
    ("night_player_start", night_start), ("night_dara_point", night_dara), ("listen_marker", listen),
    ("guard", guard), ("sun", sun),
]:
    try:
        director.set_editor_property(name, value)
    except Exception as e:
        log("Konnte %s nicht setzen: %s" % (name, e))

unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
log("Fertig. Mit dem gruenen Play-Knopf testen.")
unreal.EditorDialog.show_message(
    "Mission 01",
    "Mission 01 ist aufgebaut und gespeichert.\n\nJetzt oben auf den gruenen Play-Knopf klicken.\n"
    "Steuerung: WASD, Maus, E = interagieren, C = ducken, Leertaste = Untertitel weiter, 1/2 = Entscheidung.",
    unreal.AppMsgType.OK)
