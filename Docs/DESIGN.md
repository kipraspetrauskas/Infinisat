# Infinisat - design rules (coherence pass, 2026-11-15)

## What the research said

| Game | What makes it hang together | What we take |
|---|---|---|
| Kerbal Space Program (career) | One hub (the Space Center); each building does one job; contracts pay **funds**, flights earn **science**, success earns **reputation**; science buys the tech tree, reputation brings bigger contracts. Critique: when one resource only feeds one thing it stops mattering. | Hub = our space centre at Mahia. Three resources, each with a clear sink. |
| KSP 2 | Better onboarding (short animated lessons) but players said it taught *how* and not *why*, then left them in a sandbox with no goal. | Every order says what you will learn **and** what it pays; the debrief names the one limiting factor. |
| Mars Horizon (ESA-backed) | Agency management + missions + building are separate screens but feed each other; risk vs reward per launch. Critique: too much dice. | Failure comes from the design (budgets), not random rolls; anomalies are real-history events with choices. |
| Universe Sandbox | The simulation is the toy: real scale, clean dark UI, numbers always visible. | Real Earth (Cesium), real orbits, real pads; one dark-panel UI language. |
| Car Mechanic Simulator / PowerWash | Hands-on, physical work in a believable place; the loop is job -> work -> pay -> better tools. | CanSat bench and satellite stand: parts go into real places; payment -> research -> better parts. |
| Career Reimagined (KSP mod) | Milestones that mark real beats of a space programme unlock the matching tech; total science only a bit above the tree cost, so choices matter. | One tech tier per chapter; costs tuned so a 2-star flight pays for the next tier. |

## The loop (one sentence each)

1. **Mission Control** (or the garage laptop): a client posts an order. Orders open with **reputation**.
2. **R&D Lab**: research the tech tier the order needs with **science + funds**.
3. **Assembly Hall stand** (or the garage bench for CanSats): build it; only researched parts are on the stand.
4. **Launch**: buy a ride; it flies from that rocket's **real pad** on the real Earth.
5. **Mission**: time warp, anomalies, choices.
6. **Debrief**: stars, the limiting factor, compare with the real mission, **payment** (funds, reputation, science).
7. Back at the space centre: new orders, new research. Repeat.

## Rules for new work

- **One entry point per thing.** Orders live at Mission Control (+ the laptop). Research lives in the Lab. Maps live at the Tracking Station. The title only starts or continues the company.
- **Every number pays or costs something.** If a new mechanic produces a number, it must feed funds, reputation or science, or gate something.
- **Show the company everywhere** (top bar in panels, top right in the world).
- **Hints are contextual**: only show keys that do something right now.
- **Real places**: launches, sun, orbits use the real launch site of the rocket that was bought.
- **No free scores**: efficiency only counts when the satellite did its job.
- **Teacher mode is a setting**, off by default; it opens everything without touching the save.

## Balance (ISCareer.cpp)

- Contract payment: 5% of the client budget per new star + 25% of money saved (first completion), +10 reputation per new star, -5 for a 0-star flight.
- Science per flight: 3 x objectives met + 5 x stars + up to 20 from returned data (half on repeats).
- Reputation to open chapter N: 0, 10, 30, 50, 70, 90, 110, 130.
- Tier N costs: science 0/15/30/45/60/80/100/120, funds $0/0/5M/10M/20M/30M/45M/60M.

## Still to do (next passes)

- **Art**: one style across house, space centre, pad and stand (currently low-poly house vs grey boxes vs clean room). Blender pass per the memory plan.
- **Facilities**: let funds upgrade buildings (bigger stand = heavier buses, tracking dish = more data/science).
- **Time**: one company calendar (missions take days/years; contracts expire), instead of real-world dates.
- **Window exposure**: the outside looks dark through windows from inside the house.
- **Globe view**: in `-ISScreen=globe` screenshots the Cesium globe had not streamed in (blue faceted surface); check in a real play session.

## Licensing note on `Infinisat 5.8/New data`

That folder contains C# decompiled from Universe Sandbox (`SandboxManager.cs`, `Body.cs` ...) and models extracted from its asset bundles. Those are copyrighted, and the same rule applies as for KSP in "The idea and point of the game": study only, never copy. Nothing from that folder is used in the game. The C++ files are Principia (MIT licence) with names changed; if we ever use Principia ideas in code, credit Principia under its own name and licence.
