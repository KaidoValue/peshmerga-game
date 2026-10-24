# 3D-Modelle (Qandil)

Alle von [Poly Haven](https://polyhaven.com), Lizenz CC0 (frei nutzbar, keine Namensnennung noetig).
Format glTF, Texturen 2K (Baum 1K).

| Ordner | Poly-Haven-Asset | Verwendung |
|---|---|---|
| boulder_01 | [Boulder 01](https://polyhaven.com/a/boulder_01) | Felsbrocken |
| rock_face_01 | [Rock Face 01](https://polyhaven.com/a/rock_face_01) | Felswand, nur an sehr steilen Haengen |
| rock_face_02 | [Rock Face 02](https://polyhaven.com/a/rock_face_02) | Felsplatte |
| searsia_lucida | [Searsia Lucida](https://polyhaven.com/a/searsia_lucida) | trockener Busch (7 Varianten) |
| shrub_02 | [Shrub 02](https://polyhaven.com/a/shrub_02) | Strauch (4 Varianten) |
| grass_medium_02 | [Grass Medium 02](https://polyhaven.com/a/grass_medium_02) | Grasbueschel (5 Varianten) |
| tree_small_02 | [Tree Small 02](https://polyhaven.com/a/tree_small_02) | kleiner Laubbaum |

`<name>.gltf` ist das Original. `UE_*.gltf` und `*_diff_alpha_*.png` erzeugt
`Scripts/tools/prepare_models.py` (eine Datei pro Variante, Laub mit Alpha-Maske).
`tree_small_02.bin` (95 MB) ist nicht im Repository; `Scripts/setup_vegetation.py`
laedt sie beim ersten Lauf selbst herunter.
