using System;
using CosmosLogic.Math;

namespace NBody;

public static class Integrator
{
	public enum AccelerationDerivaties
	{
		Acceleration,
		Jerk,
		Snap,
		Crackle
	}

	public struct IntegrationContext
	{
		public struct Tolerances(double gravity, double orbit, double strongestAttractor, double collision)
		{
			public double gravity = gravity;

			public double orbit = orbit;

			public double strongestAttractor = strongestAttractor;

			public double collision = collision;

			public unsafe double this[int index]
			{
				get
				{
					fixed (Tolerances* ptr = &this)
					{
						return ((double*)ptr)[index];
					}
				}
				set
				{
					fixed (Tolerances* ptr = &this)
					{
						((double*)ptr)[index] = value;
					}
				}
			}

			public static double4 operator *(double a, Tolerances b)
			{
				
				
				return a * new double4(b.gravity, b.orbit, b.strongestAttractor, b.collision);
			}

			public static double4 operator *(Tolerances a, double b)
			{
				
				
				return b * new double4(a.gravity, a.orbit, a.strongestAttractor, a.collision);
			}

			public static double4 operator *(double4 a, Tolerances b)
			{
				
				
				
				return a * new double4(b.gravity, b.orbit, b.strongestAttractor, b.collision);
			}

			public static double4 operator *(Tolerances a, double4 b)
			{
				
				
				
				return b * new double4(a.gravity, a.orbit, a.strongestAttractor, a.collision);
			}
		}

		public bool adaptiveTimestep;

		public bool enhancedOrbitStability;

		public double globalTolerance;

		public double baseToleranceScaling;

		public Tolerances tolerances;

		public double orbitEccentricityThreshold;

		public TimeStepData.TimeStepModes timestepMode;

		public TimeStepData.TimeStepClampingModes timestepClampingMode;

		public double clampingFactor;

		public double4 ScaledTolerances
		{
			get
			{
				
				return globalTolerance * baseToleranceScaling * tolerances;
			}
		}

		public IntegrationContext(bool adaptiveTimestep, TimeStepData.TimeStepModes timeStepMode, TimeStepData.TimeStepClampingModes timeStepClampingModes, double clampingFactor, bool enhancedOrbitStability, double globalTolerance, double baseToleranceScaling, double gravityTolerance, double orbitTolerance, double strongestAttractorTolerance, double collisionTolerance, double eccentricityThreshold)
		{
			this.adaptiveTimestep = adaptiveTimestep;
			timestepMode = timeStepMode;
			timestepClampingMode = timeStepClampingModes;
			this.clampingFactor = clampingFactor;
			this.enhancedOrbitStability = enhancedOrbitStability;
			this.globalTolerance = globalTolerance;
			this.baseToleranceScaling = baseToleranceScaling;
			tolerances = new Tolerances(gravityTolerance, orbitTolerance, strongestAttractorTolerance, collisionTolerance);
			orbitEccentricityThreshold = eccentricityThreshold;
		}
	}

	public enum ID
	{
		[EnumInfo("1b8434b76f2044f7aa6251227908f97c", "")]
		EulerExplicit = 1,
		[EnumInfo("fb01ae67d50c4555a53af791a79a6526", "")]
		EulerSemiImplicit = 2,
		[EnumInfo("846e4c7c2e6042088b527d24184ac4a2", "")]
		VelocityVerlet = 3,
		[EnumInfo("a45b4f11c87f426b89fabb9a3e1e3b31", "")]
		RK2 = 4,
		[EnumInfo("47d5c8e5f2ed4f84982e5497523b7170", "")]
		ForrestRuth = 6,
		[EnumInfo("235e9efebcc3410ab9c88371a9e58537", "")]
		PEFRL = 7,
		[EnumInfo("d5fe6945d4fa4de2901fa6bcb8be54f2", "")]
		Hermite = 8
	}

	public static class Buffer
	{
		public const int From = 0;

		public const int To = 1;

		public const int StateCount = 2;
	}

	public static string Description(ID integratorId)
	{
		return integratorId switch
		{
			ID.EulerExplicit => "First order explicit integrator. Very simple and not very accurate or stable", 
			ID.EulerSemiImplicit => "First order semi implicit integrator. Very simple and not very accurate, but slightly stable.", 
			ID.VelocityVerlet => "Second order symplectic integrator. Somewhat simple, somewhat accurate and somewhat stable.", 
			ID.RK2 => "Second order integrator. Slight improvement over explicit Euler, but still rather unstable.", 
			ID.ForrestRuth => "Fourth order symplectic integrator which is quite accurate and stable", 
			ID.PEFRL => "Fourth order symplectic integrator which is quite accurate and very stable", 
			ID.Hermite => "Fourth, Sixth and Eighth order iterative predict/correct integrater, which is very accurate and very stable", 
			_ => throw new NotSupportedException("Unknown integrator id in Integraton.Description"), 
		};
	}

	public static bool LeavesPositionBasedAcceleration(ID integratorId)
	{
		return integratorId switch
		{
			ID.EulerExplicit => false, 
			ID.EulerSemiImplicit => true, 
			ID.VelocityVerlet => true, 
			ID.RK2 => false, 
			ID.ForrestRuth => true, 
			ID.PEFRL => true, 
			ID.Hermite => false, 
			_ => throw new NotSupportedException("Unknown integrator id in Integraton.LeavesPositionBasedAcceleration"), 
		};
	}

	public static int Order(ID integratorId)
	{
		return integratorId switch
		{
			ID.EulerExplicit => 1, 
			ID.EulerSemiImplicit => 1, 
			ID.VelocityVerlet => 2, 
			ID.RK2 => 2, 
			ID.ForrestRuth => 4, 
			ID.PEFRL => 4, 
			ID.Hermite => 8, 
			_ => throw new NotSupportedException("Unknown integrator id in Integraton.Order"), 
		};
	}

	public static string OrderString(int order)
	{
		return order switch
		{
			1 => "1st", 
			2 => "2nd", 
			4 => "4th", 
			6 => "6th", 
			8 => "8th", 
			_ => throw new NotSupportedException("Unknown integrator id in Integraton.Order"), 
		};
	}

	public static double BaseTolerance(ID integratorId)
	{
		return integratorId switch
		{
			ID.EulerExplicit => 0.5, 
			ID.EulerSemiImplicit => 0.5, 
			ID.VelocityVerlet => 1.5, 
			ID.RK2 => 1.0, 
			ID.ForrestRuth => 5.0, 
			ID.PEFRL => 7.5, 
			ID.Hermite => 10.0, 
			_ => throw new NotSupportedException("Unknown integrator id in Integraton.Order"), 
		};
	}

	public static double OrbitTolerance(ID integratorId)
	{
		return integratorId switch
		{
			ID.EulerExplicit => 1.0, 
			ID.EulerSemiImplicit => 1.0, 
			ID.VelocityVerlet => 1.0, 
			ID.RK2 => 0.5, 
			ID.ForrestRuth => 1.0, 
			ID.PEFRL => 1.0, 
			ID.Hermite => 3.0, 
			_ => throw new NotSupportedException("Unknown integrator id in Integraton.Order"), 
		};
	}

	public static int InternalGravitySteps(ID integratorId)
	{
		return integratorId switch
		{
			ID.EulerExplicit => 1, 
			ID.EulerSemiImplicit => 1, 
			ID.VelocityVerlet => 2, 
			ID.RK2 => 2, 
			ID.ForrestRuth => 4, 
			ID.PEFRL => 5, 
			ID.Hermite => 4, 
			_ => throw new NotSupportedException("Unknown integrator id in Integraton.Order"), 
		};
	}

	public static AccelerationDerivaties HighestDerivateUsedForSafeTimestep(ID integratorId)
	{
		return integratorId switch
		{
			ID.EulerExplicit => AccelerationDerivaties.Jerk, 
			ID.EulerSemiImplicit => AccelerationDerivaties.Jerk, 
			ID.VelocityVerlet => AccelerationDerivaties.Jerk, 
			ID.RK2 => AccelerationDerivaties.Jerk, 
			ID.ForrestRuth => AccelerationDerivaties.Jerk, 
			ID.PEFRL => AccelerationDerivaties.Jerk, 
			ID.Hermite => AccelerationDerivaties.Crackle, 
			_ => throw new NotSupportedException("Unknown integrator id in Integraton.Order"), 
		};
	}

	public static AccelerationDerivaties HighestDerivateUsedForIntegration(ID integratorId)
	{
		return integratorId switch
		{
			ID.EulerExplicit => AccelerationDerivaties.Acceleration, 
			ID.EulerSemiImplicit => AccelerationDerivaties.Acceleration, 
			ID.VelocityVerlet => AccelerationDerivaties.Acceleration, 
			ID.RK2 => AccelerationDerivaties.Acceleration, 
			ID.ForrestRuth => AccelerationDerivaties.Acceleration, 
			ID.PEFRL => AccelerationDerivaties.Acceleration, 
			ID.Hermite => AccelerationDerivaties.Crackle, 
			_ => throw new NotSupportedException("Unknown integrator id in Integraton.Order"), 
		};
	}

	public static string SafeTimeUsesDerivatives(ID integratorId)
	{
		return integratorId switch
		{
			ID.EulerExplicit => "Acceleration, Jerk", 
			ID.EulerSemiImplicit => "Acceleration, Jerk", 
			ID.VelocityVerlet => "Acceleration, Jerk", 
			ID.RK2 => "Acceleration, Jerk", 
			ID.ForrestRuth => "Acceleration, Jerk", 
			ID.PEFRL => "Acceleration, Jerk", 
			ID.Hermite => "Acceleration, Jerk, Snap, Crackle", 
			_ => throw new NotSupportedException("Unknown integrator id in Integraton.Order"), 
		};
	}

	public static string IntegratorUsesDerivatives(ID integratorId)
	{
		return integratorId switch
		{
			ID.EulerExplicit => "Acceleration", 
			ID.EulerSemiImplicit => "Acceleration", 
			ID.VelocityVerlet => "Acceleration", 
			ID.RK2 => "Acceleration", 
			ID.ForrestRuth => "Acceleration", 
			ID.PEFRL => "Acceleration", 
			ID.Hermite => "Acceleration, Jerk, Snap, Crackle", 
			_ => throw new NotSupportedException("Unknown integrator id in Integraton.Order"), 
		};
	}
}
