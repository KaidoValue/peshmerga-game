# Mission 01 „Feuer über Qandil“ – Karte aufbauen

Sommer 2015, Lager Berfîn in einem Seitental des Qandil-Gebirges. Rojhat holt
Wasser, bringt Dara Tee, hilft Azad am Funkgerät. Cemal schickt Baran und Kawa
ohne Funkgerät ins Tal. Dann schlägt es dreimal ein. Der Angreifer wird nie
gezeigt. Rojhat trägt Verwundete in die Höhle, flieht mit Dara vor der
Wärmebild-Drohne zur Hirtenhütte und hört in der Nacht, wie Cemal am
Satellitentelefon mit „Direktorat 7“ spricht. Am Morgen entscheidet er: Dara
alles erzählen oder schweigen.

## Ablauf im Code

`APGMissionDirector` steuert alles. Die Schritte (`EPGMissionStep`):

| Schritt | Was passiert | Ausgelöst durch |
|---|---|---|
| Intro | Einblendung „Qandil-Gebirge · Sommer 2015“ | Start |
| Water | Ziel: Wasser an der Quelle | 4,5 s nach Start |
| Tea | Ziel: Dara den Tee bringen | E an der Quelle |
| Radio | Dialog „tea“, dann Ziel: Azad am Funkgerät | E bei Dara |
| Attack | Dialog „radio“, 6 s Stille, drei Einschläge | E am Funkgerät oder bei Azad |
| Cave | Tinnitus, gedämpfter Ton, „Drohne! Alle in die Höhle!“ | erster Einschlag |
| Wounded | 150 s, Verwundete einzeln in die Höhle tragen | Höhle betreten |
| Escape | Drohne aktiv, Felsdächer schützen, Ziel Hirtenhütte | alle gerettet oder Zeit um |
| Night | Abblende, Nacht, Dara in der Hütte | Hütten-Trigger erreicht |
| Listen | Geduckt (C) hinter Hütte B: vier Sätze am Telefon | Dialog „night“ fertig |
| Return | Zurück zu Dara | alle vier Sätze gehört |
| Choice | 1 = erzählen, 2 = schweigen | E bei Dara |
| Dawn | Morgengrauen, Stoffmarke im Schutt | nach der Antwort |
| Finished | Spielstand: `bM01ToldDara`, `M01SavedWounded` | 6 s später |

Wird Rojhat von der Drohne gesehen, startet er am Höhlenausgang neu. Sieht
ihn die Wache in der Nacht, startet er an der Hütte A neu.

## 1. Gelände

Wie im README: `Qandil_Berfin.png` importieren. Das Lager liegt in der
Bildmitte im Talboden, also bei X 0 / Y 0 der Landschaft. Norden ist −Y.

Ungefähre Lage der Orte (in Metern vom Lager, aus dem Browser-Prototyp):

| Ort | Richtung |
|---|---|
| Quelle | 17 m Ost, 22 m Nord |
| Höhle | 29 m West |
| Hirtenhütten A und B | etwa 370 m Süd, die Schlucht hinauf |
| Felsdächer (7 Stück) | entlang des Pfades in der Schlucht |

Den Talboden um das Lager mit dem *Flatten*-Werkzeug leicht ebnen, ebenso den
Platz bei den Hütten.

## 2. Datentabellen

1. `RawData/Dialogue/DT_M01_Dialogue.csv` in den Content Browser ziehen → Typ **PGDialogueLine** → speichern als `Content/Data/DT_M01_Dialogue`.
2. `RawData/Dialogue/DT_M01_UIText.csv` → Typ **PGUIText** → `Content/Data/DT_M01_UIText`.

Die Zeilennamen (z. B. `M01_tea_01`) dürfen nicht geändert werden. Der
Director spielt eine Gruppe (`tea`, `radio`, `attack`, `cave`, `escape`,
`night`, `listen`, `tell`, `silent`) in Reihenfolge ab.

## 3. Objekte in der Karte

Aus dem *Place Actors*-Panel (Suche nach „PG“):

| Objekt | Klasse | Einstellungen |
|---|---|---|
| Quelle | PGInteractableActor | Kind = Water, Active Steps = Water |
| Dara | PGInteractableActor | Kind = Talk, Active Steps = Tea, Return. MetaHuman als Child Actor |
| Azad | PGInteractableActor | Kind = Talk, Active Steps = Radio |
| Funkgerät | PGInteractableActor | Kind = Radio, Active Steps = Radio |
| 3 Verwundete | PGInteractableActor | Kind = Wounded, Active Steps = Wounded. Draußen zwischen den Hütten verteilt |
| Höhle | PGMissionTrigger | Trigger Id = **Cave**, Box füllt den Höhleneingang |
| Hirtenhütte A | PGMissionTrigger | Trigger Id = **Hut** |
| Hinter Hütte B | PGMissionTrigger | Trigger Id = **Listen**, kleine Box an der Rückwand |
| Felsdächer | PGShelterVolume | je eines unter jedem Überhang |
| Drohne | PGReconDrone | Patrol Points über dem Pfad, Altitude 9000, Footprint-Material rot und leuchtend |
| Wache | PGGuard | Patrol Points um die Hütten, Mesh = MetaHuman oder Mannequin |
| Director | PGMissionDirector | einmal in die Karte |

Leere Actors (*Empty Actor* oder *Target Point*) als Punkte:
`ImpactPoints` (drei Hütten im Lager), `CaveMarker`, `CaveDropPoints` (drei
Liegeplätze in der Höhle), `EscapeCheckpoint` (Höhlenausgang), `HutMarker`,
`NightPlayerStart` (in Hütte A), `NightDaraPoint`, `ListenMarker`.

Cemal, Baran, Kawa und die übrigen Kämpfer im Lager kommen in
`HideAfterAttack`. Nach dem ersten Einschlag sind sie fort.

## 4. Director verbinden

`PGMissionDirector` auswählen und im Details-Panel alle Felder unter
*Mission* zuweisen: Datentabellen, Figuren, Punkte, Drohne, Wache und die
Sonne (*Directional Light*). Effekte sind optional, machen aber den Angriff:

* **ExplosionFX:** Niagara-System (z. B. aus dem kostenlosen *Niagara Examples* oder Fab).
* **ExplosionSound**, **TinnitusSound:** Pfeifton von etwa 8 s.
* **MuffleMix:** Sound Mix mit Tiefpass auf der Master-Sound-Class (etwa 800 Hz).
* **ImpactShake:** Blueprint von *CameraShakeBase* mit Perlin-Noise.

Im **Level-Blueprint** zwei Ereignisse des Directors nutzen:

* `OnImpact(Index, Point)`: Hütte gegen zerstörte Version tauschen, Feuer und Rauch einschalten.
* `OnTimeOfDay(Phase)`: bei „Night“ Mondlicht, dunkler Nebel, Sterne; bei „Dawn“ warmer Nebel.

## 5. HUD mit Sorani

Ohne weiteres zeichnet `APGHUD` ein einfaches Text-HUD (gut zum Testen auf
Deutsch). Für Sorani:

1. *Widget Blueprint* → Elternklasse **PGHUDWidget** → `WBP_HUD`.
2. Text-Elemente mit genau diesen Namen anlegen: `ObjectiveText`, `SpeakerText`, `SubtitleText`, `PromptText`, `CaptionTitleText`, `CaptionSubText`.
3. Optional: `ObjectiveMarker` (Bild), `ChoicePanel` mit `ChoiceQuestionText`, `ChoiceTellButton` + `ChoiceTellText`, `ChoiceSilentButton` + `ChoiceSilentText`.
4. Schrift *Noto Sans Arabic* als Font-Asset importieren und allen Texten zuweisen.
5. Blueprint von **PGPlayerController** anlegen, `HUD Widget Class` = `WBP_HUD`, im Game Mode (Blueprint von PGGameMode) als Player Controller setzen.

## 6. Prüfliste

* [ ] Start: Einblendung, dann Ziel „Hol Wasser an der Quelle“.
* [ ] Quelle → Dara → Funkgerät in dieser Reihenfolge, sonst keine Reaktion.
* [ ] Drei Einschläge, Tinnitus, Lager leer bis auf die Verwundeten.
* [ ] Verwundete tragen (langsamer, kein Rennen), in der Höhle ablegen, Zähler steigt.
* [ ] Nach 150 s geht es auch mit weniger Geretteten weiter.
* [ ] Drohne: im roten Kreis nach 1,2 s entdeckt, unter Felsdach nicht.
* [ ] Nacht: nur geduckt hinter Hütte B laufen die Telefonsätze weiter.
* [ ] Wache sieht dich stehend auf 16 m, geduckt auf etwa 7 m.
* [ ] Entscheidung mit 1 oder 2, danach Morgengrauen und Ende.
* [ ] `PGLanguage ckb`: alle Texte auf Sorani, von rechts nach links.
