#include "ISData.h"

namespace
{
	const TCHAR* CUBE = TEXT("/Engine/BasicShapes/Cube.Cube");
	const TCHAR* CYL = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
	const TCHAR* SPH = TEXT("/Engine/BasicShapes/Sphere.Sphere");
	const TCHAR* CONE = TEXT("/Engine/BasicShapes/Cone.Cone");

	FString Kit(const TCHAR* N) { return FString::Printf(TEXT("/Game/Infinisat/Models/%s/SM_%s.SM_%s"), N, N, N); }

	// Small helper so the catalog below stays readable.
	struct FP
	{
		FISPart P;
		FP(FName Id, EISCat C, const TCHAR* Name, float Mass, float Cost, int32 Unlock = 0)
		{
			P.Id = Id; P.Cat = C; P.Name = Name; P.MassKg = Mass; P.CostM = Cost; P.Unlock = Unlock;
		}
		FP& Pw(float W) { P.PowerW = W; return *this; }
		FP& Solar(float W) { P.SolarW = W; return *this; }
		FP& Rtg(float W) { P.RtgW = W; P.Tag = ISTag::RTG; return *this; }
		FP& Bat(float Wh) { P.BatteryWh = Wh; return *this; }
		FP& Rel(float R) { P.RelPerYear = R; return *this; }
		FP& MaxKg(float K) { P.MaxTotalKg = K; return *this; }
		FP& Point(float D) { P.PointingDeg = D; return *this; }
		FP& Radio(float Mbps, float RangeKm = 0.f, float Dish = 0.f, float Tx = 0.f) { P.RadioMbps = Mbps; P.RadioRangeKm = RangeKm; P.DishM = Dish; P.TxW = Tx; return *this; }
		FP& Prop(float Kg, float Isp) { P.PropKg = Kg; P.IspS = Isp; return *this; }
		FP& Sail(float M2) { P.SailM2 = M2; return *this; }
		FP& Res(float M, float RefAlt) { P.ResolutionM = M; P.RefAltKm = RefAlt; return *this; }
		FP& Data(float Gb) { P.DataGbDay = Gb; return *this; }
		FP& Rad(float K) { P.RadTolKrad = K; return *this; }
		FP& Tag(FName T) { P.Tag = T; return *this; }
		FP& Mesh(const FString& M) { P.MeshPath = M; return *this; }
		FP& Col(float R, float G, float B) { P.Color = FLinearColor(R, G, B); return *this; }
		FP& Edu(const TCHAR* T) { P.Edu = T; return *this; }
	};

	TArray<FISPart> BuildParts()
	{
		TArray<FISPart> A;
		auto Add = [&A](const FP& F) { A.Add(F.P); };
		using C = EISCat;

		// ------------------------------------------------------------ BUS (structure)
		Add(FP("bus_can", C::Bus, TEXT("CanSat shell (soda-can size)"), 0.06f, 0.000005f).MaxKg(0.35f).Rel(0.999f).Mesh(CYL).Col(0.85f, 0.85f, 0.9f)
			.Edu(TEXT("A CanSat is a whole satellite squeezed into a 66 mm x 115 mm can. Student competitions (ESA, NASA) cap it at about 350 g. It never reaches orbit: a hobby rocket or balloon lifts it and it parachutes back, but it teaches the same budgets a real satellite has.")));
		Add(FP("bus_1u", C::Bus, TEXT("CubeSat 1U frame (10 cm cube)"), 0.2f, 0.02f).MaxKg(1.33f).Rel(0.995f).Mesh(Kit(TEXT("Kit_body1"))).Col(0.75f, 0.75f, 0.78f)
			.Edu(TEXT("One 'U' is a 10 cm cube, max 1.33 kg (CubeSat standard, Cal Poly 1999). Cheap, standard deployers mean you can ride along on almost any launch.")));
		Add(FP("bus_3u", C::Bus, TEXT("CubeSat 3U frame"), 0.4f, 0.05f).MaxKg(4.f).Rel(0.995f).Mesh(Kit(TEXT("Kit_body2"))).Col(0.75f, 0.75f, 0.78f)
			.Edu(TEXT("Three units stacked (10x10x34 cm). The most flown CubeSat size: room for a small camera or radio experiment.")));
		Add(FP("bus_6u", C::Bus, TEXT("CubeSat 6U frame"), 0.9f, 0.1f, 1).MaxKg(12.f).Rel(0.995f).Mesh(Kit(TEXT("Kit_body3"))).Col(0.75f, 0.75f, 0.78f)
			.Edu(TEXT("6U CubeSats (like NASA's CAPSTONE, which flew to the Moon in 2022) can carry propulsion and deep-space radios.")));
		Add(FP("bus_small", C::Bus, TEXT("Small satellite bus (500 kg class)"), 80.f, 8.f, 1).MaxKg(500.f).Rel(0.998f).Mesh(CUBE).Col(0.82f, 0.68f, 0.2f)
			.Edu(TEXT("The 'bus' is the satellite's chassis: structure, wiring and mounting points. A 500 kg-class bus is typical for Earth observation and constellation satellites.")));
		Add(FP("bus_med", C::Bus, TEXT("Medium bus (Landsat class, 2.5 t)"), 300.f, 30.f, 1).MaxKg(2500.f).Rel(0.998f).Mesh(CUBE).Col(0.85f, 0.7f, 0.2f)
			.Edu(TEXT("Landsat 1 was built on a modified Nimbus weather-satellite bus: 953 kg total, 3 m tall, 1.5 m wide. Reusing a proven bus saves money and risk.")));
		Add(FP("bus_geo", C::Bus, TEXT("Large GEO bus (6.5 t)"), 1100.f, 90.f, 1).MaxKg(6500.f).Rel(0.998f).Mesh(CUBE).Col(0.88f, 0.72f, 0.22f)
			.Edu(TEXT("Big communications and weather satellites in geostationary orbit use heavy buses with large propellant tanks for 15 years of station-keeping.")));
		Add(FP("bus_deep", C::Bus, TEXT("Deep-space probe bus (4 t)"), 600.f, 120.f, 5).MaxKg(4000.f).Rel(0.998f).Mesh(CYL).Col(0.8f, 0.8f, 0.82f)
			.Edu(TEXT("Deep-space buses (Juno, Cassini, Voyager) are built for decades of cold, radiation and no repairs. Everything important is doubled.")));
		Add(FP("bus_ultra", C::Bus, TEXT("Ultralight sail-craft frame (150 kg)"), 6.f, 15.f, 7).MaxKg(150.f).Rel(0.997f).Mesh(CUBE).Col(0.9f, 0.9f, 0.95f)
			.Edu(TEXT("Solar sails accelerate faster the lighter the craft is, so the whole probe must weigh very little for its sail area.")));

		// ------------------------------------------------------------ POWER
		Add(FP("pwr_none", C::Power, TEXT("No power generation (battery only)"), 0.f, 0.f).Edu(TEXT("Fine for a short flight (CanSat): the battery must hold all the energy the mission needs. Energy (Wh) = Power (W) x time (h).")));
		Add(FP("pwr_body", C::Power, TEXT("Body-mounted solar cells (2 W)"), 0.05f, 0.01f).Solar(2.f).Rel(0.995f).Col(0.1f, 0.15f, 0.5f)
			.Edu(TEXT("Solar cells glued to the CubeSat's sides. Only some sides face the Sun at any time, so output is small.")));
		Add(FP("pwr_cube", C::Power, TEXT("Deployable CubeSat panels (20 W)"), 0.3f, 0.06f).Solar(20.f).Rel(0.99f).Mesh(Kit(TEXT("Kit_wings1"))).Col(0.1f, 0.15f, 0.5f)
			.Edu(TEXT("Hinged panels unfold after deployment. More area = more power, but deployment is a classic failure point.")));
		Add(FP("pwr_s", C::Power, TEXT("Solar array 600 W"), 15.f, 1.f, 1).Solar(600.f).Rel(0.995f).Mesh(Kit(TEXT("Kit_wings2"))).Col(0.1f, 0.15f, 0.5f)
			.Edu(TEXT("At Earth's distance sunlight delivers 1361 W/m^2. With ~30% efficient cells, 600 W needs about 2 m^2 of panel.")));
		Add(FP("pwr_m", C::Power, TEXT("Solar array 2.5 kW"), 60.f, 5.f, 1).Solar(2500.f).Rel(0.995f).Mesh(Kit(TEXT("Kit_wings2"))).Col(0.1f, 0.15f, 0.5f)
			.Edu(TEXT("Medium arrays power imagers and radios on Landsat-class satellites. Arrays lose about 2-3% output per year to radiation damage.")));
		Add(FP("pwr_l", C::Power, TEXT("Solar array 12 kW (GEO class)"), 250.f, 20.f, 1).Solar(12000.f).Rel(0.995f).Mesh(Kit(TEXT("Kit_wings3"))).Col(0.1f, 0.15f, 0.5f)
			.Edu(TEXT("Communication satellites in GEO need many kilowatts for their transponders.")));
		Add(FP("pwr_juno", C::Power, TEXT("Giant solar wings (14 kW at Earth)"), 340.f, 35.f, 5).Solar(14000.f).Rel(0.995f).Mesh(Kit(TEXT("Kit_wings3"))).Col(0.1f, 0.15f, 0.5f)
			.Edu(TEXT("Juno's three 9 m wings (60 m^2) give ~14 kW at Earth but only ~500 W at Jupiter: sunlight falls with 1/distance^2, and Jupiter is 5.2 AU away (1/27 of the light).")));
		Add(FP("pwr_mmrtg", C::Power, TEXT("RTG nuclear battery (110 W)"), 45.f, 75.f, 5).Rtg(110.f).Rel(0.999f).Mesh(CYL).Col(0.35f, 0.35f, 0.38f)
			.Edu(TEXT("A Radioisotope Thermoelectric Generator turns heat from decaying plutonium-238 into electricity. Works in the dark, far from the Sun. Output drops ~1.6%/year (Curiosity's MMRTG: 110 W).")));
		Add(FP("pwr_gphs", C::Power, TEXT("Triple RTG pack (470 W)"), 170.f, 160.f, 6).Rtg(470.f).Rel(0.999f).Mesh(CYL).Col(0.35f, 0.35f, 0.38f)
			.Edu(TEXT("Voyager launched with three RTGs giving ~470 W in 1977. Almost 50 years later they still give ~220 W, which is why Voyager can still phone home.")));

		// ------------------------------------------------------------ BATTERY
		Add(FP("bat_aa", C::Battery, TEXT("18650 Li-ion cell + holder (9 Wh)"), 0.055f, 0.0000064f).Bat(9.25f).Rel(0.99f).Col(0.2f, 0.6f, 0.2f)
			.Edu(TEXT("An 18650 cell (18 mm x 65 mm, the size laptops and flashlights use) stores 2500 mAh x 3.7 V = 9.25 Wh. Energy (Wh) = capacity (Ah) x voltage (V).")));
		Add(FP("bat_cube", C::Battery, TEXT("CubeSat Li-ion (40 Wh)"), 0.3f, 0.01f).Bat(40.f).Rel(0.99f).Col(0.2f, 0.6f, 0.2f)
			.Edu(TEXT("Keeps a CubeSat alive through the ~35 minute shadow (eclipse) on every low orbit.")));
		Add(FP("bat_s", C::Battery, TEXT("Li-ion battery 600 Wh"), 5.f, 0.4f, 1).Bat(600.f).Rel(0.995f).Col(0.2f, 0.6f, 0.2f)
			.Edu(TEXT("Rule: battery energy needed = load (W) x eclipse time (h) / allowed depth of discharge. Draining Li-ion only ~30% per orbit lets it survive tens of thousands of cycles.")));
		Add(FP("bat_m", C::Battery, TEXT("Li-ion battery 2.5 kWh"), 18.f, 1.2f, 1).Bat(2500.f).Rel(0.995f).Col(0.2f, 0.6f, 0.2f)
			.Edu(TEXT("Medium satellites need a few kWh to ride through eclipse.")));
		Add(FP("bat_l", C::Battery, TEXT("Li-ion battery 10 kWh"), 70.f, 4.f, 1).Bat(10000.f).Rel(0.995f).Col(0.2f, 0.6f, 0.2f)
			.Edu(TEXT("GEO satellites face eclipses up to 72 minutes long around the equinoxes.")));
		Add(FP("bat_probe", C::Battery, TEXT("Probe primary battery (1 kWh, one use)"), 12.f, 2.f, 6).Bat(1000.f).Rel(0.999f).Col(0.2f, 0.6f, 0.2f)
			.Edu(TEXT("The Galileo atmospheric probe ran on non-rechargeable batteries for its 58-minute plunge into Jupiter.")));

		// ------------------------------------------------------------ ADCS (attitude)
		Add(FP("adcs_none", C::ADCS, TEXT("None (tumbling)"), 0.f, 0.f).Point(0.f).Edu(TEXT("Without attitude control the satellite tumbles. OK for a beacon, useless for a camera or a narrow-beam antenna.")));
		Add(FP("adcs_mag", C::ADCS, TEXT("Passive magnet + hysteresis rods (~10 deg)"), 0.05f, 0.005f).Point(10.f).Rel(0.999f)
			.Edu(TEXT("A permanent magnet makes the satellite line up with Earth's magnetic field, like a compass needle. No power needed.")));
		Add(FP("adcs_mtq", C::ADCS, TEXT("Magnetorquers (~5 deg)"), 0.2f, 0.03f).Pw(1.f).Point(5.f).Rel(0.995f)
			.Edu(TEXT("Electromagnets push against Earth's magnetic field to turn the satellite. Only works near Earth.")));
		Add(FP("adcs_rw", C::ADCS, TEXT("Reaction wheels + sun sensors (0.5 deg)"), 6.f, 1.5f, 1).Pw(25.f).Point(0.5f).Rel(0.985f)
			.Edu(TEXT("Spinning a wheel one way turns the satellite the other way (conservation of angular momentum). Wheels wear out: they are the #1 cause of lost pointing (Kepler, Dawn).")));
		Add(FP("adcs_st", C::ADCS, TEXT("Star trackers + reaction wheels (0.01 deg)"), 22.f, 6.f, 1).Pw(60.f).Point(0.01f).Rel(0.99f)
			.Edu(TEXT("Cameras recognise star patterns to know orientation to a few arc-seconds. Needed for sharp images and laser links.")));
		Add(FP("adcs_spin", C::ADCS, TEXT("Spin stabilisation (1 deg)"), 8.f, 1.f, 5).Pw(5.f).Point(1.f).Rel(0.998f)
			.Edu(TEXT("A spinning spacecraft is stable like a gyroscope. Juno spins at 2-5 rpm; Pioneer 10 spun too. Simple and very reliable.")));

		// ------------------------------------------------------------ COMMS
		Add(FP("com_433", C::Comms, TEXT("LoRa radio module (433 MHz, ~10 km)"), 0.005f, 0.0000054f).Pw(0.4f).Radio(0.005f, 10.f).Rel(0.995f)
			.Edu(TEXT("LoRa radios trade speed for range: a few kbit/s, but several km with a simple wire antenna. Signal strength falls with distance squared.")));
		Add(FP("com_uhf", C::Comms, TEXT("UHF beacon radio (9.6 kbps)"), 0.1f, 0.01f).Pw(2.f).Radio(0.0096f, 3000.f).Rel(0.99f).Mesh(Kit(TEXT("Kit_radio2")))
			.Edu(TEXT("Amateur-band radio used by most CubeSats. Radio amateurs worldwide can help receive your data for free.")));
		Add(FP("com_s", C::Comms, TEXT("S-band transceiver (2 Mbps)"), 3.f, 0.4f).Pw(20.f).Radio(2.f, 500000.f, 0.f, 5.f).Rel(0.995f).Mesh(Kit(TEXT("Kit_radio1")))
			.Edu(TEXT("S-band (2-4 GHz) is the workhorse for commands and housekeeping. Landsat 1 used S-band and VHF.")));
		Add(FP("com_x", C::Comms, TEXT("X-band downlink (300 Mbps)"), 8.f, 1.5f, 1).Pw(60.f).Radio(300.f, 500000.f, 0.5f, 10.f).Rel(0.995f).Mesh(Kit(TEXT("Kit_radio1")))
			.Edu(TEXT("Higher frequency = more bandwidth = more data. Imaging satellites dump gigabits of pictures in a few minutes over a ground station.")));
		Add(FP("com_ka", C::Comms, TEXT("Ka-band high-rate (1.2 Gbps)"), 15.f, 4.f, 3).Pw(120.f).Radio(1200.f, 500000.f, 1.f, 20.f).Rel(0.993f).Mesh(Kit(TEXT("Kit_radio3")))
			.Edu(TEXT("Ka-band (26-40 GHz) carries huge data rates but rain absorbs it, so ground stations are placed in dry places.")));
		Add(FP("com_laser", C::Comms, TEXT("Laser terminal (622 Mbps)"), 30.f, 12.f, 5).Pw(90.f).Radio(622.f, 400000.f).Rel(0.99f).Mesh(CYL)
			.Edu(TEXT("NASA's LLCD (2013) sent 622 Mbps from lunar orbit using a laser. Needs very precise pointing (star trackers).")));
		Add(FP("com_hga", C::Comms, TEXT("3.7 m high-gain dish, X-band (deep space)"), 55.f, 10.f, 5).Pw(80.f).Radio(10.f, 0.f, 3.7f, 23.f).Tag(ISTag::HGA).Rel(0.998f).Mesh(Kit(TEXT("Kit_radio3")))
			.Edu(TEXT("Voyager's 3.7 m dish and 23 W transmitter. Data rate falls with distance^2: 115 kbps at Jupiter, ~160 bps today from 25 billion km.")));

		// ------------------------------------------------------------ PROPULSION
		Add(FP("prop_none", C::Propulsion, TEXT("No propulsion"), 0.f, 0.f).Edu(TEXT("Cheapest and simplest, but you cannot change orbit, dodge debris, or deorbit at end of life.")));
		Add(FP("prop_cold", C::Propulsion, TEXT("Cold gas thruster (CubeSat)"), 0.5f, 0.1f).Prop(0.3f, 65.f).Rel(0.99f)
			.Edu(TEXT("Squirts compressed gas. Low efficiency (Isp 65 s) but very safe.")));
		Add(FP("prop_hyd_s", C::Propulsion, TEXT("Hydrazine system, 60 kg fuel"), 15.f, 2.f, 1).Prop(60.f, 220.f).Rel(0.995f).Mesh(SPH).Col(0.9f, 0.9f, 0.95f)
			.Edu(TEXT("Monopropellant hydrazine (Isp ~220 s). Landsat 1 had hydrazine with 3 thrusters. Rocket equation: dv = Isp x g0 x ln(m_full / m_empty).")));
		Add(FP("prop_hyd_l", C::Propulsion, TEXT("Hydrazine system, 300 kg fuel"), 45.f, 5.f, 1).Prop(300.f, 220.f).Rel(0.995f).Mesh(SPH).Col(0.9f, 0.9f, 0.95f)
			.Edu(TEXT("More fuel = more dv, but fuel is heavy, so every extra kilo gives less extra dv (the 'tyranny of the rocket equation').")));
		Add(FP("prop_biprop", C::Propulsion, TEXT("Bipropellant engine, 1800 kg fuel"), 150.f, 15.f, 3).Prop(1800.f, 318.f).Rel(0.993f).Mesh(SPH).Col(0.9f, 0.9f, 0.95f)
			.Edu(TEXT("Fuel + oxidiser burn together (Isp ~318 s). Used to climb from transfer orbit to GEO and to brake into orbit around the Moon or Jupiter.")));
		Add(FP("prop_ion", C::Propulsion, TEXT("Ion thruster, 120 kg xenon"), 40.f, 12.f, 4).Pw(150.f).Prop(120.f, 3000.f).Rel(0.99f).Mesh(CYL).Col(0.4f, 0.5f, 0.9f)
			.Edu(TEXT("Electric propulsion accelerates xenon ions to ~30 km/s (Isp 3000 s): 10x more efficient than chemical rockets but with tiny thrust. Starlink uses Hall thrusters; Dawn used ion engines.")));
		Add(FP("prop_sail_s", C::Propulsion, TEXT("Solar sail 80 m^2 (ACS3 class)"), 16.f, 4.f, 7).Sail(80.f).Tag(ISTag::SolarSail).Rel(0.98f).Mesh(TEXT("/Game/Infinisat/Models/SolarSail/SM_SolarSail.SM_SolarSail"))
			.Edu(TEXT("Sunlight pushes ~9 micro-newtons per square metre at 1 AU. NASA's ACS3 (2024) tested an 80 m^2 sail. Free thrust forever, but tiny.")));
		Add(FP("prop_sail_l", C::Propulsion, TEXT("Solar sail 1,600 m^2"), 30.f, 20.f, 7).Sail(1600.f).Tag(ISTag::SolarSail).Rel(0.97f).Mesh(TEXT("/Game/Infinisat/Models/SolarSail/SM_SolarSail.SM_SolarSail"))
			.Edu(TEXT("A 40 x 40 m sail of ultra-thin film. The key number is area / mass: more m^2 per kg = more acceleration.")));
		Add(FP("prop_sail_xl", C::Propulsion, TEXT("Next-gen sail 40,000 m^2 (200 x 200 m)"), 45.f, 80.f, 7).Sail(40000.f).Tag(ISTag::SolarSail).Rel(0.95f).Mesh(TEXT("/Game/Infinisat/Models/SolarSail/SM_SolarSail.SM_SolarSail"))
			.Edu(TEXT("Future sails of graphene/aluminium films only a few micrometres thick. Concept studies show they could leave the solar system faster than Voyager.")));

		// ------------------------------------------------------------ COMPUTER
		Add(FP("cpu_ard", C::Computer, TEXT("ESP32 microcontroller (240 MHz, Wi-Fi/Bluetooth)"), 0.01f, 0.0000038f).Pw(0.25f).Rad(1.f).Rel(0.99f)
			.Edu(TEXT("The ESP32 is a tiny dual-core computer used in millions of gadgets: it reads the sensors and sends packets to the radio. In orbit, radiation can flip its bits.")));
		Add(FP("cpu_cube", C::Computer, TEXT("CubeSat on-board computer"), 0.1f, 0.02f).Pw(1.f).Rad(15.f).Rel(0.985f)
			.Edu(TEXT("Commercial parts with some protection. Tolerates ~15 krad total dose: fine in low orbit for a few years.")));
		Add(FP("cpu_rh", C::Computer, TEXT("Radiation-hardened flight computer"), 5.f, 3.f, 1).Pw(15.f).Rad(1000.f).Rel(0.998f)
			.Edu(TEXT("RAD750-class computers (Curiosity, JWST, Juno) are slow by laptop standards but survive huge radiation doses.")));

		// ------------------------------------------------------------ THERMAL
		Add(FP("thm_passive", C::Thermal, TEXT("Passive (paint + insulation)"), 0.05f, 0.001f).Rel(0.9995f)
			.Edu(TEXT("Coatings and multi-layer insulation (the gold foil!) balance heat from the Sun against heat radiated to cold space.")));
		Add(FP("thm_none", C::Thermal, TEXT("None: the can itself (CanSat, 10 minute flight)"), 0.f, 0.f)
			.Edu(TEXT("A CanSat flies for minutes in air at 0-30 C: the air carries heat away, so no thermal system is needed. In vacuum there is no air, which is why real satellites need one.")));
		Add(FP("thm_heat", C::Thermal, TEXT("MLI blankets + heaters + radiators"), 8.f, 0.5f, 1).Pw(40.f).Tag(ISTag::Heaters).Rel(0.998f).Col(0.85f, 0.65f, 0.1f)
			.Edu(TEXT("Far from the Sun or in long shadows, electronics freeze without heaters. Heaters cost power, so the power budget grows.")));

		// ------------------------------------------------------------ PAYLOADS
		Add(FP("pl_temp", C::Payload, TEXT("DHT11 temperature + humidity sensor"), 0.003f, 0.0000013f).Pw(0.01f).Data(0.0001f).Tag(ISTag::TempSensor)
			.Edu(TEXT("Measures air temperature and humidity. Going up, air cools by about 6.5 C per km: weather balloons measure exactly this every day.")));
		Add(FP("pl_cam_mini", C::Payload, TEXT("Mini camera OV2640 (CanSat)"), 0.01f, 0.0000113f).Pw(0.8f).Data(0.001f).Tag(ISTag::Camera)
			.Edu(TEXT("A tiny camera module: images use far more data than sensor readings.")));
		Add(FP("pl_gnss", C::Payload, TEXT("Space-grade GNSS receiver (uses GPS/Galileo)"), 0.05f, 0.005f).Pw(0.8f).Data(0.0001f).Tag(ISTag::GNSSRx)
			.Edu(TEXT("Uses EXISTING satellites: by timing signals from 4+ GNSS satellites the receiver solves for x, y, z and its clock error. Helps you find your CanSat, and gives satellites their precise orbit.")));
		Add(FP("pl_bme280", C::Payload, TEXT("BME280 pressure + temperature sensor"), 0.002f, 0.0000059f).Pw(0.004f).Data(0.0001f).Tag(ISTag::TempSensor)
			.Edu(TEXT("Air pressure falls about 12 Pa for every metre you climb near the ground, so a pressure sensor is also an altimeter: it tells you how high the CanSat is.")));
		Add(FP("pl_gnss_mini", C::Payload, TEXT("GNSS module u-blox MAX-M10S (CanSat)"), 0.008f, 0.0000091f).Pw(0.03f).Data(0.0001f).Tag(ISTag::GNSSRx)
			.Edu(TEXT("A thumbnail-sized receiver that listens to GPS, Galileo, GLONASS and BeiDou at once. It needs signals from 4 satellites to solve x, y, z and its own clock error: your CanSat radios its position so you can find it.")));
		Add(FP("pl_clock", C::Payload, TEXT("Atomic clocks (2x rubidium)"), 14.f, 3.f, 1).Pw(80.f).Tag(ISTag::Clock).Rel(0.985f)
			.Edu(TEXT("GNSS works by timing: a clock error of 1 nanosecond = 30 cm of position error (light travels 30 cm per ns). Satellites carry several atomic clocks for backup.")));
		Add(FP("pl_nav", C::Payload, TEXT("Navigation signal payload (L-band)"), 180.f, 35.f, 1).Pw(700.f).Tag(ISTag::NavPayload).Rel(0.995f).Mesh(CONE).Col(0.9f, 0.9f, 0.9f)
			.Edu(TEXT("Broadcasts ranging codes on two L-band carriers so receivers can also correct for ionosphere delay.")));
		Add(FP("pl_mss", C::Payload, TEXT("Multispectral scanner (80 m, 4 bands)"), 60.f, 8.f, 1).Pw(50.f).Res(80.f, 917.f).Data(15.f).Tag(ISTag::Imager).Rel(0.99f).Mesh(CYL).Col(0.3f, 0.3f, 0.32f)
			.Edu(TEXT("Like Landsat 1's MSS: green, red and two near-infrared bands at 80 m, 185 km swath. Near-infrared shows healthy plants brightly, which is why Landsat maps crops and forests.")));
		Add(FP("pl_oli", C::Payload, TEXT("Modern land imager (15/30 m, 9 bands)"), 150.f, 40.f, 2).Pw(200.f).Res(15.f, 705.f).Data(400.f).Tag(ISTag::Imager).Rel(0.993f).Mesh(CYL).Col(0.3f, 0.3f, 0.32f)
			.Edu(TEXT("Landsat 8/9's OLI. Better resolution means much more data: halving pixel size = 4x the pixels.")));
		Add(FP("pl_hires", C::Payload, TEXT("High-res narrow-angle camera (0.5 m)"), 100.f, 30.f, 4).Pw(100.f).Res(0.5f, 50.f).Data(300.f).Tag(ISTag::Imager).Rel(0.99f).Mesh(CYL).Col(0.25f, 0.25f, 0.28f)
			.Edu(TEXT("LRO's camera sees 0.5 m details on the Moon from 50 km, enough to spot Apollo landers and pick safe landing spots.")));
		Add(FP("pl_wx", C::Payload, TEXT("Weather imager (16 bands, full disk / 10 min)"), 450.f, 120.f, 3).Pw(500.f).Res(2000.f, 35786.f).Data(800.f).Tag(ISTag::WeatherImager).Rel(0.993f).Mesh(CUBE).Col(0.9f, 0.9f, 0.92f)
			.Edu(TEXT("GOES/Meteosat-style imagers scan the whole visible Earth every 10-15 minutes from GEO, so forecasters can watch storms grow.")));
		Add(FP("pl_sounder", C::Payload, TEXT("Infrared/microwave sounder"), 110.f, 30.f, 3).Pw(120.f).Data(60.f).Tag(ISTag::Sounder).Rel(0.993f).Mesh(CUBE).Col(0.9f, 0.9f, 0.92f)
			.Edu(TEXT("Polar satellites like Metop and NOAA-20 measure temperature and humidity layer by layer. This data is the biggest single input to weather forecasts.")));
		Add(FP("pl_xpdr", C::Payload, TEXT("Internet transponders + phased arrays"), 60.f, 0.4f, 4).Pw(800.f).Tag(ISTag::Transponder).Rel(0.99f).Mesh(CUBE).Col(0.95f, 0.95f, 0.95f)
			.Edu(TEXT("Each broadband satellite covers only a small spot of Earth from low orbit, so you need hundreds or thousands of them.")));
		Add(FP("pl_lola", C::Payload, TEXT("Laser altimeter (slope maps)"), 15.f, 12.f, 5).Pw(35.f).Data(2.f).Tag(ISTag::Altimeter).Rel(0.993f).Mesh(CYL).Col(0.2f, 0.2f, 0.22f)
			.Edu(TEXT("Times laser pulses bouncing off the surface to measure height. LRO's LOLA mapped Moon slopes: landers need slopes under ~8-15 degrees.")));
		Add(FP("pl_neutron", C::Payload, TEXT("Neutron spectrometer (water ice)"), 20.f, 10.f, 5).Pw(15.f).Data(0.5f).Tag(ISTag::Neutron).Rel(0.995f).Mesh(CUBE).Col(0.6f, 0.6f, 0.65f)
			.Edu(TEXT("Hydrogen (in water ice) slows neutrons. Fewer fast neutrons above a crater = likely ice. Ice near the lunar poles = water and rocket fuel for astronauts.")));
		Add(FP("pl_fields", C::Payload, TEXT("Magnetometer + particle detectors"), 12.f, 8.f, 5).Pw(12.f).Data(1.f).Tag(ISTag::Fields).Rel(0.996f).Mesh(CYL).Col(0.7f, 0.7f, 0.7f)
			.Edu(TEXT("Measures magnetic fields and charged particles. Voyager used them to detect when it crossed the heliopause into interstellar space (2012).")));
		Add(FP("pl_mwr", C::Payload, TEXT("Microwave radiometer (deep atmosphere)"), 40.f, 25.f, 6).Pw(30.f).Data(3.f).Tag(ISTag::Radiometer).Rel(0.994f).Mesh(CUBE).Col(0.85f, 0.85f, 0.85f)
			.Edu(TEXT("Juno's MWR looks hundreds of km below Jupiter's cloud tops to measure water and ammonia.")));
		Add(FP("pl_probe", C::Payload, TEXT("Atmospheric entry probe (the 'beep')"), 340.f, 60.f, 6).Pw(0.f).Data(0.1f).Tag(ISTag::EntryProbe).Rel(0.99f).Mesh(CONE).Col(0.55f, 0.4f, 0.25f)
			.Edu(TEXT("Galileo's probe (1995) hit Jupiter at 47 km/s behind a heat shield, then radioed data for 58 minutes until heat and pressure destroyed it.")));
		Add(FP("pl_record", C::Payload, TEXT("Golden message record"), 0.5f, 0.2f, 7).Tag(ISTag::Record).Rel(1.f).Mesh(CYL).Col(1.f, 0.8f, 0.2f)
			.Edu(TEXT("Voyager carries gold-plated records with sounds and images of Earth, for anyone who might find it.")));

		// ------------------------------------------------------------ EXTRAS
		Add(FP("ex_chute_s", C::Extra, TEXT("Parachute 0.05 m^2"), 0.01f, 0.000005f).Sail(0.05f).Tag(ISTag::Parachute)
			.Edu(TEXT("Descent speed v = sqrt(2 m g / (air density x Cd x area)). Smaller chute = faster, harder landing.")));
		Add(FP("ex_chute_l", C::Extra, TEXT("Parachute 0.2 m^2"), 0.025f, 0.00001f).Sail(0.2f).Tag(ISTag::Parachute)
			.Edu(TEXT("Four times the area halves the descent speed (speed goes with 1/sqrt(area)).")));
		Add(FP("ex_wires", C::Extra, TEXT("Jumper wires + pin headers"), 0.01f, 0.0000021f).Tag(ISTag::Wiring)
			.Edu(TEXT("Wires carry power and data between boards. Real CanSats are soldered and glued: launches shake everything loose!")));
		Add(FP("ex_buzzer", C::Extra, TEXT("Buzzer (recovery beeper)"), 0.003f, 0.0000002f).Pw(0.02f).Tag(ISTag::Buzzer)
			.Edu(TEXT("A loud beeper helps you find your CanSat in tall grass. Real teams always add one.")));
		Add(FP("ex_vault", C::Extra, TEXT("Titanium radiation vault"), 180.f, 15.f, 6).Tag(ISTag::Vault).Rel(1.f).Mesh(CUBE).Col(0.55f, 0.55f, 0.6f)
			.Edu(TEXT("Juno's electronics sit in a 1 cm-thick titanium box (~180 kg) that cuts Jupiter's radiation ~800x. Without it the electronics would die within weeks.")));
		Add(FP("ex_redund", C::Extra, TEXT("Redundant electronics (backups)"), 0.f, 0.f, 1).Tag(ISTag::Redundancy).Rel(1.f)
			.Edu(TEXT("Doubling critical boxes adds 10% mass and 15% cost but lets you switch to a backup after a failure. Mass/cost are added automatically.")));
		Add(FP("ex_dragsail", C::Extra, TEXT("Deorbit drag sail"), 4.f, 0.3f, 4).Sail(10.f).Tag(ISTag::DragSail).Rel(0.98f)
			.Edu(TEXT("A sail that opens at end of life to increase air drag so the satellite burns up within a few years instead of becoming space debris.")));
		Add(FP("ex_retro", C::Extra, TEXT("Laser retroreflector"), 2.f, 0.1f, 1).Tag(ISTag::Retro).Rel(1.f)
			.Edu(TEXT("Mirrors that bounce ground lasers straight back. Satellite Laser Ranging measures orbits to millimetres, and many GNSS satellites carry them for geodesy.")));

		// ------------------------------------------------------------ GROUND SEGMENT (use existing networks!)
		Add(FP("gnd_yard", C::Ground, TEXT("Back-yard antenna (your own)"), 0.f, 0.000015f)
			.Edu(TEXT("A Yagi antenna in your garden. Fine for a CanSat a few km away, or a few short passes per day of a low satellite.")));
		Add(FP("gnd_own", C::Ground, TEXT("One ground station (your own)"), 0.f, 2.f)
			.Edu(TEXT("A low satellite passes over one station 4-6 times a day for ~8 minutes. That limits how much data you can download.")));
		Add(FP("gnd_net", C::Ground, TEXT("Rent a global ground network"), 0.f, 6.f, 1)
			.Edu(TEXT("Companies rent time on dozens of antennas worldwide (including polar stations that see every polar orbit). Using an existing network is cheaper than building your own.")));
		Add(FP("gnd_relay", C::Ground, TEXT("Rent relay satellites (TDRS-like)"), 0.f, 15.f, 2)
			.Edu(TEXT("Relay satellites in GEO (like NASA's TDRS) see low satellites almost all the time, so data flows ~85% of every orbit.")));
		Add(FP("gnd_dsn", C::Ground, TEXT("Deep Space Network (3 x 70 m dishes)"), 0.f, 25.f, 5)
			.Edu(TEXT("Goldstone (USA), Madrid (Spain), Canberra (Australia): 120 degrees apart so one always sees your probe. Their 70 m dishes collect signals weaker than a billionth of a billionth of a watt.")));

		return A;
	}

	TArray<FISVehicle> BuildVehicles()
	{
		TArray<FISVehicle> V;
		auto Add = [&V](FName Id, const TCHAR* Name, const TCHAR* Site, float Cost, float Rel,
			TArray<float> Cap, const TCHAR* Edu, float PerKg = 0.f)
		{
			FISVehicle X; X.Id = Id; X.Name = Name; X.Site = Site; X.CostM = Cost; X.Reliability = Rel; X.CostPerKgM = PerKg; X.Edu = Edu;
			for (int32 i = 0; i < Cap.Num() && i < (int32)EISDest::Count; ++i) X.Capacity[i] = Cap[i];
			V.Add(X);
		};
		//                                                              Backyard LEO   SSO   MEO   GEO*  Moon  Jup   Escape
		Add("lv_hobby", TEXT("Hobby rocket (1 km)"), TEXT("Your back yard / school field"), 0.0002f, 0.95f, { 1, 0, 0, 0, 0, 0, 0, 0 },
			TEXT("A solid-motor model rocket lifts a CanSat to about 1 km. Safety rules and permission needed!"));
		Add("lv_balloon", TEXT("Weather balloon (30 km)"), TEXT("Your back yard / school field"), 0.0005f, 0.97f, { 2, 0, 0, 0, 0, 0, 0, 0 },
			TEXT("A helium balloon rises to ~30 km, above 99% of the air, then bursts. Weather services launch ~1,800 of these every day."));
		Add("lv_rideshare", TEXT("Rideshare slot (SSO, Vandenberg)"), TEXT("Vandenberg SFB, California"), 0.3f, 0.99f, { 0, 0, 830, 0, 0, 0, 0, 0 },
			TEXT("Share a big rocket with dozens of other satellites: ~$6,500 per kg. Cheap, but the rocket goes where the main customer wants (sun-synchronous orbit)."), 0.0065f);
		Add("lv_electron", TEXT("Small launcher (Electron class)"), TEXT("Mahia, New Zealand"), 7.5f, 0.92f, { 0, 300, 200, 0, 0, 30, 0, 0 },
			TEXT("Dedicated small rocket: you choose the exact orbit and date. An Electron with a kick stage sent CAPSTONE toward the Moon."));
		Add("lv_vega", TEXT("Vega-C (ESA)"), TEXT("Guiana Space Centre, Kourou"), 40.f, 0.9f, { 0, 3300, 2300, 0, 0, 0, 0, 0 },
			TEXT("Europe's small-medium launcher. Kourou is just 5 deg from the equator, so Earth's spin gives rockets a free ~460 m/s eastward boost."));
		Add("lv_f9", TEXT("Falcon 9 (reusable)"), TEXT("Cape Canaveral, Florida"), 70.f, 0.99f, { 0, 17500, 15000, 4400, 5500, 3000, 0, 0 },
			TEXT("Reusable first stage lowered launch prices a lot. Launched GPS III satellites directly to MEO."));
		Add("lv_a6", TEXT("Ariane 6 (A64)"), TEXT("Guiana Space Centre, Kourou"), 115.f, 0.95f, { 0, 21600, 15500, 6000, 11500, 8500, 5000, 0 },
			TEXT("ESA's heavy launcher from Kourou: GEO satellites, Galileo navigation satellites, and planetary missions (using gravity assists)."));
		Add("lv_fh", TEXT("Falcon Heavy"), TEXT("Kennedy Space Center, Florida"), 150.f, 0.98f, { 0, 63800, 40000, 15000, 26700, 16000, 6000, 1500 },
			TEXT("Three Falcon 9 cores. Launched Psyche and Europa Clipper. With planetary gravity assists it can send probes out of the solar system."));
		Add("lv_sls", TEXT("SLS Block 1"), TEXT("Kennedy Space Center, Florida"), 2000.f, 0.95f, { 0, 95000, 0, 0, 0, 27000, 9000, 4000 },
			TEXT("NASA's super-heavy Moon rocket (Artemis). Enormous capacity, enormous price."));
		return V;
	}

	FISQuiz Q(const TCHAR* Qs, std::initializer_list<const TCHAR*> A, int32 C, const TCHAR* Why)
	{
		FISQuiz X; X.Q = Qs; for (const TCHAR* S : A) X.A.Add(S); X.Correct = C; X.Why = Why; return X;
	}

	TArray<FISMission> BuildMissions()
	{
		TArray<FISMission> M;
		{
			FISMission X; X.Id = "m0"; X.Chapter = 0; X.Title = TEXT("Ch.0  Back Yard: CanSat");
			X.Client = TEXT("Your school science club");
			X.Brief = TEXT("Build a CanSat (max 350 g) that measures air temperature on the way down from 1 km and radios it to your ground antenna. It must land gently (under 10 m/s) and keep transmitting for 2 hours so you can find it.");
			X.Dest = EISDest::Backyard; X.AltKm = 1.f; X.BudgetM = 0.0004f; X.LifeYears = 2.f / 8760.f; X.MaxMassKg = 0.35f;
			X.Required = { ISTag::TempSensor, ISTag::Parachute, ISTag::Wiring }; X.Bonus = { ISTag::GNSSRx, ISTag::Camera, ISTag::Buzzer };
			X.Learn = { TEXT("Mass budget: every gram counts"), TEXT("Battery energy: Wh = W x hours"), TEXT("Radio range"), TEXT("Parachute physics: v = sqrt(2mg / (rho Cd A))") };
			X.RealName = TEXT("ESA CanSat competition");
			X.RealMesh = TEXT("");
			X.RealFacts = TEXT("Student CanSats: 66 mm x 115 mm, max 350 g, launched to ~1 km by rocket, descent ~8-11 m/s. Thousands of students build one every year across Europe and the USA.");
			X.Quiz = {
				Q(TEXT("Your CanSat draws 2 W. How much battery energy do you need for 2 hours?"), { TEXT("1 Wh"), TEXT("4 Wh"), TEXT("20 Wh") }, 1, TEXT("Energy = power x time = 2 W x 2 h = 4 Wh (add margin!).")),
				Q(TEXT("You make the parachute 4x bigger. The descent speed..."), { TEXT("halves"), TEXT("becomes 4x slower"), TEXT("stays the same") }, 0, TEXT("v depends on 1/sqrt(area): sqrt(4) = 2, so speed halves.")) };
			X.Lesson = TEXT("Even the smallest satellite has a mass, power and communications budget.");
			M.Add(X);
		}
		{
			FISMission X; X.Id = "m1"; X.Chapter = 1; X.Title = TEXT("Ch.1  Where Am I? GNSS satellite");
			X.Client = TEXT("Navigation agency (GPS/Galileo-style constellation)");
			X.Brief = TEXT("One satellite in our 24-satellite navigation constellation has failed. Build a replacement for medium Earth orbit (20,200 km, 55 deg). It needs atomic clocks, a navigation signal payload, Earth-pointing better than 1 deg, and must work for 10 years in the radiation belts.");
			X.Dest = EISDest::MEO; X.AltKm = 20200.f; X.IncDeg = 55.f; X.BudgetM = 250.f; X.LifeYears = 10.f;
			X.Required = { ISTag::Clock, ISTag::NavPayload }; X.Bonus = { ISTag::Retro, ISTag::Redundancy }; X.MaxPointingDeg = 1.f;
			X.Learn = { TEXT("Kepler's 3rd law: period T = 2 pi sqrt(a^3 / mu)"), TEXT("Why GNSS needs 4 satellites (x, y, z + clock)"), TEXT("1 ns clock error = 30 cm position error"), TEXT("Radiation dose and reliability over years") };
			X.RealName = TEXT("GPS Block IIF");
			X.RealMesh = TEXT("");
			X.RealFacts = TEXT("GPS: 24+ satellites, 20,200 km circular orbits, period ~12 h (11 h 58 min). GPS IIF: ~1,630 kg, 12-year design life, rubidium + cesium clocks. Galileo: 23,222 km, 30 satellites in 3 planes. GLONASS: 19,140 km. BeiDou: 35 incl. GEO/IGSO.");
			X.Quiz = {
				Q(TEXT("Why does a receiver need signals from at least 4 GNSS satellites?"), { TEXT("3 for x,y,z plus 1 for its own clock error"), TEXT("one per continent"), TEXT("for backup only") }, 0, TEXT("Four unknowns (x, y, z and the cheap receiver clock's offset) need four equations.")),
				Q(TEXT("A GNSS clock is wrong by 10 nanoseconds. Position error is about..."), { TEXT("3 cm"), TEXT("3 m"), TEXT("3 km") }, 1, TEXT("Light travels ~30 cm per ns, so 10 ns = 3 m.")) };
			X.Lesson = TEXT("Navigation is really about time: atomic clocks in orbit are the product.");
			M.Add(X);
		}
		{
			FISMission X; X.Id = "m2"; X.Chapter = 2; X.Title = TEXT("Ch.2  Eyes on Earth: Landsat");
			X.Client = TEXT("Earth observation programme (it's 1972!)");
			X.Brief = TEXT("Map every farm, forest and city on Earth. Put a multispectral imager (80 m or better) in a sun-synchronous orbit (917 km, 99.2 deg) so every image is taken at the same local time. Pointing 0.7 deg or better, downlink at least 15 Gb of images per day, operate at least 1 year.");
			X.Dest = EISDest::SSO; X.AltKm = 917.f; X.IncDeg = 99.2f; X.BudgetM = 220.f; X.LifeYears = 1.f;
			X.Required = { ISTag::Imager }; X.Bonus = { ISTag::Redundancy, ISTag::GNSSRx }; X.MaxResolutionM = 80.f; X.MaxPointingDeg = 0.7f; X.MinDataGbDay = 15.f;
			X.Learn = { TEXT("Sun-synchronous orbit: same local time every pass"), TEXT("Resolution vs swath vs data volume"), TEXT("Eclipse: battery must cover ~35 min of shadow per orbit"), TEXT("Downlink time over ground stations limits your data") };
			X.RealName = TEXT("Landsat 1 (ERTS-1)");
			X.RealMesh = TEXT("/Game/Infinisat/Models/Landsat1/SM_Landsat1.SM_Landsat1");
			X.RealFacts = TEXT("Launched 23 July 1972 on a Delta 900 from Vandenberg. 953 kg, 917 km SSO, 99.2 deg, 103 min period, 18-day repeat. MSS: 4 bands, 80 m, 185 km swath. Its RBV camera suffered a power fault and was switched off after 15 days, but MSS worked. Design life 1 year; it worked 5.5 years (to Jan 1978).");
			X.Quiz = {
				Q(TEXT("Why use a sun-synchronous orbit for land imaging?"), { TEXT("Same lighting/shadows every pass, so images compare over years"), TEXT("It is the cheapest orbit"), TEXT("It never has eclipses") }, 0, TEXT("The orbit plane turns ~1 deg/day with the Sun, so the satellite crosses the equator at the same local time (Landsat 1: ~9:30 am).")),
				Q(TEXT("You halve the pixel size (80 m -> 40 m). Data volume becomes..."), { TEXT("2x"), TEXT("4x"), TEXT("the same") }, 1, TEXT("Pixels are 2D: half the size in both directions = 4x the pixels.")) };
			X.Lesson = TEXT("Landsat 1 lost a camera in 2 weeks but still mapped the planet: redundancy and margins matter.");
			M.Add(X);
		}
		{
			FISMission X; X.Id = "m3"; X.Chapter = 3; X.Title = TEXT("Ch.3  Storm Watch: weather satellite");
			X.Client = TEXT("European weather service");
			X.Brief = TEXT("Forecasters need a fresh image of the whole Europe/Africa disk every 15 minutes to track storms. Choose the right orbit (hint: what stays above one spot?), carry a weather imager, last 10 years. A sounder is a bonus for better forecasts.");
			X.Dest = EISDest::GEO; X.AltKm = 35786.f; X.IncDeg = 0.f; X.BudgetM = 600.f; X.LifeYears = 10.f;
			X.Required = { ISTag::WeatherImager }; X.Bonus = { ISTag::Sounder, ISTag::Redundancy }; X.MaxPointingDeg = 0.05f; X.MinDataGbDay = 300.f;
			X.Learn = { TEXT("Geostationary orbit: period = 1 day, so it hovers over one spot"), TEXT("GEO vs polar: constant view vs high detail"), TEXT("Delta-v: climbing from transfer orbit to GEO (~1.5 km/s)"), TEXT("Station-keeping fuel limits lifetime") };
			X.RealName = TEXT("GOES-R series / Meteosat");
			X.RealMesh = TEXT("/Game/Infinisat/Models/GOES/SM_GOES.SM_GOES");
			X.RealFacts = TEXT("GEO: 35,786 km above the equator, period 23 h 56 min. GOES-16: ~5,200 kg at launch (~2,900 kg dry), ABI imager scans full disk every 10 min, 15-year life. Polar-orbiting weather satellites (~830 km SSO) give global sounding data twice a day: satellite data is the largest input to modern forecasts.");
			X.Quiz = {
				Q(TEXT("Why does a GEO satellite stay above one spot?"), { TEXT("Its orbit period equals one Earth rotation"), TEXT("It hovers with engines"), TEXT("Gravity is zero there") }, 0, TEXT("At 35,786 km the orbital period is one sidereal day, so it moves with the ground.")),
				Q(TEXT("What do polar-orbiting weather satellites do better than GEO?"), { TEXT("Higher-resolution, global coverage including poles"), TEXT("Continuous view of one spot"), TEXT("Nothing") }, 0, TEXT("They fly ~40x closer to Earth and pass over the poles, covering the whole globe twice a day.")) };
			X.Lesson = TEXT("Pick the orbit from the requirement: 'every 15 minutes, same region' means GEO.");
			M.Add(X);
		}
		{
			FISMission X; X.Id = "m4"; X.Chapter = 4; X.Title = TEXT("Ch.4  Connect: broadband in a crowded sky");
			X.Client = TEXT("Rural broadband start-up");
			X.Brief = TEXT("Design ONE satellite for a 550 km, 53 deg constellation, then decide how many to build (at least 60 for basic coverage of mid-latitudes). LEO is crowded: 11,000+ Starlink satellites already fly. You must be able to dodge debris and deorbit within 5 years after the 5-year mission. Keep it cheap: mass production!");
			X.Dest = EISDest::LEO; X.AltKm = 550.f; X.IncDeg = 53.f; X.BudgetM = 900.f; X.LifeYears = 5.f; X.Fleet = 60;
			X.Required = { ISTag::Transponder }; X.Bonus = { ISTag::DragSail }; X.MaxPointingDeg = 0.5f;
			X.Learn = { TEXT("Coverage: low orbit sees a small circle, so you need many satellites"), TEXT("Space debris and the 5-year deorbit rule"), TEXT("Electric propulsion for orbit raising and collision avoidance"), TEXT("Cost per satellite x fleet size") };
			X.RealName = TEXT("Starlink (data: planet4589.org, 13 Nov 2026)");
			X.RealMesh = TEXT("");
			X.RealFacts = TEXT("13,096 launched, 11,193 in orbit, 11,179 working, 9,704 in operational orbits; 1,903 already re-entered. Shells at 43, 53, 70 and 97 deg. Starlink satellites use krypton/argon Hall thrusters and avoid collisions automatically.");
			X.Quiz = {
				Q(TEXT("Why does a low-orbit internet system need so many satellites?"), { TEXT("Each one sees only a small area and moves quickly"), TEXT("They break often"), TEXT("Law requires it") }, 0, TEXT("At 550 km a satellite sees a circle only ~1,000 km wide and crosses it in minutes.")),
				Q(TEXT("What happens to a dead satellite at 550 km without propulsion?"), { TEXT("Stays forever"), TEXT("Air drag brings it down in roughly 5 years"), TEXT("Falls immediately") }, 1, TEXT("Thin upper atmosphere slowly drags it down; higher orbits take far longer (centuries above ~800 km).")) };
			X.Lesson = TEXT("In a crowded sky, end-of-life disposal is part of the design, not an afterthought.");
			M.Add(X);
		}
		{
			FISMission X; X.Id = "m5"; X.Chapter = 5; X.Title = TEXT("Ch.5  Moonshot: landing-site scout");
			X.Client = TEXT("Lunar landing programme");
			X.Brief = TEXT("Future landers need safe spots near the lunar south pole. Orbit the Moon at 50 km for 1 year. Map with a camera of 1 m or better and a laser altimeter (slopes). Bonus: find water ice with a neutron spectrometer. Brake into lunar orbit yourself (~0.9 km/s) and send 100 Gb/day home.");
			X.Dest = EISDest::Moon; X.AltKm = 50.f; X.IncDeg = 90.f; X.BudgetM = 650.f; X.LifeYears = 1.f;
			X.Required = { ISTag::Imager, ISTag::Altimeter }; X.Bonus = { ISTag::Neutron, ISTag::Redundancy }; X.MaxResolutionM = 1.f; X.MaxPointingDeg = 0.05f; X.MinDataGbDay = 100.f;
			X.Learn = { TEXT("Rocket equation: dv = Isp g0 ln(m0/m1)"), TEXT("Lunar orbit insertion burn"), TEXT("Landing site criteria: slope, light, Earth visibility, ice"), TEXT("Light-time: 1.3 s to the Moon") };
			X.RealName = TEXT("Lunar Reconnaissance Orbiter");
			X.RealMesh = TEXT("/Game/Infinisat/Models/LRO/SM_LRO.SM_LRO");
			X.RealFacts = TEXT("Launched 2009 on Atlas V. 1,916 kg (898 kg propellant). 50 km polar orbit. LROC camera 0.5 m/pixel, LOLA laser altimeter, LEND neutron detector. Ka-band downlink ~460 Gb/day. Still working after 15+ years.");
			X.Quiz = {
				Q(TEXT("Your engine has Isp 318 s. To get 1 km/s of dv, the mass ratio m0/m1 is about..."), { TEXT("1.38"), TEXT("3"), TEXT("10") }, 0, TEXT("ln(m0/m1) = 1000 / (318 x 9.81) = 0.32, so m0/m1 = e^0.32 = 1.38.")),
				Q(TEXT("Why do landing planners care about crater shadows at the poles?"), { TEXT("Permanently shadowed craters may hold water ice"), TEXT("They are warmer"), TEXT("Radio works better") }, 0, TEXT("Some polar craters never see sunlight: cold traps (-230 C) where ice can survive billions of years.")) };
			X.Lesson = TEXT("Fuel is mass: every m/s of braking must be carried all the way from Earth.");
			M.Add(X);
		}
		{
			FISMission X; X.Id = "m6"; X.Chapter = 6; X.Title = TEXT("Ch.6  Big Planet: Jupiter probe");
			X.Client = TEXT("Planetary science directorate");
			X.Brief = TEXT("Orbit Jupiter for 3 years and study its deep atmosphere and magnetic field. At 5.2 AU sunlight is 27x weaker: choose giant solar wings or nuclear RTGs. Jupiter's radiation kills unprotected electronics within weeks. Brake into orbit (~0.6 km/s). Final act (bonus): drop an entry probe that beeps until it is crushed.");
			X.Dest = EISDest::Jupiter; X.AltKm = 4200.f; X.IncDeg = 90.f; X.BudgetM = 1400.f; X.LifeYears = 3.f;
			X.Required = { ISTag::Fields, ISTag::Radiometer, ISTag::HGA }; X.Bonus = { ISTag::EntryProbe, ISTag::Redundancy }; X.MaxPointingDeg = 1.f; X.MinDataGbDay = 0.2f;
			X.Learn = { TEXT("Inverse-square law: sunlight at 5.2 AU = 1/27"), TEXT("Radiation shielding"), TEXT("Link budget over 800 million km"), TEXT("Light-time: ~45 minutes each way") };
			X.RealName = TEXT("Juno (and Galileo's probe)");
			X.RealMesh = TEXT("/Game/Infinisat/Models/Juno/SM_Juno.SM_Juno");
			X.RealFacts = TEXT("Juno: launched 2011, 3,625 kg (incl. ~2,000 kg propellant), first solar-powered Jupiter probe: 3 x 9 m wings, ~14 kW at Earth, ~500 W at Jupiter. 180 kg titanium vault. Spin-stabilised. Galileo's entry probe (1995) transmitted for 58 min inside Jupiter.");
			X.Quiz = {
				Q(TEXT("Jupiter is 5.2 AU from the Sun. A 10 kW (at Earth) solar array gives there about..."), { TEXT("2 kW"), TEXT("370 W"), TEXT("10 kW") }, 1, TEXT("1/5.2^2 = 1/27, so 10,000 W / 27 = ~370 W.")),
				Q(TEXT("A radio signal from Jupiter to Earth takes about..."), { TEXT("1 second"), TEXT("45 minutes"), TEXT("1 day") }, 1, TEXT("~800 million km / 300,000 km/s = ~2,700 s = ~45 min. No joystick control possible!")) };
			X.Lesson = TEXT("Far from the Sun, power and radiation drive the whole design.");
			M.Add(X);
		}
		{
			FISMission X; X.Id = "m7"; X.Chapter = 7; X.Title = TEXT("Ch.7  Infinity: leave the solar system");
			X.Client = TEXT("Humanity (the logo mission)");
			X.Brief = TEXT("Launch from Kourou and reach 120 AU (the heliopause, where interstellar space begins) within 40 years, still sending data. You need ~14 km/s of escape speed: use a huge rocket with gravity assists, OR an ultralight solar sail. Power must last 40 years (RTG decay!). Bonus: carry a golden record.");
			X.Dest = EISDest::Interstellar; X.AltKm = 0.f; X.BudgetM = 900.f; X.LifeYears = 40.f;
			X.Required = { ISTag::Fields, ISTag::HGA }; X.Bonus = { ISTag::Record, ISTag::SolarSail }; X.MaxPointingDeg = 1.f; X.MinDataGbDay = 0.001f;
			X.Learn = { TEXT("Escape speed from the Sun"), TEXT("Solar sail lightness number: beta = sail push / solar gravity"), TEXT("RTG decay over decades"), TEXT("Data rate falls with distance^2") };
			X.RealName = TEXT("Voyager 1");
			X.RealMesh = TEXT("/Game/Infinisat/Models/Voyager/SM_Voyager.SM_Voyager");
			X.RealFacts = TEXT("Launched 5 Sept 1977 on Titan IIIE. 722 kg. Gravity assists at Jupiter and Saturn gave ~17 km/s. Crossed the heliopause at ~121 AU in Aug 2012 (35 years). 3 RTGs: 470 W at launch. Still transmitting (~160 bps) from over 165 AU.");
			X.Quiz = {
				Q(TEXT("A solar sail has lightness number beta = 0.75. Starting from Earth's orbit (29.8 km/s), escape speed is about..."), { TEXT("0 (cannot escape)"), TEXT("21 km/s"), TEXT("100 km/s") }, 1, TEXT("v = 29.8 x sqrt(2 beta - 1) = 29.8 x sqrt(0.5) = ~21 km/s. Beta must be over 0.5 to escape.")),
				Q(TEXT("Why can't Voyager use solar panels?"), { TEXT("At 100+ AU sunlight is 10,000x weaker"), TEXT("Panels are too heavy"), TEXT("It spins") }, 0, TEXT("1/100^2 = 1/10,000 of Earth's sunlight: an RTG is the only option.")) };
			X.Lesson = TEXT("The farthest journeys are won by making things light, efficient, and patient.");
			M.Add(X);
		}
		{
			FISMission X; X.Id = "sandbox"; X.Chapter = 99; X.Title = TEXT("Sandbox: free design");
			X.Client = TEXT("You");
			X.Brief = TEXT("No requirements, unlimited budget. Pick any destination with the arrows and see how your design does. Great for experiments: what is the smallest satellite that can reach Jupiter?");
			X.Dest = EISDest::LEO; X.AltKm = 500.f; X.IncDeg = 51.6f; X.BudgetM = 100000.f; X.LifeYears = 1.f;
			X.Learn = { TEXT("Experiment: change one thing at a time and watch the budgets") };
			X.Lesson = TEXT("Engineering is trade-offs: there is no free lunch in mass, power or money.");
			M.Add(X);
		}
		return M;
	}
}

namespace ISData
{
	const TArray<FISPart>& Parts() { static TArray<FISPart> P = BuildParts(); return P; }
	const TArray<FISVehicle>& Vehicles() { static TArray<FISVehicle> V = BuildVehicles(); return V; }
	const TArray<FISMission>& Missions() { static TArray<FISMission> M = BuildMissions(); return M; }

	const FISPart* FindPart(FName Id)
	{
		for (const FISPart& P : Parts()) if (P.Id == Id) return &P;
		return nullptr;
	}
	const FISVehicle* FindVehicle(FName Id)
	{
		for (const FISVehicle& V : Vehicles()) if (V.Id == Id) return &V;
		return nullptr;
	}

	FString CatName(EISCat C)
	{
		switch (C)
		{
		case EISCat::Bus: return TEXT("Structure (bus)");
		case EISCat::Power: return TEXT("Power source");
		case EISCat::Battery: return TEXT("Battery");
		case EISCat::ADCS: return TEXT("Attitude control");
		case EISCat::Comms: return TEXT("Radio");
		case EISCat::Propulsion: return TEXT("Propulsion");
		case EISCat::Computer: return TEXT("Computer");
		case EISCat::Thermal: return TEXT("Thermal");
		case EISCat::Payload: return TEXT("Payload (instruments)");
		case EISCat::Extra: return TEXT("Extras");
		case EISCat::Ground: return TEXT("Ground network");
		default: return TEXT("?");
		}
	}

	FString CatHelp(EISCat C)
	{
		switch (C)
		{
		case EISCat::Bus: return TEXT("The skeleton. Its size limits the total mass you can build.");
		case EISCat::Power: return TEXT("Makes electricity. Must cover the average load including the time spent in Earth's shadow.");
		case EISCat::Battery: return TEXT("Stores energy for eclipses (and for the whole flight of a CanSat).");
		case EISCat::ADCS: return TEXT("Attitude Determination & Control: keeps cameras and antennas pointed.");
		case EISCat::Comms: return TEXT("Sends data home. Higher bands carry more data.");
		case EISCat::Propulsion: return TEXT("Gives delta-v (change in velocity) for orbit changes, braking, dodging debris and deorbiting.");
		case EISCat::Computer: return TEXT("Runs everything. Must survive the radiation of your orbit.");
		case EISCat::Thermal: return TEXT("Keeps electronics between about -20 and +50 C.");
		case EISCat::Payload: return TEXT("The reason the mission exists. Choose several.");
		case EISCat::Extra: return TEXT("Optional add-ons. Choose several.");
		case EISCat::Ground: return TEXT("Who receives your data? Building your own is expensive: renting existing networks is often smarter.");
		default: return TEXT("");
		}
	}

	FString DestName(EISDest D)
	{
		switch (D)
		{
		case EISDest::Backyard: return TEXT("Back yard (suborbital, 1 km)");
		case EISDest::LEO: return TEXT("Low Earth orbit");
		case EISDest::SSO: return TEXT("Sun-synchronous orbit");
		case EISDest::MEO: return TEXT("Medium Earth orbit");
		case EISDest::GEO: return TEXT("Geostationary orbit");
		case EISDest::Moon: return TEXT("Lunar orbit");
		case EISDest::Jupiter: return TEXT("Jupiter orbit");
		case EISDest::Interstellar: return TEXT("Interstellar space");
		default: return TEXT("?");
		}
	}
}
