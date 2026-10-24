# -*- coding: utf-8 -*-
"""
Bereitet die Poly-Haven-Modelle in RawData/Models fuer Unreal vor.
Nur fuer Entwickler, laeuft ausserhalb von Unreal: python Scripts/tools/prepare_models.py
(braucht Pillow).

Fuer jedes Modell
  - bekommen Blaetter und Gras eine Farbtextur mit Alphakanal (PNG). Poly Haven
    liefert die Farbe als JPG ohne Alpha und die Maske als eigene Datei; ohne
    das waeren die Blaetter in Unreal undurchsichtige Vierecke.
  - werden durchsichtige Materialien auf "MASK" gestellt (Laub braucht keine
    echte Transparenz und sieht so mit Nanite und Lumen sauber aus).
  - wird jede Variante (Poly Haven legt mehrere Bueschel nebeneinander in eine
    Datei) als eigene UE_<name>.gltf gespeichert, mit Ursprung am Fuss.

setup_vegetation.py importiert nur die UE_*.gltf.
"""
import copy
import glob
import json
import os
import sys

from PIL import Image

ROOT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "RawData", "Models")


def rgba(folder, diff_uri, alpha_path):
    out_uri = os.path.splitext(diff_uri)[0].replace("_diff", "_diff_alpha") + ".png"
    out = os.path.join(folder, out_uri)
    if not os.path.isfile(out):
        rgb = Image.open(os.path.join(folder, diff_uri)).convert("RGB")
        a = Image.open(alpha_path).convert("L").resize(rgb.size)
        rgb.putalpha(a)
        rgb.save(out, optimize=True)
        print("  ", out_uri)
    return out_uri


def prepare(asset):
    folder = os.path.join(ROOT, asset)
    g = json.load(open(os.path.join(folder, asset + ".gltf")))
    alphas = glob.glob(os.path.join(folder, "textures", "*alpha*.png"))
    alphas = [a for a in alphas if "_diff_alpha" not in a]
    for m in g.get("materials", []):
        if m.get("alphaMode") not in ("MASK", "BLEND"):
            continue
        m["alphaMode"] = "MASK"
        m["alphaCutoff"] = 0.5
        tex = m["pbrMetallicRoughness"]["baseColorTexture"]["index"]
        img = g["images"][g["textures"][tex]["source"]]
        if img["uri"].endswith(".png"):
            continue
        if len(alphas) != 1:
            sys.exit("%s: keine eindeutige Alpha-Maske gefunden" % asset)
        img["uri"] = rgba(folder, img["uri"], alphas[0])
        img["mimeType"] = "image/png"

    for old in glob.glob(os.path.join(folder, "UE_*.gltf")):
        os.remove(old)
    for ni in g["scenes"][g.get("scene", 0)]["nodes"]:
        node = g["nodes"][ni]
        if "mesh" not in node:
            continue
        name = node["name"].replace("_LOD0", "")
        v = copy.deepcopy(g)
        n = {k: val for k, val in node.items() if k not in ("translation", "children")}
        n["name"] = name
        n["mesh"] = 0
        v["nodes"] = [n]
        v["meshes"] = [copy.deepcopy(g["meshes"][node["mesh"]])]
        v["meshes"][0]["name"] = name
        v["scenes"] = [{"name": name, "nodes": [0]}]
        v["scene"] = 0
        with open(os.path.join(folder, "UE_%s.gltf" % name), "w") as f:
            json.dump(v, f, indent=1)
        print("  UE_%s.gltf" % name)


for asset in sorted(os.listdir(ROOT)):
    if os.path.isfile(os.path.join(ROOT, asset, asset + ".gltf")):
        print(asset)
        prepare(asset)
