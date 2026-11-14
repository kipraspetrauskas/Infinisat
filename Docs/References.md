# Real data and logic sources (all free)

| Source | Used for |
|---|---|
| UCS Satellite Database 5-1-2023 (`Content/Infinisat/Data/ucs_satellites.csv`, `Tools/import_ucs.py`) | 7,560 real satellites: orbits, masses, power, lifetimes, launch sites and vehicles. Orbit map population, Sat-o-pedia, builder benchmarks, mission/contract generation |
| CelesTrak GP data formats - https://celestrak.org/NORAD/documentation/gp-data-formats.php | OMM/TLE orbital elements -> live positions of real satellites (SGP4-style propagation) |
| SatNOGS (Libre Space Foundation) - https://www.libre.space/projects/satnogs/ | Real global network of amateur ground stations: contact windows, downlink planning |
| CubeSat Design Specification rev 14 - https://www.nasa.gov/wp-content/uploads/2022/02/cubesatdesignspecificationrev14_12022-02-09.pdf | Builder rules: 1U = 10 x 10 x 11.35 cm, max 2.00 kg per U, rails, deployment switches, RBF pin |
| NASA CubeSat 101 - https://www.nasa.gov/wp-content/uploads/2017/03/nasa_csli_cubesat_101_508.pdf | Mission life cycle, licensing, testing (vibration, thermal-vacuum) for the career flow |
| SpaceX Rideshare Payload User's Guide - https://storage.googleapis.com/rideshare-static/Rideshare_Payload_Users_Guide.pdf | Realistic launch prices, mass/volume limits, environments (loads, vibration) |
| NASA Small Spacecraft State of the Art 2026 - https://www.nasa.gov/wp-content/uploads/2026/05/soa-2026.pdf | Real part specs for the parts catalogue (power, propulsion, ADCS, comms, C&DH, structures) |
| NASA Basics of Space Flight - https://science.nasa.gov/learn/basics-of-space-flight/ | In-game explanations (Sat-o-pedia, "why?" popups) |
| Braeunig, Rocket and Space Technology - http://www.braeunig.us/space/ | Rocket equation, propulsion, atmosphere model, launcher specs for the flight engine |
| MIT 16.346 Astrodynamics / 16.851 Satellite Engineering (OCW) | Orbit propagation, manoeuvres, subsystem sizing equations |
| TU Delft OCW: Intro Aerospace Engineering II, Spacecraft Technology | Subsystem sizing, launcher performance |
| Basilisk (AVS Lab) - https://hanspeterschaub.info/basilisk/ | Reference for attitude/orbit simulation structure (logic only, no code copied) |
| NASA software GSC-19640-1 | Reference |
| ECSS standards - https://ecss.nl/ | Mission phases and reviews (PDR/CDR/FRR) for the ESA career stage |

## Principia (mockingbirdnest/Principia, MIT) - ideas ported, no code copied
From the user's "Cosmos_Logic" PDFs (part.hpp/.cpp, pile_up.hpp/.cpp, vessel.hpp, interface_part.cpp):
- Part -> PileUp: each attached piece contributes mass, centre of mass and inertia; thrust off the centre line is a torque
  (`ApplyIntrinsicForceWithLeverArm`). Ported as `FISRocketSim::UpdateMassProperties` + `StepAttitude` (engine gimbal,
  aerodynamic instability of a finless rocket, RCS when coasting).
- Mass loss keeps the angular velocity (lost propellant carries its own angular momentum, as Principia does for solid motors).
- History / prediction: an authoritative past trajectory and a forward-integrated prediction. Ported as `History` and
  `Predict()` (velocity-Verlet, symplectic), drawn as orange / cyan lines in the flight view.
