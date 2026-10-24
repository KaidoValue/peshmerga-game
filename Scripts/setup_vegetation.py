# -*- coding: utf-8 -*-
"""
Verteilt Felsen, Bueschel, Gras und Baeume auf der Landschaft von Qandil.

Im Editor: Werkzeuge -> Python-Skript ausfuehren... -> Scripts/setup_vegetation.py

Die Modelle liegen in RawData/Models (Poly Haven, CC0, siehe QUELLEN.md).
Beim ersten Mal importiert das Skript sie nach /Game/Vegetation und schaltet
Nanite ein (dauert ein paar Minuten). Die grosse Baum-Datei (95 MB) laedt es
selbst von polyhaven.com, falls sie fehlt.

Regeln:
  - Felsen an steilen Haengen, einzelne auch an mittleren
  - Bueschel und Gras auf flachen Stellen, Gras dicht rund um den Talweg
  - Baeume selten, nur flach und nicht hoch ueber dem Talboden
  - nichts im Lager (40 m um die Mitte, nur Gras zwischen den Huetten), auf dem Talweg nach Sueden,
    an den Felsdaechern und bei den Hirtenhuetten

Alles kommt als Instanzen (HISM) in den Ordner "Vegetation" der Karte. Es kann
mehrfach ausgefuehrt werden: Was es selbst gesetzt hat (Tag "PG_Vegetation"),
wird vorher entfernt. Mit gleichem SEED kommt jedes Mal dasselbe heraus.

Koordinaten in Metern wie in setup_m01.py: x nach Osten, z nach Norden,
Ursprung = Mitte der Landschaft (Lager Berfin).
"""
import json
import math
import os
import random
import urllib.request

import unreal

TAG = "PG_Vegetation"
SEED = 1987
DEST = "/Game/Vegetation"
REIMPORT = False  # True = Modelle neu importieren, auch wenn sie schon da sind

actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assets = unreal.EditorAssetLibrary
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
raw_dir = os.path.join(os.path.abspath(unreal.Paths.project_dir()), "RawData", "Models")

# Modell -> Gewicht innerhalb seiner Gruppe, Groesse (min, max), wie stark es
# sich dem Hang anpasst (0 = senkrecht, 1 = ganz), Einsinken (Anteil der Hoehe)
GROUPS = {
    "fels": {
        "boulder_01": dict(weight=6, scale=(1.4, 3.2), align=0.8, sink=0.35),
        # Felsplatte nur tief eingesunken als Aufschluss an sehr steilen Stellen
        "rock_face_02": dict(weight=1, scale=(1.2, 2.2), align=1.0, sink=0.45, min_slope=32),
    },
    "busch": {
        "searsia_lucida": dict(weight=3, scale=(1.0, 1.8), align=0.15, sink=0.05),
        "shrub_02": dict(weight=2, scale=(1.0, 1.6), align=0.15, sink=0.03),
    },
    "gras": {
        "grass_medium_02": dict(weight=1, scale=(4.0, 7.0), align=0.6, sink=0.0),
    },
    "baum": {
        "tree_small_02": dict(weight=1, scale=(1.6, 2.6), align=0.0, sink=0.02),
    },
}

# Wo und wie viel. slope = Neigung in Grad, height = Meter ueber dem Lager,
# near = Anteil nahe am Talweg (Rest verteilt im Umkreis "radius"),
# spread = typischer Abstand vom Weg in Metern, gap = Mindestabstand,
# cluster = Anzahl je Gruppe und deren Radius
RULES = {
    "fels": dict(count=900, slope=(18, 90), steep=30, height=(-400, 1400), near=0.6, spread=220,
                 radius=1500, gap=4.0, cluster=(1, 0), collision=True, cull=0),
    "busch": dict(count=1600, slope=(0, 30), height=(-400, 700), near=0.85, spread=100,
                  radius=1200, gap=2.0, cluster=(5, 5.0), collision=False, cull=50000),
    "gras": dict(count=22000, slope=(0, 28), height=(-400, 900), near=1.0, spread=55,
                 radius=600, gap=0.0, cluster=(16, 2.5), collision=False, cull=15000, camp=False),
    "baum": dict(count=160, slope=(0, 28), height=(-300, 450), near=0.8, spread=140,
                 radius=1400, gap=12.0, cluster=(3, 14.0), collision=True, cull=0),
}

CAMP_RADIUS = 40.0
# Gras darf ins Lager, aber nicht in Huetten, Feuerstelle und Quelle
CAMP_SPOTS = [(-12, -10, 4.0), (13, -6, 4.5), (-4, 14, 4.0), (16, 12, 4.0), (3, -2, 3.0), (17, 22, 3.5),
              (-7.4, 10.1, 1.5)]
PATH_WIDTH = 7.0  # Abstand links und rechts vom Talweg
# Talweg aus setup_m01.py, vom Lager bis zu den Hirtenhuetten
PATH = [(0, 0), (0, -40), (14.3, -90), (16, -140), (16, -190), (16, -240), (16, -290),
        (29.3, -340), (24, -372), (39, -376)]
KEEP_OUT = [(x, z, 8.0) for x, z in [(5.8, -72), (22, -118), (10, -165), (22, -212), (10, -258),
                                      (22, -305), (23.4, -342)]]  # Felsdaecher
KEEP_OUT += [(24, -372, 18.0), (39, -376, 18.0), (-31, 2, 10.0)]  # Huetten, Hoehle

PH_BIN = "https://dl.polyhaven.org/file/ph-assets/Models/gltf/8k/{a}/{f}"


def log(msg):
    unreal.log("[Vegetation] " + msg)


# ------------------------------------------------------------------ Modelle

def ensure_files(asset, variants):
    """Laedt fehlende Dateien (z. B. die grosse Baum-Datei) von Poly Haven."""
    folder = os.path.join(raw_dir, asset)
    for v in variants:
        g = json.load(open(v))
        for b in g.get("buffers", []):
            f = os.path.join(folder, b["uri"])
            if os.path.isfile(f):
                continue
            url = PH_BIN.format(a=asset, f=b["uri"])
            log("Lade %s (einmalig, bitte warten)" % url)
            req = urllib.request.Request(url, headers={"User-Agent": "PeshmergaGame-Setup"})
            with urllib.request.urlopen(req) as r, open(f + ".part", "wb") as out:
                while True:
                    chunk = r.read(1 << 20)
                    if not chunk:
                        break
                    out.write(chunk)
            os.replace(f + ".part", f)


def find_meshes(folder):
    out = []
    for p in assets.list_assets(folder, recursive=True, include_folder=False):
        a = unreal.load_asset(p.split(".")[0])
        if isinstance(a, unreal.StaticMesh):
            out.append(a)
    return out


def setup_mesh(mesh, collision):
    try:
        nanite = mesh.get_editor_property("nanite_settings")
        if not nanite.get_editor_property("enabled"):
            nanite.set_editor_property("enabled", True)
            mesh.set_editor_property("nanite_settings", nanite)
    except Exception as e:
        log("Nanite fuer %s nicht eingeschaltet: %s" % (mesh.get_name(), e))
    if collision:
        try:
            body = mesh.get_editor_property("body_setup")
            body.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
        except Exception as e:
            log("Kollision fuer %s nicht gesetzt: %s" % (mesh.get_name(), e))
    assets.save_loaded_asset(mesh)


def load_models():
    models = {}
    files = []
    for group, entries in GROUPS.items():
        for asset in entries:
            folder = os.path.join(raw_dir, asset)
            variants = sorted(os.path.join(folder, f) for f in os.listdir(folder)
                              if f.startswith("UE_") and f.endswith(".gltf")) if os.path.isdir(folder) else []
            if not variants:
                log("Keine Modelle in %s gefunden, uebersprungen" % folder)
                continue
            dest = "%s/%s" % (DEST, asset)
            if REIMPORT or not find_meshes(dest):
                files.append((group, asset, dest, variants))
            models[asset] = dest

    if files:
        n = sum(len(v) for _, _, _, v in files)
        with unreal.ScopedSlowTask(n, "Importiere Felsen und Pflanzen (einmalig)") as task:
            task.make_dialog(True)
            for group, asset, dest, variants in files:
                try:
                    ensure_files(asset, variants)
                except Exception as e:
                    log("Download fuer %s fehlgeschlagen: %s" % (asset, e))
                    continue
                tasks = []
                for v in variants:
                    t = unreal.AssetImportTask()
                    t.set_editor_property("filename", v)
                    t.set_editor_property("destination_path", dest)
                    t.set_editor_property("replace_existing", True)
                    t.set_editor_property("automated", True)
                    t.set_editor_property("save", True)
                    tasks.append(t)
                task.enter_progress_frame(len(variants), "Importiere " + asset)
                unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
                for mesh in find_meshes(dest):
                    setup_mesh(mesh, RULES[group]["collision"])
                log("Importiert: %s (%d Varianten)" % (asset, len(find_meshes(dest))))

    out = {}
    for group, entries in GROUPS.items():
        out[group] = []
        for asset, cfg in entries.items():
            meshes = find_meshes(models[asset]) if asset in models else []
            for m in meshes:
                box = m.get_bounding_box()
                out[group].append(dict(cfg, mesh=m, asset=asset, bottom=box.min.z,
                                       height=max(box.max.z - box.min.z, 1.0),
                                       weight=cfg["weight"] / float(len(meshes))))
    return out


# ------------------------------------------------------------------ Gelaende

landscape = None
for a in actors.get_all_level_actors():
    if isinstance(a, unreal.LandscapeProxy):
        landscape = a
        break
if landscape is None:
    unreal.EditorDialog.show_message(
        "Vegetation", "Keine Landschaft gefunden.\n\nBitte zuerst die Karte M01_Qandil oeffnen.",
        unreal.AppMsgType.OK)
    raise SystemExit

# Alte Platzierung entfernen
removed = 0
for a in actors.get_all_level_actors():
    if TAG in [str(t) for t in a.tags]:
        actors.destroy_actor(a)
        removed += 1
if removed:
    log("%d alte Vegetations-Actors entfernt" % removed)

origin, extent = landscape.get_actor_bounds(False)
CX, CY = origin.x, origin.y
TOP = origin.z + extent.z + 5000.0
BOTTOM = origin.z - extent.z - 5000.0
HALF = min(extent.x, extent.y) / 100.0 - 20.0  # Meter, mit Rand
# Nur die Landschaft treffen, nicht Huetten oder Felsdaecher
IGNORE = [a for a in actors.get_all_level_actors() if not isinstance(a, unreal.LandscapeProxy)]


def hit_value(hit, name, index):
    try:
        return hit.get_editor_property(name)
    except Exception:
        return hit.to_tuple()[index]


def probe(x, z):
    """Boden bei x/z (Meter): (Weltposition, Normale, Neigung in Grad) oder None."""
    wx, wy = CX + x * 100.0, CY - z * 100.0
    hit = unreal.SystemLibrary.line_trace_single(
        world, unreal.Vector(wx, wy, TOP), unreal.Vector(wx, wy, BOTTOM),
        unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, True, IGNORE, unreal.DrawDebugTrace.NONE, True)
    if not hit:
        return None
    p = hit_value(hit, "impact_point", 5)
    n = hit_value(hit, "impact_normal", 7)
    slope = math.degrees(math.acos(max(-1.0, min(1.0, n.z))))
    return p, n, slope


def dist_segment(px, pz, ax, az, bx, bz):
    dx, dz = bx - ax, bz - az
    t = max(0.0, min(1.0, ((px - ax) * dx + (pz - az) * dz) / (dx * dx + dz * dz)))
    return math.hypot(px - ax - t * dx, pz - az - t * dz)


def dist_path(x, z):
    return min(dist_segment(x, z, a[0], a[1], b[0], b[1]) for a, b in zip(PATH, PATH[1:]))


def blocked(x, z, camp=True):
    if abs(x) > HALF or abs(z) > HALF:
        return True
    if camp and math.hypot(x, z) < CAMP_RADIUS:
        return True
    if any(math.hypot(x - kx, z - kz) < r for kx, kz, r in CAMP_SPOTS):
        return True
    if any(math.hypot(x - kx, z - kz) < r for kx, kz, r in KEEP_OUT):
        return True
    return dist_path(x, z) < PATH_WIDTH


camp = probe(0, 0)
CAMP_Z = camp[0].z if camp else origin.z


# ------------------------------------------------------------------ Verteilen

class Spacing(object):
    """Mindestabstand ueber ein einfaches Raster."""

    def __init__(self, gap):
        self.gap = gap
        self.cells = {}

    def ok(self, x, z):
        if self.gap <= 0:
            return True
        cx, cz = int(math.floor(x / self.gap)), int(math.floor(z / self.gap))
        for i in (-1, 0, 1):
            for j in (-1, 0, 1):
                for ox, oz in self.cells.get((cx + i, cz + j), ()):
                    if math.hypot(x - ox, z - oz) < self.gap:
                        return False
        return True

    def add(self, x, z):
        if self.gap > 0:
            key = (int(math.floor(x / self.gap)), int(math.floor(z / self.gap)))
            self.cells.setdefault(key, []).append((x, z))


def pick(rng, models):
    r = rng.random() * sum(m["weight"] for m in models)
    for m in models:
        r -= m["weight"]
        if r <= 0:
            return m
    return models[-1]


def transform(rng, m, p, n):
    s = rng.uniform(*m["scale"])
    # Neigung teilweise an den Hang anpassen, dann zufaellig drehen
    a = m["align"]
    up = unreal.Vector(n.x * a, n.y * a, 1.0 - a + n.z * a)
    tilt = unreal.MathLibrary.make_rot_from_z(up)
    spin = unreal.Rotator(roll=0.0, pitch=0.0, yaw=rng.uniform(0.0, 360.0))
    r = unreal.MathLibrary.compose_rotators(spin, tilt)
    # Fuss auf den Boden, dann entlang der geneigten Hochachse einsinken lassen
    ln = math.sqrt(up.x * up.x + up.y * up.y + up.z * up.z)
    d = -(m["bottom"] + m["sink"] * m["height"]) * s / ln
    loc = unreal.Vector(p.x + up.x * d, p.y + up.y * d, p.z + up.z * d)
    return unreal.Transform(location=loc, rotation=r,
                            scale=unreal.Vector(s, s, s))


def scatter(group, models, rule, rng):
    placed = {id(m): [] for m in models}
    spacing = Spacing(rule["gap"])
    per_cluster, cluster_r = rule["cluster"]
    total, tries = 0, 0
    while total < rule["count"] and tries < rule["count"] * 25:
        tries += 1
        if rng.random() < rule["near"]:
            ax, az = rng.choice(PATH)
            bx, bz = rng.choice(PATH)
            t = rng.random()
            cx = ax + (bx - ax) * t + rng.gauss(0.0, rule["spread"])
            cz = az + (bz - az) * t + rng.gauss(0.0, rule["spread"])
        else:
            ang, rad = rng.uniform(0, 2 * math.pi), rule["radius"] * math.sqrt(rng.random())
            cx, cz = 15.0 + rad * math.cos(ang), -180.0 + rad * math.sin(ang)
        for k in range(per_cluster):
            if total >= rule["count"]:
                break
            x = cx + (rng.gauss(0.0, cluster_r) if k else 0.0)
            z = cz + (rng.gauss(0.0, cluster_r) if k else 0.0)
            if blocked(x, z, rule.get("camp", True)) or not spacing.ok(x, z):
                continue
            hit = probe(x, z)
            if hit is None:
                continue
            p, n, slope = hit
            h = (p.z - CAMP_Z) / 100.0
            if not (rule["height"][0] <= h <= rule["height"][1]):
                continue
            if not (rule["slope"][0] <= slope <= rule["slope"][1]):
                continue
            # Felsen: unterhalb von "steep" nur jeder vierte
            if "steep" in rule and slope < rule["steep"] and rng.random() > 0.25:
                continue
            choices = [m for m in models if slope >= m.get("min_slope", 0)]
            if not choices:
                continue
            m = pick(rng, choices)
            placed[id(m)].append(transform(rng, m, p, n))
            spacing.add(x, z)
            total += 1
    log("%s: %d gesetzt (%d Versuche)" % (group, total, tries))
    return placed


# ------------------------------------------------------------------ Instanzen

def add_hism(owner):
    sub = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    gather = getattr(sub, "k2_gather_subobject_data_for_instance", None) or sub.gather_subobject_data_for_instance
    handles = gather(owner)
    params = unreal.AddNewSubobjectParams(parent_handle=handles[0],
                                          new_class=unreal.HierarchicalInstancedStaticMeshComponent,
                                          blueprint_context=None)
    handle, fail = sub.add_new_subobject(params)
    if str(fail):
        raise RuntimeError(str(fail))
    data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
    return unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)


def spawn_holder(label, folder):
    a = actors.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(CX, CY, origin.z))
    a.set_editor_property("tags", [unreal.Name(TAG)])
    a.set_folder_path(folder)
    a.set_actor_label(label)
    return a


def optional(comp, group, rule):
    """Einstellungen, die nur Leistung sparen. Fehler hier sind egal."""
    steps = [lambda: comp.set_mobility(unreal.ComponentMobility.STATIC)]
    if not rule["collision"]:
        steps.append(lambda: comp.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION))
        steps.append(lambda: comp.set_editor_property("can_ever_affect_navigation", False))
    if group == "gras":
        steps.append(lambda: comp.set_cast_shadow(False))
    if rule["cull"]:
        steps.append(lambda: comp.set_cull_distances(int(rule["cull"] * 0.8), int(rule["cull"])))
    for step in steps:
        try:
            step()
        except Exception as e:
            log("Hinweis: %s" % e)


def build(group, models, placed, rule):
    folder = "Vegetation/" + group.capitalize()
    for m in models:
        xf = placed[id(m)]
        if not xf:
            continue
        name = m["mesh"].get_name()
        holder = spawn_holder("Veg_" + name, folder)
        try:
            comp = add_hism(holder)
            comp.set_static_mesh(m["mesh"])
            comp.add_instances(xf, False, True)
            optional(comp, group, rule)
        except Exception as e:
            # Notloesung: einzelne Static-Mesh-Actors (langsamer, ohne Gras)
            log("HISM ging nicht (%s), setze einzelne Actors fuer %s" % (e, name))
            actors.destroy_actor(holder)
            if group == "gras":
                continue
            for i, t in enumerate(xf):
                a = spawn_holder("%s_%d" % (name, i + 1), folder)
                a.set_actor_transform(t, False, False)
                a.get_editor_property("static_mesh_component").set_static_mesh(m["mesh"])


models = load_models()
rng = random.Random(SEED)
summary = []
with unreal.ScopedSlowTask(len(RULES), "Verteile Felsen und Pflanzen") as task:
    task.make_dialog(True)
    for group, rule in RULES.items():
        task.enter_progress_frame(1, "Verteile: " + group)
        if not models.get(group):
            summary.append("%s: keine Modelle" % group)
            continue
        placed = scatter(group, models[group], rule, rng)
        build(group, models[group], placed, rule)
        summary.append("%s: %d" % (group, sum(len(v) for v in placed.values())))

unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
log("Fertig: " + ", ".join(summary))
unreal.EditorDialog.show_message(
    "Vegetation",
    "Felsen und Pflanzen sind verteilt und die Karte ist gespeichert.\n\n" + "\n".join(summary) +
    "\n\nNochmal ausfuehren ersetzt die alte Verteilung.",
    unreal.AppMsgType.OK)
