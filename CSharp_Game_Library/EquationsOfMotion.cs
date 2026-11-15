using System;
using CosmosLogic.Math;

namespace Mathematics;

public static class Equations
{
	public static class Physics
	{
		public static class Gravity
		{
			public static class HolsappleParams
			{
				private const double phi = Math.PI / 4.0;

				private const double alpha = 0.7;

				private const double beta = 0.7;

				private const double beta2 = 0.48999999999999994;

				private const double beta4 = 0.24009999999999992;

				private static readonly double s = 2.0 * Math.Sin(Math.PI / 4.0) / (Math.Sqrt(3.0) * (3.0 * Math.Sin(Math.PI / 4.0)));

				public static readonly double C = Math.Pow(0.48999999999999994, 1.0 / 3.0) * Math.Sqrt(5.0 * (3.0 * s * 1.49 - Math.Sqrt(2.2503)) / (3.0 * s * s * Math.Pow(1.49, 2.0) - 1.0 + 0.48999999999999994 - 0.24009999999999992));

				public const double Kappa = 225000.0;
			}

			private const double oneOverSig = 1.1126500560536185E-17;

			public static double SurfaceGravityAcceleration(double mass, double radius, double G)
			{
				return G * mass / (radius * radius);
			}

			public static bool Orbit(double G, Vector3D otherVelocity, Vector3D thisVelocity, Vector3D otherPosition, Vector3D thisPosition, double otherMass, double thisMass, out double pericenter)
			{
				double apocenter;
				double semiMajorAxis;
				double eccentricity;
				double period;
				return Orbit(G, otherVelocity, thisVelocity, otherPosition, thisPosition, otherMass, thisMass, out apocenter, out pericenter, out semiMajorAxis, out eccentricity, out period);
			}

			public static Vector3D BaryCenter(Vector3D position0, Vector3D position1, double mass0, double mass1)
			{
				return (position0 * mass0 + position1 * mass1) / (mass0 + mass1);
			}

			public static bool Orbit(double G, Vector3D otherVelocity, Vector3D thisVelocity, Vector3D otherPosition, Vector3D thisPosition, double otherMass, double thisMass, out double apocenter, out double pericenter, out double semiMajorAxis, out double eccentricity, out double period)
			{
				Vector3D vector3D = (otherPosition * otherMass + thisPosition * thisMass) / (otherMass + thisMass);
				Vector3D vector3D2 = (otherVelocity * otherMass + thisVelocity * thisMass) / (otherMass + thisMass);
				Vector3D a = thisPosition - vector3D;
				Vector3D vector3D3 = thisVelocity - vector3D2;
				Vector3D vector3D4 = otherPosition - vector3D;
				double num = otherMass * a.LengthSquared / Math.Pow(a.Length + vector3D4.Length, 2.0) * G;
				Vector3D b = Vector3D.Cross(a, vector3D3);
				eccentricity = (Vector3D.Cross(vector3D3, b) / num - a.UnitDirection).Length;
				double num2 = b.LengthSquared / num;
				pericenter = num2 / (1.0 + eccentricity);
				apocenter = num2 / (1.0 - eccentricity);
				semiMajorAxis = num2 / (1.0 - eccentricity * eccentricity);
				period = Math.Sqrt(39.47841760435743 / num * semiMajorAxis * semiMajorAxis * semiMajorAxis);
				if (eccentricity >= 0.0)
				{
					return eccentricity < 1.0;
				}
				return false;
			}

			public static double Orbit(double G, Vector3D otherVelocity, Vector3D thisVelocity, Vector3D otherPosition, Vector3D thisPosition, double otherMass, double thisMass)
			{
				Vector3D vector3D = (otherPosition * otherMass + thisPosition * thisMass) / (otherMass + thisMass);
				Vector3D vector3D2 = (otherVelocity * otherMass + thisVelocity * thisMass) / (otherMass + thisMass);
				Vector3D a = thisPosition - vector3D;
				Vector3D vector3D3 = thisVelocity - vector3D2;
				Vector3D vector3D4 = otherPosition - vector3D;
				double num = a.Length + vector3D4.Length;
				double num2 = otherMass * a.LengthSquared / (num * num) * G;
				Vector3D b = Vector3D.Cross(a, vector3D3);
				double length = (Vector3D.Cross(vector3D3, b) / num2 - a.UnitDirection).Length;
				double num3 = b.LengthSquared / num2;
				_ = num3 / (1.0 + length);
				_ = num3 / (1.0 - length);
				double num4 = num3 / (1.0 - length * length);
				double result = Math.Sqrt(39.47842025756836 / num2 * num4 * num4 * num4);
				if (length >= 0.0 && length < 1.0)
				{
					return result;
				}
				return double.MaxValue;
			}

			public static double TidalForce(double mass0, double mass1, double distance, double diameter, double G)
			{
				return 2.0 * G * mass0 * mass1 / (distance * distance * distance) * diameter;
			}

			public static double MeanMotion(double semiMajorAxis, double mass0, double mass1, double G)
			{
				return Math.Sqrt(G * (mass0 + mass1) / Math.Pow(semiMajorAxis, 3.0));
			}

			public static double Acceleration(double mass, double distance, double G)
			{
				return (0.0 - G * mass) / (distance * distance);
			}

			public static double TidalLockingRate(double distance, double radius, double density, double relativeAngularVelocity, double attractorMass, double G)
			{
				double num = 1.0;
				double num2 = 30000000000.0;
				double num3 = Sphere.Volume(radius) * density;
				double num4 = G * num3 / Math.Pow(radius, 2.0);
				double num5 = 1.5 / (1.0 + 19.0 * num2 / (2.0 * density * num4 * radius));
				return 3.0 * G * Math.Pow(attractorMass, 2.0) * num5 * Math.Pow(radius, 5.0) / (relativeAngularVelocity * Math.Pow(distance, 6.0) * Sphere.MomentOfInertia(radius, num3) * num);
			}

			public static double TidalLockingTimeSimple(double distance, double radius, double mass, double attractorMass, double G)
			{
				double num = 30000000000.0;
				return 6.0 * Math.Pow(distance, 6.0) * radius * num / (mass * attractorMass * attractorMass) * 10000000000.0 * 365.0 * 24.0 * 3600.0;
			}

			public static double RocheLobeRadius(double thisMass, double otherMass, double separation)
			{
				double num = thisMass / otherMass;
				if (num > 1000000.0)
				{
					return double.MaxValue;
				}
				double num2 = math.pow(num, 2.0 / 3.0);
				double num3 = math.pow(num, 1.0 / 3.0);
				return 0.866 * separation * (0.49 * num2) / (0.6 * num2 + math.log(1.0 + num3));
			}

			public static double DistanceFromFreeFallTimeWithNoAcceleration(double radiusA, double radiusB, double relativeSpeedTowardEachOther, double targetTime)
			{
				return radiusA + radiusB + relativeSpeedTowardEachOther * targetTime;
			}

			public static double DistanceAndInitialVelocity(double massA, double radiusA, double radiusB, ref double initialVelocityB, double finalVelocityB, double targetTime, double G)
			{
				double num = 0.0;
				double num2 = radiusA + radiusB;
				double num3 = 0.0 - finalVelocityB;
				double num4 = targetTime / 10000.0;
				for (double num5 = 0.0; num5 < targetTime; num5 += num4)
				{
					num3 -= Acceleration(massA, num2 - num, G) * num4;
					num2 += (0.0 - num3) * num4;
				}
				initialVelocityB = 0.0 - num3;
				return num2;
			}

			public static double DistanceAndInitialVelocity(double massA, double radiusA, double massB, double radiusB, ref double initialVelocityB, double finalVelocityB, double targetTime, double G)
			{
				double num = 0.0;
				double num2 = radiusA + radiusB;
				double num3 = 0.0;
				double num4 = 0.0 - finalVelocityB;
				double num5 = targetTime / 10000.0;
				for (double num6 = 0.0; num6 < targetTime; num6 += num5)
				{
					num4 -= Acceleration(massA, num2 - num, G) * num5;
					num3 += Acceleration(massB, num2 - num, G) * num5;
					num -= num3 * num5;
					num2 += (0.0 - num4) * num5;
				}
				initialVelocityB = 0.0 - num4 + num3;
				return num2 - num;
			}

			public static double DistanceFromFreeFallTime(double massA, double radiusA, double massB, double radiusB, double relativeSpeedtowardEachOther, double targetTime, double acceptableError, double G, int maxIterations)
			{
				return radiusA + radiusB + relativeSpeedtowardEachOther * targetTime;
			}

			public static double EscapeVelocity(double M, double r, double G)
			{
				return Math.Sqrt(2.0 * G * M / r);
			}

			public static double FreeFallTime(double density, double G)
			{
				return Math.Sqrt(Math.PI * 3.0 / (32.0 * G * density));
			}

			public static double FreeFallTime(double mass, double velocity, double collisionDistance, double initialDistance, double G, double dt)
			{
				double num = initialDistance;
				double num2 = 0.0 - velocity;
				double num3 = 0.0;
				do
				{
					num += num2 * dt;
					num2 += Acceleration(mass, num, G) * dt;
					num3 += dt;
				}
				while (num > collisionDistance);
				return num3;
			}

			public static double MaximalFlightHeight(double velocity, double initialHeight, double mass, double G = 6.6740831E-11)
			{
				double num = 0.5 * velocity * velocity;
				double num2 = (0.0 - G) * mass / initialHeight;
				double num3 = (0.0 - G) * mass / (num + num2);
				if (num3 > 0.0)
				{
					return num3 - initialHeight;
				}
				return double.PositiveInfinity;
			}

			public static double VelocityToReachAltitude(double attractorMass, double initialAltitude, double finalAltitude, double G = 6.67E-11)
			{
				double num = (0.0 - G) * attractorMass / initialAltitude;
				double num2 = (0.0 - G) * attractorMass / finalAltitude - num;
				return Math.Sqrt(2.0 * num2);
			}

			public static double PotentialEnergy(double M, double m, double r, double G)
			{
				return (0.0 - G * M * m) / r;
			}

			public static double SchwarzschildRadius(double m, double G)
			{
				return 2.0 * G * m * 1.1126500560536185E-17;
			}

			public static double CriticalSpinRate(double mass, double radius, double G = 6.6740831E-11)
			{
				return Math.Sqrt(G * mass / (radius * radius * radius));
			}

			public static double CriticalSpinRate(double density, double G = 6.6740831E-11)
			{
				return Math.Sqrt(Math.PI * 4.0 * G * density / 3.0);
			}

			public static double MaxSpinRate(double distanceFromAxis, double mass, double density, double G)
			{
				double num = 225000.0 / Math.Sqrt(distanceFromAxis);
				double val = HolsappleParams.C * Math.Sqrt(num / density) / distanceFromAxis;
				return Math.Max(CriticalSpinRate(mass, distanceFromAxis, G), val);
			}
		}

		public static class Tides
		{
			public static double HeatingRate(double parentMass, double radius, double distance, double period, double eccentricity, double density, double G)
			{
				double num = 30000000000.0;
				double num2 = Sphere.Volume(radius) * density;
				double num3 = G * num2 / Math.Pow(radius, 2.0);
				double num4 = 1.5 / (1.0 + 19.0 * num / (2.0 * density * num3 * radius));
				double num5 = Math.PI * 2.0 / period;
				return num4 * 10.5 * G * MathS.Sqr(parentMass) * Math.Pow(radius, 5.0) * num5 * MathS.Sqr(eccentricity) / Math.Pow(distance, 6.0);
			}
		}

		public static class Impact
		{
			public struct FragmentVolumeRange(double totalVolume, double minVolume, double maxVolume, double exponent)
			{
				public double TotalVolume = totalVolume;

				public double MinVolume = minVolume;

				public double MaxVolume = maxVolume;

				public double Exponent = exponent;

				public double Generate(double sample)
				{
					if (Exponent == 0.0)
					{
						return MathS.Lerp(MinVolume, MaxVolume, sample);
					}
					return Math.Pow((Math.Pow(MaxVolume, Exponent + 1.0) - Math.Pow(MinVolume, Exponent + 1.0)) * sample + Math.Pow(MinVolume, Exponent + 1.0), 1.0 / (Exponent + 1.0));
				}

				public double AverageVolume()
				{
					if (Exponent == 0.0)
					{
						return 0.5 * (MinVolume + MaxVolume);
					}
					return (Exponent + 1.0) / (Exponent + 2.0) * (Math.Pow(MaxVolume, Exponent + 2.0) - Math.Pow(MinVolume, Exponent + 2.0)) / (Math.Pow(MaxVolume, Exponent + 1.0) - Math.Pow(MinVolume, Exponent + 1.0));
				}

				public void UpdateTotalVolume(double newVolume)
				{
					double num = newVolume / TotalVolume;
					MinVolume *= num;
					MaxVolume *= num;
					TotalVolume = newVolume;
				}
			}

			public static double EstimatedCraterDiameter(double impactEnergy)
			{
				return 765.0 * Math.Pow(impactEnergy / 4184000000000000.0, 75.0 / 255.0);
			}

			public static double EstimatedCraterDiameter(double primaryMass, double primaryRadius, double secondaryMass, double secondaryRadius, double relativeVelocity)
			{
				double x = 6.6740831E-11 * primaryMass / (primaryRadius * primaryRadius);
				double num = primaryMass / Sphere.Volume(primaryRadius);
				double num2 = secondaryMass / Sphere.Volume(secondaryRadius);
				return 1.161 * Math.Pow(num2 / num, 0.3333) * Math.Pow(2.0 * secondaryRadius, 0.78) * Math.Pow(relativeVelocity, 0.44) * Math.Pow(x, -0.22);
			}

			public static double MeltVolume(double impactEnergy, double impactAngle)
			{
				return 8.9E-12 * impactEnergy * Math.Cos(impactAngle);
			}

			public static double ImpactEnergy(double primaryRadius, double primaryDensity, double secondaryRadius, double secondaryDensity, double impactSpeed)
			{
				return 0.5 * secondaryDensity * Sphere.Volume(secondaryRadius) * impactSpeed * impactSpeed / (primaryDensity * Sphere.Volume(primaryRadius));
			}

			public static double CriticalSpecificImpactEnergy(double primaryRadius, double primaryDensity)
			{
				double x = 100.0 * primaryRadius;
				double num = 0.001 * primaryDensity;
				double num2 = 90000000.0 * Math.Pow(x, -0.36) + 0.5 * num * Math.Pow(x, 1.36);
				return 0.0001 * num2;
			}

			public static double LargestRemnantMass(double totalMass, double specificEnergy, double primaryRadius, double primaryDensity)
			{
				double num = CriticalSpecificImpactEnergy(primaryRadius, primaryDensity);
				if (specificEnergy < num)
				{
					return totalMass * (-0.5 * (specificEnergy / num - 1.0) + 0.5);
				}
				double num2 = specificEnergy / num;
				double val = totalMass * (-0.35 * (num2 - 1.0) + 0.5);
				double val2 = 0.008 * num2 * Math.Exp(0.0 - Math.Min(num2 * num2 / 16.0, 40.0)) * totalMass;
				return Math.Max(val, val2);
			}

			public static double TotalFragmentVolume(double primaryRadius, double primaryMass, double secondaryMass, double impactVelocity)
			{
				double num = primaryMass / Sphere.Volume(primaryRadius);
				double num2 = primaryMass + secondaryMass;
				double specificEnergy = 0.5 * secondaryMass * impactVelocity * impactVelocity / num2;
				double num3 = LargestRemnantMass(num2, specificEnergy, primaryRadius, num);
				return Math.Max(num2 - num3, secondaryMass * (1.0 - secondaryMass / primaryMass)) / num;
			}

			public static FragmentVolumeRange FragmentDistribution(double totalFragmentVolume, int expectedFragmentCount)
			{
				double num = totalFragmentVolume / (double)expectedFragmentCount;
				return new FragmentVolumeRange(totalFragmentVolume, 0.05 * num, 1.95 * num, 0.0);
			}

			public static double ShockwaveSpeed(double impactSpeed)
			{
				return 0.8 * impactSpeed;
			}

			public static double ShockwaveSpeed(double impactSpeed, double softness)
			{
				double val = MathS.Lerp(1480.0, 5120.0, softness);
				return Math.Max(ShockwaveSpeed(impactSpeed), val);
			}

			public static double FragmentTemperature(double specificKineticEnergy, double primaryDensity)
			{
				double num = specificKineticEnergy * 0.05;
				double num2 = primaryDensity / 3500.0;
				return Math.Min(num / 500.0 * num2, 2400.0);
			}
		}

		public static class Fluid
		{
			public static double ReynoldsNumber(double velocity, double density, double radius, double viscosity)
			{
				return density * velocity * radius * 2.0 / viscosity;
			}

			public static double RamPressure(Vector3D velocity, double density)
			{
				return 0.5 * density * velocity.LengthSquared;
			}

			public static Vector3D DragForce(Vector3D flowVelocity, double density, double radius)
			{
				double num = 0.5;
				double num2 = Math.PI * radius * radius;
				return 0.5 * density * num2 * flowVelocity * flowVelocity.Length * num;
			}
		}

		public static class Planet
		{
			public static double CoreTemperature(double radius, double density)
			{
				double num = radius * 0.001;
				return 5700.0 * Math.Pow(num / 6378.0, 0.91) * Math.Pow(density / 5513.0, 0.65);
			}

			public static double MomentOfInertia(double radius, double equatorialRadius, double spinRate, double mass, double G)
			{
				double num = Math.Pow(radius, 3.0) / Math.Pow(equatorialRadius, 2.0);
				double num2 = (equatorialRadius - num) / equatorialRadius;
				double num3 = Math.Pow(spinRate, 2.0) * Math.Pow(equatorialRadius, 3.0) / (G * mass);
				double num4 = 5.0 * num3 / (2.0 * num2) - 2.0;
				return mass * Math.Pow(equatorialRadius, 2.0) * 0.667 * (1.0 - 0.4 * Math.Sqrt(1.0 + num4));
			}
		}

		public static class Explosion
		{
			public static double ShockVelocity(double energy, double radius, double density, double beta = 0.75)
			{
				return 0.4 * Math.Sqrt(Math.Pow(beta, 5.0) * energy / (density * radius * radius * radius));
			}

			public static double ExplosionEnergy(double velocity, double radius, double density, double beta = 0.75)
			{
				return 6.25 * velocity * velocity * Math.Pow(beta, -5.0) * density * radius * radius * radius;
			}

			public static double ShockHeating(double shockVelocity)
			{
				return 2000.0 * Math.Pow(shockVelocity / 20000.0, 2.0);
			}
		}

		public static Vector3D AngularOrbitMomentumWorld(double mass, Vector3D velocity, Vector3D r)
		{
			return Vector3D.Cross(r, velocity) * mass;
		}

		public static Vector3D LinearMomentum(double mass, Vector3D velocity)
		{
			return mass * velocity;
		}

		public static double RelativisticKineticEnergy(double mass, double velocity, double c)
		{
			double num = c * c;
			double num2 = Math.Sqrt(Math.Min(num * 0.998, velocity * velocity));
			if (num2 > 0.01 * c)
			{
				return mass * num / Math.Sqrt(1.0 - Math.Pow(num2 / c, 2.0)) - mass * num;
			}
			return 0.5 * mass * velocity * velocity;
		}
	}

	public static class Sphere
	{
		public static class Cap
		{
			public static double Area(double radius, double capHeight)
			{
				return Math.PI * 2.0 * radius * capHeight;
			}

			public static double CenterOfMass(double radius, double capHeight)
			{
				if (capHeight <= radius)
				{
					return 3.0 * Math.Pow(2.0 * radius - capHeight, 2.0) / (4.0 * (3.0 * radius - capHeight));
				}
				double num = 3.0 * radius * radius / (8.0 * radius);
				double num2 = Volume(radius, radius);
				double num3 = (0.0 - 3.0 * Math.Pow(2.0 * radius - (radius * 2.0 - capHeight), 2.0)) / (4.0 * (3.0 * radius - (radius * 2.0 - capHeight)));
				double num4 = Volume(radius, radius * 2.0 - capHeight);
				double num5 = Sphere.Volume(radius) - num2 - num4;
				double num6 = (0.0 - (num * num2 + num3 * num4)) / num5;
				return (num * num2 + num6 * num5) / (num2 + num5);
			}

			public static double Volume(double radius, double capHeight)
			{
				return Math.PI * capHeight * capHeight * (3.0 * radius - capHeight) / 3.0;
			}
		}

		public static class Lens
		{
			public static bool HasIntersection(Vector3D position1, double radius1, Vector3D position2, double radius2)
			{
				double lengthSquared = (position1 - position2).LengthSquared;
				double num = (radius1 + radius2) * (radius1 + radius2);
				double num2 = Math.Abs(radius1 - radius2) * Math.Abs(radius1 - radius2);
				if (lengthSquared <= num2 || lengthSquared >= num)
				{
					return false;
				}
				return true;
			}

			public static void CapHeights(Vector3D position1, double radius1, Vector3D position2, double radius2, out double h1, out double h2)
			{
				double length = (position1 - position2).Length;
				if (length >= radius1 + radius2)
				{
					h1 = (h2 = 0.0);
					return;
				}
				if (length + Math.Min(radius1, radius2) <= Math.Max(radius1, radius2))
				{
					if (radius1 > radius2)
					{
						h1 = 0.0;
						h2 = 2.0 * radius2;
					}
					else
					{
						h1 = 2.0 * radius1;
						h2 = 0.0;
					}
					return;
				}
				Vector3D vector3D = Center(position1, radius1, position2, radius2);
				Vector3D a = vector3D - position1;
				Vector3D a2 = vector3D - position2;
				h1 = radius1 - a.Length;
				h2 = radius2 - a2.Length;
				if (Vector3D.Dot(a, position2 - position1) < 0.0)
				{
					h1 = radius1 * 2.0 - h1;
				}
				if (Vector3D.Dot(a2, position1 - position2) < 0.0)
				{
					h2 = radius2 * 2.0 - h2;
				}
			}

			public static Vector3D Center(Vector3D position1, double radius1, Vector3D position2, double radius2)
			{
				double length = (position1 - position2).Length;
				double num = (radius1 * radius1 + length * length - radius2 * radius2) / (2.0 * radius1 * length) * radius1;
				return position1 + (position2 - position1).UnitDirection * num;
			}

			public static Vector3D CenterOfMass(Vector3D position1, double radius1, Vector3D position2, double radius2)
			{
				CapHeights(position1, radius1, position2, radius2, out var h, out var h2);
				double num = Cap.Volume(radius1, h) * 1.0;
				double num2 = Cap.Volume(radius2, h2) * 1.0;
				Vector3D vector3D = position1 + (position2 - position1).UnitDirection * Cap.CenterOfMass(radius1, h);
				Vector3D vector3D2 = position2 + (position1 - position2).UnitDirection * Cap.CenterOfMass(radius2, h2);
				return (vector3D * num + vector3D2 * num2) / (num + num2);
			}

			public static double Radius(double radius1, double radius2, double distance)
			{
				double num = (radius1 * radius1 + distance * distance - radius2 * radius2) / (2.0 * radius1 * distance);
				if (Math.Abs(num) > 1.0)
				{
					return 0.0;
				}
				return Math.Sin(Math.Acos(num)) * radius1;
			}

			public static void Tangent(Vector3D position1, double radius1, Vector3D position2, double radius2, Vector3D axis, ref Vector3D t1, ref Vector3D t2)
			{
				if (HasIntersection(position1, radius1, position2, radius2))
				{
					double length = (position1 - position2).Length;
					double num = Radius(radius1, radius2, length);
					Vector3D vector3D = Center(position1, radius1, position2, radius2);
					Vector3D b = vector3D - position1;
					Vector3D vector3D2 = vector3D + Vector3D.Cross(axis, b).UnitDirection * num;
					Vector3D v = vector3D2 - position1;
					Vector3D v2 = vector3D2 - position2;
					t1 = Vector3D.RotateAround(v, axis, Math.PI / 2.0).UnitDirection;
					t2 = Vector3D.RotateAround(v2, axis, -Math.PI / 2.0).UnitDirection;
				}
			}

			public static Vector3D Tangent(Vector3D position1, double radius1, Vector3D position2, double radius2, Vector3D axis)
			{
				double length = (position1 - position2).Length;
				double num = Radius(radius1, radius2, length);
				Vector3D vector3D = Center(position1, radius1, position2, radius2);
				Vector3D b = vector3D - position1;
				Vector3D vector3D2 = vector3D + Vector3D.Cross(axis, b).UnitDirection * num;
				Vector3D v = vector3D2 - position1;
				Vector3D v2 = vector3D2 - position2;
				Vector3D unitDirection = Vector3D.RotateAround(v, axis, Math.PI / 2.0).UnitDirection;
				Vector3D unitDirection2 = Vector3D.RotateAround(v2, axis, -Math.PI / 2.0).UnitDirection;
				return (unitDirection + unitDirection2).UnitDirection;
			}

			public static double Volume(double radius1, double radius2, double distance)
			{
				if (distance >= radius1 + radius2)
				{
					return 0.0;
				}
				return Math.PI * Math.Pow(radius1 + radius2 - distance, 2.0) * (distance * distance + 2.0 * distance * radius2 - 3.0 * radius2 * radius2 + 2.0 * distance * radius1 + 6.0 * radius2 * radius1 - 3.0 * radius1 * radius1) / (12.0 * distance);
			}
		}

		public static class Shell
		{
			public static double MomentOfInertial(double innerRadius, double outerRadius, double mass)
			{
				return 0.4 * mass * (Math.Pow(outerRadius, 5.0) - Math.Pow(innerRadius, 5.0)) / (Math.Pow(outerRadius, 3.0) - Math.Pow(innerRadius, 3.0));
			}
		}

		public static Vector3D SurfacePointWorldVelocity(Vector3D r, Vector3D angularVelocity, double radius, Quaternion orientation, Vector3D centerVelocity)
		{
			r.SetLength(radius);
			return SurfacePointWorldVelocity(r, angularVelocity, orientation, centerVelocity);
		}

		public static Vector3D SurfacePointWorldVelocity(Vector3D r, Vector3D angularVelocity, Quaternion orientation, Vector3D centerVelocity)
		{
			return Vector3D.Cross(orientation.Rotate(angularVelocity), r) + centerVelocity;
		}

		public static Vector3D AngularSpinMomentumObject(double mass, double radius, Vector3D angularVelocity)
		{
			return MomentOfInertia(radius, mass) * angularVelocity;
		}

		public static Vector3D AngularSpinMomentumWorld(double mass, double radius, Vector3D angularVelocity, Quaternion orientation)
		{
			return orientation.Rotate(AngularSpinMomentumObject(mass, radius, angularVelocity));
		}

		public static double Area(double radius)
		{
			return Math.PI * 4.0 * radius * radius;
		}

		public static double KineticEnergy(Vector3D velocity, Vector3D angularVelocity, double mass, double radius)
		{
			return 0.5 * (mass * velocity.LengthSquared + MomentOfInertia(radius, mass) * angularVelocity.LengthSquared);
		}

		public static double KineticEnergyLinear(Vector3D velocity, double mass)
		{
			return 0.5 * mass * velocity.LengthSquared;
		}

		public static double KineticEnergyRotational(Vector3D angularVelocity, double momentOfInertia)
		{
			return 0.5 * momentOfInertia * angularVelocity.LengthSquared;
		}

		public static double MomentOfInertia(double radius, double mass)
		{
			return 0.4 * mass * radius * radius;
		}

		public static bool PointInside(Vector3D center, double radius, Vector3D point)
		{
			return (center - point).LengthSquared < radius * radius;
		}

		public static double PotentialEnergy(double mass, double radius, double G)
		{
			return -3.0 * G * mass * mass / (5.0 * radius);
		}

		public static double Pressure(double r, double r0, double density, double G)
		{
			if (r <= r0)
			{
				return Math.PI * 2.0 / 3.0 * G * MathS.Sqr(density) * (r0 * r0 - r * r);
			}
			return 0.0;
		}

		public static double RadiusFromVolume(double volume)
		{
			return Math.Pow(3.0 * volume * (1.0 / (4.0 * Math.PI)), 0.33333333333);
		}

		public static double Volume(double radius)
		{
			return Math.PI * 4.0 * (Math.Pow(radius, 3.0) * 0.3333333333);
		}

		public static double VolumeOverlappingSpheres(double radius1, double radius2, double distance)
		{
			if (distance >= radius1 + radius2)
			{
				return Volume(radius1) + Volume(radius2);
			}
			if (distance < Math.Max(radius1, radius2) - Math.Min(radius1, radius2))
			{
				return Volume(Math.Max(radius1, radius2));
			}
			return Volume(radius1) + Volume(radius2) - Lens.Volume(radius1, radius2, distance);
		}

		public static double ArcLengthFromChordLength(double sphereRadius, double chordLength)
		{
			double d = chordLength / (2.0 * sphereRadius);
			return 2.0 * Math.Asin(d) * sphereRadius;
		}

		public static bool Touching(Vector3D position1, double radius1, Vector3D position2, double radius2)
		{
			double lengthSquared = (position2 - position1).LengthSquared;
			double num = radius1 + radius2;
			double num2 = num * num;
			return lengthSquared <= num2;
		}
	}

	public static class Transform
	{
		public static double Map(double value, SimpleRange<double> range)
		{
			if (value <= range.Min)
			{
				return 0.0;
			}
			if (value >= range.Max)
			{
				return 1.0;
			}
			value -= range.Min;
			value /= range.Max - range.Min;
			return value;
		}
	}

	public static double LinearMapping(double from0, double from1, double to0, double to1, double value, bool clamp = false)
	{
		double num = (value - from0) / (from1 - from0) * (to1 - to0) + to0;
		if (clamp)
		{
			double num2 = Math.Max(to0, to1);
			double num3 = Math.Min(to0, to1);
			if (num > num2)
			{
				num = num2;
			}
			else if (num < num3)
			{
				num = num3;
			}
		}
		return num;
	}

	public static double QuadraticMapping(double from0, double from1, double from2, double to0, double to1, double to2, double value)
	{
		//IL_0027: Unknown result type (might be due to invalid IL or missing references)
		//IL_0033: Unknown result type (might be due to invalid IL or missing references)
		//IL_0038: Unknown result type (might be due to invalid IL or missing references)
		//IL_003d: Unknown result type (might be due to invalid IL or missing references)
		//IL_003e: Unknown result type (might be due to invalid IL or missing references)
		//IL_0043: Unknown result type (might be due to invalid IL or missing references)
		double3x3 val = new double3x3(from0 * from0, from0, 1.0, from1 * from1, from1, 1.0, from2 * from2, from2, 1.0);
		double3 val2 = new double3(to0, to1, to2);
		double3 val3 = math.mul(math.inverse(val), val2);
		return val3[0] * value * value + val3[1] * value + val3[2];
	}

	public static Vector3D AngularVelocity(Vector3D thisPosition, Vector3D thisVelocity, Vector3D otherPosition, Vector3D otherVelocity)
	{
		Vector3D b = otherVelocity - thisVelocity;
		Vector3D a = otherPosition - thisPosition;
		return Vector3D.Cross(a, b) / a.LengthSquared;
	}
}
