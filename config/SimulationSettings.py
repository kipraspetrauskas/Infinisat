using System;
using System.Runtime.InteropServices;
using CosmosLogic.Core;

namespace NBody;

[StructLayout(LayoutKind.Sequential)]
public class NBodyOptions
{
	public double LargeImpactorFrictionCoefficient = 16.0;

	public double SmallImpactorFrictionCoefficient = 0.05;

	public int MaxVolatileFragments = 8;

	public double MaxVolatileLossParameter = 43.0;

	private double gravitationalConstant = 6.6740831E-11;

	private double cellDistanceRatio = 0.25;

	private double openingAngle = 0.8;

	private double tidalHeatingFactor = 0.00287;

	private double tidalLockingAndAccelerationFactor = 1.0;

	private bool acceptPositionOnlyBasedAcceleration = true;

	private double rocheLimitMinimalRadius = 100000.0;

	private bool baryCentricParenting = true;

	private double availableFragments = 1.0;

	private bool collisionsEnabled = true;

	private bool fragmentationEnabled = true;

	private bool maintainCenterOfMass;

	private double collisionDelayMultiplier = 0.1;

	private double normalTangentialFrictionRatio = 0.02;

	private double rollingFrictionMagnitude = 0.05;

	private double spinFrictionCoefficient = 0.1;

	private double shearCompensationFactor = 0.8;

	private double ejectionVelocityScalar = 0.2;

	private double shockWaveRangeMultiplier = 0.1;

	private double tetherTolerance = 0.01;

	private double rocheHelperAccelerationFraction = 0.5;

	private double rocheSampleFragmentsStartRocheLobeRatio = 1.0;

	private double rocheSampleFragmentsStopRocheLobeRatio = 1.5;

	private int numSampleFragments = 100;

	private double rocheLobeLossFactor = 0.001;

	private double rocheSampleProjectFactor = 0.7;

	private double rocheSampleSpawnFactor = 0.8;

	private double rocheSampleEscapeFactor = 0.9;

	private double rocheSampleDelayBeforeCollisionFactor = 5.0;

	private double rocheMaximalFragmentMassRatio = 0.01;

	private double spinFragmentationSpawnRate = 2.777777777777778E-05;

	private double spinFragmentationRandomVelocityFactor = 0.1;

	private double totalFragmentationDurationFactor = 0.2;

	private double totalFragmentationCoreMassFraction = 0.125;

	private double totalFragmentationCoreFractionThreshold = 0.005;

	private double totalFragmentationBudgetFactor = 0.75;

	private double totalFragmentationFragmentDensityFactor = 0.75;

	private double gasFragmentsDensityFactor = 0.05;

	private double frictionFragmentationSpawnFactor = 0.01;

	private double frictionFragmentationSweeping = 0.02;

	private bool rocheFragmentationEnabled = true;

	private bool frictionFragmentationEnabled = true;

	private bool spinFragmentationEnabled = true;

	private bool totalFragmentationEnabled = true;

	private double fragmentSpinRateFactor = 1.0;

	private bool gravityEnabled = true;

	private double maxSpinRateIncreasePerFrame = 1.2;

	[PropertyInfo("7366f63a25ac474692f89ce881fc359c", "")]
	public int NumExplosionFillerFragments => Settings.Global.MaxNonAttracting / 4;

	[PropertyInfo("eee3e22dda4549d2a3bc1f837621d546", "")]
	public bool GravityEnabled
	{
		get
		{
			return gravityEnabled;
		}
		set
		{
			gravityEnabled = value;
		}
	}

	[PropertyInfo("66e8a457112f4b9eaea486787738b6dd", "")]
	public double GravitationalConstant
	{
		get
		{
			if (!GravityEnabled)
			{
				return 0.0;
			}
			return gravitationalConstant;
		}
		set
		{
			gravitationalConstant = value;
			if (BuildInfo.DEBUG_SIMULATIONSTART)
			{
				Debug.Log((object)("Changing gravitational constant to " + gravitationalConstant));
			}
		}
	}

	[PropertyInfo("1f8029884dab4c6181ece34541f17fd4", "")]
	public double CellDistanceRatio
	{
		get
		{
			return cellDistanceRatio;
		}
		set
		{
			cellDistanceRatio = value;
			if (BuildInfo.DEBUG_SIMULATIONSTART)
			{
				Debug.Log((object)("Changing cellDistanceRatio to " + cellDistanceRatio));
			}
		}
	}

	[PropertyInfo("26a8812a62024fb79088b85ff25000ed", "")]
	public bool AcceptPositionOnlyBasedAcceleration
	{
		get
		{
			return acceptPositionOnlyBasedAcceleration;
		}
		set
		{
			acceptPositionOnlyBasedAcceleration = value;
		}
	}

	[PropertyInfo("0b1da339e6b7416fb9493e5aff1eeb0d", "")]
	public bool FrictionFragmentationEnabled
	{
		get
		{
			return frictionFragmentationEnabled;
		}
		set
		{
			frictionFragmentationEnabled = value;
		}
	}

	[PropertyInfo("108f7620a08046dea61035cd484595e0", "")]
	public bool RocheFragmentationEnabled
	{
		get
		{
			return rocheFragmentationEnabled;
		}
		set
		{
			rocheFragmentationEnabled = value;
		}
	}

	[PropertyInfo("7b52af8a10ef4bfda0ed2e0c22aa7dde", "")]
	[Unit(UnitType.Length)]
	public double RocheLimitMinimalRadius
	{
		get
		{
			return rocheLimitMinimalRadius;
		}
		set
		{
			if (value < 1000.0)
			{
				throw new ArgumentOutOfRangeException("RocheLimitMinimalRadius radus must be at least 1000 meters");
			}
			rocheLimitMinimalRadius = value;
			Debug.Log((object)("Changing Roche minimal radius to " + value));
		}
	}

	[PropertyInfo("1698da3d10444a5db9ec542b950fed24", "")]
	[Unit(UnitType.Generic)]
	public double RocheHelperAccelerationFraction
	{
		get
		{
			return rocheHelperAccelerationFraction;
		}
		set
		{
			if (value < 0.0)
			{
				throw new ArgumentException("The roche helping acceleration fracting must be non zero");
			}
			rocheHelperAccelerationFraction = value;
		}
	}

	public double RocheSampleFragmentsStartRocheLobeRatio
	{
		get
		{
			return rocheSampleFragmentsStartRocheLobeRatio;
		}
		set
		{
			if (value > rocheSampleFragmentsStopRocheLobeRatio)
			{
				throw new ArgumentException("The roche helping acceleration fracting must be non zero");
			}
			if (value < 0.0)
			{
				throw new ArgumentException("The roche helping acceleration fracting must be non zero");
			}
			rocheSampleFragmentsStartRocheLobeRatio = value;
		}
	}

	[PropertyInfo("d74e2ca1752740cba62eeb96fa689029", "")]
	public double RocheLobeLossFactor
	{
		get
		{
			return rocheLobeLossFactor;
		}
		set
		{
			if (value < 0.0)
			{
				throw new ArgumentOutOfRangeException("The loss factor must be positive");
			}
			rocheLobeLossFactor = value;
		}
	}

	public double RocheSampleFragmentsStopRocheLobeRatio
	{
		get
		{
			return rocheSampleFragmentsStopRocheLobeRatio;
		}
		set
		{
			if (value >= 10.0)
			{
				throw new ArgumentException("The roche helping acceleration fracting must be non zero");
			}
			if (value <= rocheSampleFragmentsStartRocheLobeRatio)
			{
				throw new ArgumentException("The roche helping acceleration fracting must be non zero");
			}
			rocheSampleFragmentsStopRocheLobeRatio = value;
		}
	}

	public double RocheSampleProjectFactor
	{
		get
		{
			return rocheSampleProjectFactor;
		}
		set
		{
			if (value < 0.0)
			{
				throw new ArgumentOutOfRangeException("Factor must be positive");
			}
			rocheSampleProjectFactor = value;
		}
	}

	public double RocheSampleSpawnFactor
	{
		get
		{
			return rocheSampleSpawnFactor;
		}
		set
		{
			if (value < 0.0)
			{
				throw new ArgumentOutOfRangeException("Factor must be positive");
			}
			rocheSampleSpawnFactor = value;
		}
	}

	public double RocheSampleEscapeFactor
	{
		get
		{
			return rocheSampleEscapeFactor;
		}
		set
		{
			if (value < 0.0)
			{
				throw new ArgumentOutOfRangeException("Factor must be positive");
			}
			rocheSampleEscapeFactor = value;
		}
	}

	public double RocheSampleDelayBeforeCollisionFactor
	{
		get
		{
			return rocheSampleDelayBeforeCollisionFactor;
		}
		set
		{
			if (value < 0.0)
			{
				throw new ArgumentOutOfRangeException("Factor must be positive");
			}
			rocheSampleDelayBeforeCollisionFactor = value;
		}
	}

	[PropertyInfo("0b6b80c9cb9340e6b5e4944e185ea340", "")]
	public double RocheMaximalFragmentMassRatio
	{
		get
		{
			return rocheMaximalFragmentMassRatio;
		}
		set
		{
			if (value <= 0.0 || value > 1.0)
			{
				throw new ArgumentOutOfRangeException("The fragment mass ratio must be greater than 0 and not greater than 1");
			}
			rocheMaximalFragmentMassRatio = value;
		}
	}

	public int SampleFragmentCount
	{
		get
		{
			return numSampleFragments;
		}
		set
		{
			if (value < 0)
			{
				throw new ArgumentException("The roche helping acceleration fracting must be non zero");
			}
			numSampleFragments = value;
			rocheMaximalFragmentMassRatio = 1.0 / (double)numSampleFragments;
		}
	}

	public bool SpinFragmentationEnabled
	{
		get
		{
			return spinFragmentationEnabled;
		}
		set
		{
			spinFragmentationEnabled = value;
		}
	}

	public double SpinFragmentationSpawnRate
	{
		get
		{
			return spinFragmentationSpawnRate;
		}
		set
		{
			if (value < 0.0)
			{
				throw new ArgumentOutOfRangeException("Factor must be positive");
			}
			spinFragmentationSpawnRate = value;
		}
	}

	public double SpinFragmentationRandomVelocityFactor
	{
		get
		{
			return spinFragmentationRandomVelocityFactor;
		}
		set
		{
			if (value < 0.0)
			{
				throw new ArgumentOutOfRangeException("Factor must be positive");
			}
			spinFragmentationRandomVelocityFactor = value;
		}
	}

	public double TotalFragmentationDurationFactor => totalFragmentationDurationFactor;

	public double TotalFragmentationCoreMassFraction => totalFragmentationCoreMassFraction;

	public double TotalFragmentationCoreFractionThreshold => totalFragmentationCoreFractionThreshold;

	public double TotalFragmentationBudgetFactor => totalFragmentationBudgetFactor;

	public double TotalFragmentationFragmentDensityFactor => totalFragmentationFragmentDensityFactor;

	public double GasFragmentsDensityFactor
	{
		get
		{
			return gasFragmentsDensityFactor;
		}
		set
		{
			if (value < 0.0 || value > 1.0)
			{
				throw new ArgumentOutOfRangeException("Density factor must be greater than 0 and not greater than 1");
			}
			gasFragmentsDensityFactor = value;
		}
	}

	public double FrictionFragmentationSpawnFactor
	{
		get
		{
			return frictionFragmentationSpawnFactor;
		}
		set
		{
			if (value < 0.0)
			{
				throw new ArgumentOutOfRangeException("Factor must be positive");
			}
			frictionFragmentationSpawnFactor = value;
		}
	}

	public double FrictionFragmentationSweeping
	{
		get
		{
			return frictionFragmentationSweeping;
		}
		set
		{
			if (value < 0.0)
			{
				throw new ArgumentOutOfRangeException("Factor must be positive");
			}
			frictionFragmentationSweeping = value;
		}
	}

	public bool BarycentricParenting
	{
		get
		{
			return baryCentricParenting;
		}
		set
		{
			baryCentricParenting = value;
			Debug.Log((object)("Changing barycentric parenting to " + value));
		}
	}

	public bool CollisionsEnabled
	{
		get
		{
			return collisionsEnabled;
		}
		set
		{
			collisionsEnabled = value;
		}
	}

	public bool ImpactFragmentationEnabled
	{
		get
		{
			return fragmentationEnabled;
		}
		set
		{
			fragmentationEnabled = value;
		}
	}

	public double AvailableFragments
	{
		get
		{
			return availableFragments;
		}
		set
		{
			if (value < 0.0)
			{
				throw new ArgumentOutOfRangeException("Available fragment count not be negative");
			}
			availableFragments = value;
		}
	}

	public bool MaintainCenterOfMass
	{
		get
		{
			return maintainCenterOfMass;
		}
		set
		{
			maintainCenterOfMass = value;
		}
	}

	public double CollisionDelayMultiplier
	{
		get
		{
			return collisionDelayMultiplier;
		}
		set
		{
			if (value < 0.0)
			{
				throw new ArgumentOutOfRangeException("Collision delay multiplier must not be less than zero");
			}
			collisionDelayMultiplier = value;
		}
	}

	public bool TotalFragmentationEnabled
	{
		get
		{
			return totalFragmentationEnabled;
		}
		set
		{
			totalFragmentationEnabled = value;
		}
	}

	public double NormalTangentialFrictionRatio => normalTangentialFrictionRatio;

	public double RollingFrictionMagnitude => rollingFrictionMagnitude;

	public double ShearCompensationFactor
	{
		get
		{
			return shearCompensationFactor;
		}
		set
		{
			if (value < 0.0 || value > 2.0)
			{
				throw new ArgumentOutOfRangeException("ShearCompensationFactor must be in the range [0..2]");
			}
			shearCompensationFactor = value;
		}
	}

	public double EjectionVelocityScalar
	{
		get
		{
			return ejectionVelocityScalar;
		}
		set
		{
			if (value <= 0.0 || value > 1.0)
			{
				throw new ArgumentOutOfRangeException("EjectionVelocityScalar must be in the range ]0..1]");
			}
			ejectionVelocityScalar = value;
		}
	}

	public double ShockWaveRangeMultiplier
	{
		get
		{
			return shockWaveRangeMultiplier;
		}
		set
		{
			if (value < 0.0)
			{
				throw new ArgumentOutOfRangeException("Shock wave range multiplier cannot be less than zero.");
			}
			shockWaveRangeMultiplier = value;
		}
	}

	public double TetherTolerance
	{
		get
		{
			return tetherTolerance;
		}
		set
		{
			if (value <= 0.0 || value > 1.0)
			{
				throw new ArgumentOutOfRangeException("The tolerance must be greater than zero and no greater than 1");
			}
			tetherTolerance = value;
		}
	}

	public double TidalLockingAndAccelerationFactor
	{
		get
		{
			return tidalLockingAndAccelerationFactor;
		}
		set
		{
			if (value < 0.0)
			{
				throw new ArgumentOutOfRangeException("Tidal locking factor must not be negative");
			}
			tidalLockingAndAccelerationFactor = value;
		}
	}

	public double TidalHeatingFactor
	{
		get
		{
			return tidalHeatingFactor;
		}
		set
		{
			if (value >= 0.0)
			{
				tidalHeatingFactor = value;
				return;
			}
			throw new ArgumentOutOfRangeException("Tidal heating factor must not be negative");
		}
	}

	public double FragmentSpinRateFactor
	{
		get
		{
			return fragmentSpinRateFactor;
		}
		set
		{
			fragmentSpinRateFactor = value;
		}
	}

	public double MaxSpinRateIncreasePerFrame => maxSpinRateIncreasePerFrame;
}
