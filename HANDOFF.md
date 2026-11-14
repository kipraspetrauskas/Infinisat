# Handoff - continue on the main PC (Claude Code: "read HANDOFF.md and do it")

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
