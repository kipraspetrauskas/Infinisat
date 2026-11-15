# Handoff - continue on the main PC (Claude Code: "read HANDOFF.md and do it")

## 2026-11-15 coherence pass (branch dev, commit a1d92b5+)
Career spine added (Source/Infinisat/ISCareer.*): funds / reputation / science / tech tiers saved in
GameUserSettings [InfinisatCareer]. Title = Continue / New career / Sandbox / Settings / Quit. R&D Lab researches
tiers; stand only offers researched parts; debriefs pay. Rockets launch from their real pads. Design rules, research
and next steps: Docs/DESIGN.md. Verify: `Tools/shots.sh out x:"-ISFresh -ISCareer=30,40,40,1 -ISScreen=lab"`,
CanSat chain with payout: `-ISFresh -ISWorld -ISCanTest -ISCanTestStay -ISShotDelay=75`.

State (commit 43160ca): packaged Shipping build v0.2 succeeded on the laptop and smoke-tested OK (title, home, orbit).
Source now also has (UNBUILT, unverified): ESA clean-room integration hall for the design view (ISScene.cpp BuildCleanRoom),
modern UI fonts/panels (SISGameUI.cpp ISFont, Content/Slate/Fonts), Docs/PLAY.md.

Do, in order (commit + push after each step):
1. `git pull` (LFS). Build editor: Build.bat InfinisatEditor Win64 Development -Project=<path>/Infinisat.uproject. Fix compile errors.
2. Verify screens: `Tools/shots.sh out title: design:"-ISMission=1 -ISScreen=design" ops:"-ISMission=1 -ISScreen=ops -ISNoLaunch" home:-ISWorld`.
   Clean room must be lit, satellite visible on stand, text readable.
3. LOGO: final logo = Docs/Logo.png (rendered by Tools/logo_render2.py from Docs/Reference/Logo_sketch_v2.jpg).
   Re-import it as /Game/Infinisat/Textures/T_UI_Logo (Tools/setup_assets.py does this), check it shows on the title screen,
   set it as the window/exe icon (Build/Windows/Application.ico from Logo.png) and splash.
4. Re-run `-ISValidate` on home world -> 0 [LAYOUT] issues.
5. Package: RunUAT BuildCookRun -project=... -platform=Win64 -clientconfig=Shipping -build -cook -stage -pak -iostore -archive
   -archivedirectory=<repo>/Packaged -prereqs -nop4 -utf8output -unattended. Smoke-test Packaged/Windows/Infinisat.exe.
6. Ship: copy Docs/PLAY.md into Packaged/Windows, zip -> Infinisat-Windows.zip.
   `gh release create v0.3 Infinisat-Windows.zip -R kipraspetrauskas/Infinisat -t "Infinisat v0.3 (Windows)" -F Docs/PLAY.md`
   (zip >2 GB -> split with 7z -v1900m). Also orphan branch `playable` with PLAY.md + link to the release.
7. Then continue backlog: parts stores (Car Mechanic Simulator style) in the hall, NanoAvionics-style configurator flow
   (Light/Mid/Max tiers, slot-zoom camera, parts inside aluminium skeleton), KSP-style map using ISOrbit, better car model
   (/Game/Infinisat/PH/covered_car), roof-light triangle bug.

8. Fusion add-in source is in Tools/FusionAddin (copy to %APPDATA%/Autodesk/Autodesk Fusion 360/API/AddIns/InfinisatCAD).
   Raw downloads (_downloads/, 2.2 GB) were NOT uploaded; all imported assets are already in Content/ (LFS). Re-fetch with Tools/*.py only if re-importing.
   v0.2 playable zip: GitHub Release v0.2.

## Status (main PC, 2026-11-14)
Steps 1-6 DONE: editor builds; screens verified; new logo on title + exe icon + splash (Build/Windows, force-added);
-ISValidate = 0 [LAYOUT] issues; Shipping packaged + smoke-tested (Low graphics -ISGraphics=0 renders OK);
Release v0.3 published (945 MB zip), `playable` README points to it.
Toolchain installed on this PC via winget: VS 2022 Build Tools (VC + Win SDK 22621), gh. 7-Zip not installed (use `tar -a -cf x.zip dir`).
Known: ops-screen satellite is very dark (needs a fill light); Low graphics sky is dark/flat.
Backlog (step 7) remaining: parts stores, configurator, KSP map, better car, roof-light triangle.

## Player feedback 2026-11-14 -> roadmap (in this order)
A. Bugs/glitches: lighting pass; workbench part pickup broken; floating objects; objects hanging over table edges;
   CanSat parts in illogical places (must sit on shelves/desks, physically resting); launch pad too far;
   rocket model broken; exhaust is a cone on the SIDE (must be at the nozzle, bottom); Earth faceted
   ("3D hexagon") - check Nanite fallback on Low (r.Nanite 0) and use a smooth high-res sphere.
B. Look: realistic PBR like PowerWash Simulator / Car Mechanic Simulator 2021. Cars, other houses and the map must
   match the main house style. Sources: Poly Haven CC0, Fab free, NASA 3D, own models in Fusion. No paid assets.
C. Rocket: real flight engine (thrust, Isp/mass flow, drag, gravity turn, staging, Tsiolkovsky), player control
   (launch time, throttle, pitch/heading, staging, abort), detailed rocket model, realistic costs.
D. Satellite builder (KSP 2 VAB style): start from a blank slate, drag parts into the correct slots with a guide
   OR place freely and let the sim respond; zoom to the slot being changed; part pictures; no overlaps;
   realistic numbers (benchmark against UCS data).
E. World structure: main base = house, then random rocket sites, ESA etc. Missions are real-world events
   (KSP-style contracts), not levels. Clock ticks in milliseconds with slow-motion and warp.
F. UI redesign: Universe Sandbox for the main pages, KSP 2 for the builder.
G. Data/map: real satellites from Content/Infinisat/Data/ucs_satellites.csv (UCS DB 5-1-2023, 7,560 sats)
   in the orbit map; proper map. Cosmos explorer (branch dev, ISCosmos.cpp) still needs its first compile.

## Progress 2026-11-14 (main PC, branch dev -> merged to main)
DONE: Cosmos explorer (ISCosmos.cpp) | smooth SM_SphereHi (fixes faceted Earth) | UCS satellite CSV + Docs/References.md |
A: bench pickup (overlap volumes + smallest-volume trace), SettleProps (props/hunt parts rest on real surfaces),
   ValidateLayout support/overhang check (0 issues), pad 18 m past gate, launch plume at nozzles |
C: real rocket flight (ISRocket.*, ISFlight.cpp): physics, 7 real launchers, autopilot, manual controls, HUD, SkyAtmosphere.
   Verify with: UnrealEditor ... -game -nullrhi -ISRocketTest  (logs [ROCKET] lines)
NEXT: A lighting pass + roof-light triangle (not reproduced yet) | C: pass the achieved orbit into Ops, launch-site choice
(real sites per vehicle), debris/explosion visuals | D builder (KSP2 VAB style) | E world structure (house -> rocket sites -> ESA,
real-world contracts from UCS data, ms clock) | B realistic art | F UI redesign | G UCS satellites on the orbit map.
