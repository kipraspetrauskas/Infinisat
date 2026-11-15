using System;
using System.Collections.Generic;
using CosmosLogic.Core;

public static class Prefabs
{
	private class PrefabHolder
	{
		private GameObject _prefab;

		private readonly Func<GameObject> loader;

		public GameObject prefab
		{
			get
			{
				if (!((Object)(object)_prefab == (Object)null))
				{
					return _prefab;
				}
				return _prefab = loader();
			}
		}

		public PrefabHolder(Func<GameObject> loader)
		{
			this.loader = loader;
		}
	}

	private static Dictionary<string, PrefabHolder> Lookup = new Dictionary<string, PrefabHolder>();

	private static bool initialized;

	public static void Initialize()
	{
		if (initialized)
		{
			return;
		}
		float realtimeSinceStartup = Time.realtimeSinceStartup;
		Lookup.Add("SphereMed", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/SphereMed");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("SphereHigh", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/SphereHigh");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("SphereHigher", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/SphereHigher");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("SphereLoD", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/SphereLoD");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("LightPulse", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/LightPulse");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("StarFlares", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/StarFlares");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("InstancedParticleRing", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/InstancedParticleRing");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("Deimos", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/Deimos");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("Phobos", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/Phobos");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("Black Hole", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/BlackHoleSphere");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("Nova", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/Nova");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("Corona", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/Corona");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("Explosion Shockwave", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/ExplosionShockwave");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("Explosion Particles", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/ExplosionParticles");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("Rocket 01", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/SpaceCraft/Rocket_01");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("SPH Renderer", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/SPHRenderer");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("Plume 01", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/Plume_01");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("SPH Particles", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/SPHParticles");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("Collision Particles", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/CollisionParticles");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("Asteroid Particles", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/AsteroidParticles");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("Nova Particles", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/NovaParticles");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("Fireworks Particles", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/FireworksParticles");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("Glow Particles", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/GlowParticles");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("Light Particles", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/LightParticles");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("OrthoCamera", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/OrthoCamera");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("Particle Ring", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/ParticleRing");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("IonizedParticleTail", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/IonizedParticleTail");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("IonTail", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/IonTail");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("DustParticleTail", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/DustParticleTail");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("GalaxyStars", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/GalaxyStars");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("GalaxyClouds", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/GalaxyCloudsNew");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("Annulus", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/Annulus");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("SolarFlare", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/SolarFlare");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("Material Particles", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/MaterialParticles");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("UpAxis", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/UpAxis");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("MagneticAxis", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/MagneticAxis");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("MeshRing", new PrefabHolder(() =>
		{
			
			
			Object val = Resources.Load("Prefabs/MeshRing");
			GameObject val2 = (GameObject)(object)((val is GameObject) ? val : null);
			Mesh sharedMesh = val2.GetComponent<MeshFilter>().sharedMesh;
			Color[] array = new Color[sharedMesh.vertices.Length];
			for (int j = 0; j < array.Length; j++)
			{
				array[j] = new Color(1f, 0f, 0f, 0f);
			}
			sharedMesh.colors = array;
			return val2;
		}));
		Lookup.Add("Fragment 1", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/Fragments/Fragment_01");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("Fragment 2", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/Fragments/Fragment_02");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		int i;
		for (i = 1; i <= 16; i++)
		{
			Lookup.Add("Sphere Fragment " + i.ToString("00"), new PrefabHolder(() =>
			{
				Object val = Resources.Load("Prefabs/Fragments/SphereFragment" + i.ToString("00"));
				return (GameObject)(object)((val is GameObject) ? val : null);
			}));
		}
		Lookup.Add("BaseBall", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/Balls/BaseBall");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("BasketBall", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/Balls/BasketBall");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("BowlingBall", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/Balls/BowlingBall");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("GolfBall", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/Balls/GolfBall");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("PoolBall", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/Balls/PoolBall");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("SoccerBall", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/Balls/SoccerBall");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("TennisBall", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/Balls/TennisBall");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("Teapot", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/Teapot");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("Dice", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/Dice/Dice");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("Dice4", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/Dice/Dice4");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("Dice6", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/Dice/Dice6");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("Dice8", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/Dice/Dice8");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("Dice10", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/Dice/Dice10");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("Dice12", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/Dice/Dice12");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("Dice20", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/Dice/Dice20");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("Monolith", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/Monolith");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("MirrorTetrahedron", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/MirrorTetrahedron");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("PhantomZoneMirror", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/PhantomZoneMirror");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("pumpkin_01", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/pumpkin_01");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("pumpkin_02", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/pumpkin_02");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("pumpkin_03", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/pumpkin_03");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("pumpkin_04", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/pumpkin_04");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("NewHorizonSpacecraft", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/SpaceCraft/NewHorizonSpacecraft");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("juno", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/SpaceCraft/juno");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("S-IVB-3", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/SpaceCraft/S-IVB-3");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("phantomzonemirror", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/phantomzonemirror");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("PoliceBox", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/PoliceBox");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("KhufuPyramid", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/Structures/KhufuPyramid");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("soyuz_complete", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/SpaceCraft/soyuz_complete");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("arrival_spacecraft", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/SpaceCraft/arrival_spacecraft");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("voyager_spacecraft", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/SpaceCraft/voyager_spacecraft");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("CassiniHuygens", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/SpaceCraft/CassiniHuygens");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("tesla_roadster", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/SpaceCraft/tesla_roadster");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("parker", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/SpaceCraft/parker");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("Longsword", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/Longsword");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("Banana", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/Banana");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("Watermelon", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/Watermelon");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("Sledgehammer", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/Sledgehammer");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("Pigeon", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/Pigeon");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("SpermWhale", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/SpermWhale");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("Petunias", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/Petunias");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("HSO_Cube", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/HSOPrimitives/HSO_Cube");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("HSO_Sphere", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/HSOPrimitives/HSO_Sphere");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("HSO_Cone", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/HSOPrimitives/HSO_Cone");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("HSO_Cylinder", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/HSOPrimitives/HSO_Cylinder");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("HSO_Capsule", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/HSOPrimitives/HSO_Capsule");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("HSO_Torus", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/HSOPrimitives/HSO_Torus");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("HSO_Pyramid", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/HSOPrimitives/HSO_Pyramid");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("GenericSat_1", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/SpacecraftParts/GenericSat_1");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("Sputnik_1", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/Spacecraft/Sputnik_1");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("BowlingPin", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/BowlingPin");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("JWST", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/SpaceCraft/JWST");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("Hubble", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/SpaceCraft/Hubble");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("Marble", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/Balls/Marble");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("SphericalCow", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/SphericalCow");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		Lookup.Add("Orion", new PrefabHolder(() =>
		{
			Object val = Resources.Load("Prefabs/Spacecraft/Orion");
			return (GameObject)(object)((val is GameObject) ? val : null);
		}));
		initialized = true;
		Debug.Log((object)("Prefabs loaded in " + (Time.realtimeSinceStartup - realtimeSinceStartup) + " seconds"));
	}

	public static GameObject GetPrefab(string name, Transform parent = null, bool worldSpace = false)
	{
		Initialize();
		if (string.IsNullOrEmpty(name) || !Lookup.ContainsKey(name) || Lookup[name] == null)
		{
			name = "SphereMed";
		}
		GameObject val = Object.Instantiate<GameObject>(Lookup[name].prefab, parent, worldSpace);
		((Object)val).name = name;
		return val;
	}
}
