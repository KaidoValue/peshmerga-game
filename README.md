# Peshmerga – Zwei Wege

Story-Spiel in der Ich-Perspektive über zwei Kurden auf verschiedenen Seiten:
**Rojhat** aus Qeladizê und **Zoran** aus Germiyan. Zwölf Missionen in den
Bergen und Ebenen der Autonomen Region Kurdistan, auf Sorani mit deutschen
Untertiteln. Gebaut mit **Unreal Engine 5** (Lumen, Nanite, virtuelle Schatten).

Dieses Repository enthält das Grundgerüst und **Mission 01 „Feuer über Qandil“**
als spielbaren Ablauf in C++. Grafik-Assets (Gelände, Fotoscans, Figuren) legst
du im Editor an. Die Anleitung unten führt dich Schritt für Schritt hin.

> **Wichtig:** Der Code wurde in einer Cloud-Umgebung ohne Unreal Engine
> geschrieben und ist **noch nicht kompiliert**. Beim ersten Öffnen kann es
> kleine Compilerfehler geben. Schick sie mir einfach, dann behebe ich sie.

## Was du brauchst

| | Mindestens | Empfohlen für „sehr hohe“ Grafik |
|---|---|---|
| Grafikkarte | NVIDIA RTX 2070 / AMD RX 6700 (8 GB) | RTX 3060 12 GB oder besser |
| Arbeitsspeicher | 16 GB | 32 GB |
| Speicher | 100 GB SSD | 150 GB NVMe-SSD |
| System | Windows 10/11 64-Bit | Windows 11 |

Software (alles kostenlos):

1. **Epic Games Launcher** → Reiter *Unreal Engine* → *Bibliothek* → **Unreal Engine 5.5** oder neuer installieren. Bei einer neueren Version: Rechtsklick auf die `.uproject` → *Switch Unreal Engine version*.
2. **Visual Studio 2022 Community** mit den Workloads *Spieleentwicklung mit C++* und *Desktopentwicklung mit C++* (inkl. Windows-SDK).
3. **Git** und **Git LFS** (`git lfs install` einmal ausführen).

## Projekt öffnen

```bash
git clone https://github.com/KaidoValue/peshmerga-game.git
cd peshmerga-game
```

1. Rechtsklick auf `PeshmergaGame.uproject` → **Generate Visual Studio project files**.
2. `PeshmergaGame.sln` öffnen, Konfiguration **Development Editor**, **Win64**, dann **Erstellen** (Strg+Umschalt+B).
3. Danach `PeshmergaGame.uproject` doppelklicken. Der Editor startet.

Beim ersten Start fragt Unreal, ob es die Module bauen soll: **Ja**.

## Ordner

| Ordner | Inhalt |
|---|---|
| `Source/PeshmergaGame` | C++: Spielfigur, Interaktion, Missionsablauf, Drohne, Wache, HUD, Speichern |
| `Config` | Grafik (Lumen, Nanite, Raytracing, DX12), Sprachen Deutsch und Sorani |
| `RawData/Heightmaps` | Echte Höhendaten als 16-Bit-PNG für die Landschaft, mit Skalierung als JSON |
| `RawData/Models` | Felsen, Büsche, Gras, Baum (glTF, Poly Haven, CC0) |
| `RawData/Dialogue` | Alle Dialoge und Oberflächentexte von Mission 01 auf Deutsch und Sorani (CSV) |
| `Docs/Mission01.md` | Wie du die Karte von Mission 01 aufbaust und die Mission verbindest |

## Die echte Landschaft importieren

Die Heightmaps stammen aus freien SRTM-Höhendaten (AWS Terrain Tiles). Jede
Karte ist 4 × 4 km groß, 1009 × 1009 Pixel.

| Datei | Ort | Mitte | Höhe |
|---|---|---|---|
| `Qandil_Berfin.png` | Seitental im Qandil-Gebirge (Mission 01) | 36,5758 N · 44,9674 E | 1591–3190 m |
| `Shingal_Suedhang.png` | Südhang des Şingal-Gebirges | 36,3881 N · 41,8050 E | 813–1355 m |
| `Gwer_GrosserZab.png` | Gwer am Großen Zab | 36,0450 N · 43,4800 E | 191–259 m |

So geht's:

1. Neue Karte: *Datei → Neue Ebene → Open World* (oder *Leer*) und als `Content/Maps/M01_Qandil` speichern.
2. *Modus* → **Landschaft** → *Neu* → **Aus Datei importieren** → `RawData/Heightmaps/Qandil_Berfin.png`.
3. **Skalierung** aus der JSON-Datei übernehmen (`ue_landscape_scale`). Für Qandil: X 396,83 · Y 396,83 · Z 312,32. Für Şingal Z 106,02, für Gwer Z 13,4.
4. Auflösung wird erkannt (1009 × 1009). **Importieren**.
5. Nanite für die Landschaft einschalten: Landschaft auswählen → *Details* → **Enable Nanite** → *Build Data*.

Damit sind Tal, Hänge und Gipfel maßstabsgetreu wie in der Wirklichkeit.

## Fotorealistische Oberflächen (Fab / Quixel Megascans)

Quixel Megascans sind Fotoscans echter Felsen, Böden und Pflanzen und für
Unreal-Projekte kostenlos. Seit 2024 findest du sie in **Fab** (im Editor:
*Fenster → Fab*, oder fab.com).

Empfohlen für die Zagros-Berge:

* **Felsen:** „Rocky Cliff“, „Mountain Rock“, „Limestone“ (Kalkstein ist typisch für Qandil)
* **Boden:** „Dry Grass“, „Rocky Dirt“, „Mountain Gravel“
* **Pflanzen:** Eichen (Zagros-Eichenwald), trockene Gräser, Disteln
* **Lager:** Holzbalken, Planen, Steinmauern, Teekessel

Ein Landschaftsmaterial mit Höhen- und Neigungsmischung (Gras unten, Fels an
steilen Hängen, Geröll in Rinnen) bringt am meisten Realismus. Danach Bäume
und Steine mit **PCG** (*Procedural Content Generation*, im Projekt schon
aktiviert) oder dem Foliage-Werkzeug verteilen und für alles **Nanite**
einschalten.

## Felsen, Büsche, Gras und Bäume

`Scripts/setup_vegetation.py` verteilt echte Fotoscan-Modelle (Poly Haven, CC0,
liegen in `RawData/Models`) auf der Karte `M01_Qandil`:

1. Karte `M01_Qandil` öffnen.
2. *Werkzeuge → Python-Skript ausführen…* → `Scripts/setup_vegetation.py`.
3. Beim ersten Mal ein paar Minuten warten (Import, Nanite, Download des Baums).

Felsen kommen an steile Hänge, Büsche und Gras auf flache Stellen, Bäume nur
vereinzelt. Lager, Talweg, Felsdächer und Hirtenhütten bleiben frei. Nochmal
ausführen ersetzt die alte Verteilung.

## Figuren (MetaHumans)

Rojhat, Dara, Azad und Cemal werden mit **MetaHuman Creator** erstellt
(kostenlos, im Browser oder ab UE 5.6 direkt im Editor). Kleidung im Stil der
Region (Shal û Shapik, Kefiyeh) gibt es auf Fab oder du passt vorhandene an.
Die MetaHuman-Blueprints hängst du als *Child Actor* an die
`PGInteractableActor`-Objekte (Dara, Azad) oder setzt sie als Mesh der Wache.

## Sprache: Sorani und Deutsch

* Alle Texte liegen in `RawData/Dialogue/*.csv` in beiden Sprachen.
* Im Editor: CSV in den Content Browser ziehen → Zeilentyp **PGDialogueLine** bzw. **PGUIText** wählen.
* Sprache wechseln: Konsole (`^`) → `PGLanguage ckb` (Sorani) oder `PGLanguage de`. Die Wahl wird gespeichert.
* **Schrift:** Für Sorani eine Schrift mit arabischen Zeichen importieren, z. B. *Noto Sans Arabic* oder *Noto Kufi Arabic* (Google Fonts, frei). Im UMG-HUD als Font setzen.
* Das einfache Text-HUD (ohne Widget) zeigt Sorani-Buchstaben nicht verbunden an. Für Sorani daher ein Widget von **PGHUDWidget** anlegen, siehe `Docs/Mission01.md`. UMG schreibt dann automatisch von rechts nach links.
* Sprachaufnahmen: Pro Dialogzeile kann eine Audiodatei (`Voice`) eingetragen werden. Dann richtet sich die Untertiteldauer nach der Aufnahme.

## Steuerung

| Taste | Aktion |
|---|---|
| W A S D / linker Stick | Gehen |
| Maus / rechter Stick | Umsehen |
| Umschalt | Rennen |
| C | Ducken (wichtig beim Lauschen in der Nacht) |
| E | Sprechen, Wasser schöpfen, Verwundete tragen |
| Leertaste | Springen, oder Untertitel überspringen |
| 1 / 2 | Entscheidung am Ende: erzählen oder schweigen |

## Grafik-Einstellungen

In `Config/DefaultEngine.ini` sind eingeschaltet: Lumen (globale Beleuchtung
und Reflexionen, mit Hardware-Raytracing), Nanite, virtuelle Schatten, TSR,
volumetrischer Nebel und DirectX 12 (SM6). Für Berge zusätzlich in der Karte:
**Sky Atmosphere**, **Volumetric Clouds**, **Exponential Height Fog** mit
volumetrischem Nebel und eine **Directional Light** als Sonne (Atmosphere Sun
Light an). Diese Sonne trägst du im `PGMissionDirector` unter *Sun* ein, dann
stellt die Mission Tag, Nacht und Morgengrauen selbst ein.

## Grundsätze der Geschichte

* Alle Gruppen in der Handlung sind erfunden (**TAZ**, **Direktorat 7**). Die Orte sind echt.
* Der Angreifer in Mission 01 wird nie gezeigt.
* Anfal und Halabja kommen nur als Erinnerung vor. Kein Kampf an Gedenkorten.
* Keine spielbare Gewalt gegen Zivilisten.

## Nächste Schritte

1. Projekt kompilieren, Fehler melden.
2. Karte `M01_Qandil` nach `Docs/Mission01.md` aufbauen.
3. Mission 02 „Das Kind ohne Heimat“ (Zoran, 1988) im gleichen System.
