using System;
using KNN;
using KNN.Jobs;
using CosmosLogic.Performance;
using CosmosLogic.Data;
using CosmosLogic.Threading;
using CosmosLogic.Math;

namespace NBody.ComputationLayer;

public class CollisionDetector
{
	private struct BroadPhaseCollision
	{
		public int Body0;

		public int Body1;

		public int PrectionSteps;
	}

	[BurstCompile(/*Could not decode attribute arguments.*/)]
	private struct BruteForceBroadPhaseJob : IJobParallelFor
	{
		[ReadOnly]
		public NativeArray<double3> PositionBefore;

		[ReadOnly]
		public NativeArray<double3> PositionNow;

		[ReadOnly]
		public NativeArray<double> Radius;

		[ReadOnly]
		public NativeArray<BodyFlags> Flags;

		[ReadOnly]
		public int ProjectedStepCount;

		[WriteOnly]
		public ParallelWriter<BroadPhaseCollision> BroadPhaseCollisions;

		[ReadOnly]
		public int NumMajors;

		public void Execute(int bodyIndex)
		{
			//IL_001d: Unknown result type (might be due to invalid IL or missing references)
			//IL_0029: Unknown result type (might be due to invalid IL or missing references)
			//IL_002e: Unknown result type (might be due to invalid IL or missing references)
			//IL_0049: Unknown result type (might be due to invalid IL or missing references)
			//IL_0055: Unknown result type (might be due to invalid IL or missing references)
			//IL_005a: Unknown result type (might be due to invalid IL or missing references)
			//IL_005f: Unknown result type (might be due to invalid IL or missing references)
			//IL_0064: Unknown result type (might be due to invalid IL or missing references)
			//IL_00bc: Unknown result type (might be due to invalid IL or missing references)
			//IL_00c1: Unknown result type (might be due to invalid IL or missing references)
			//IL_00cd: Unknown result type (might be due to invalid IL or missing references)
			//IL_00d9: Unknown result type (might be due to invalid IL or missing references)
			//IL_0103: Unknown result type (might be due to invalid IL or missing references)
			//IL_0110: Unknown result type (might be due to invalid IL or missing references)
			//IL_0115: Unknown result type (might be due to invalid IL or missing references)
			//IL_011a: Unknown result type (might be due to invalid IL or missing references)
			//IL_011f: Unknown result type (might be due to invalid IL or missing references)
			//IL_0135: Unknown result type (might be due to invalid IL or missing references)
			//IL_0142: Unknown result type (might be due to invalid IL or missing references)
			//IL_0147: Unknown result type (might be due to invalid IL or missing references)
			//IL_0162: Unknown result type (might be due to invalid IL or missing references)
			//IL_0163: Unknown result type (might be due to invalid IL or missing references)
			//IL_0165: Unknown result type (might be due to invalid IL or missing references)
			if (Flags[bodyIndex].IsNotSet(BodyFlags.Collider))
			{
				return;
			}
			double num = math.length(PositionNow[bodyIndex] - PositionBefore[bodyIndex]);
			double3 val = 0.5 * (PositionNow[bodyIndex] + PositionBefore[bodyIndex]);
			int num2 = ((bodyIndex >= NumMajors) ? 1 : ProjectedStepCount);
			double num3 = (double)num2 * num + Radius[bodyIndex];
			for (int i = 0; i < NumMajors; i++)
			{
				bool2 val2 = new bool2(Flags[i].IsNotSet(BodyFlags.ColliderGroupMajor), Flags[i].IsNotSet(BodyFlags.Collider));
				if (!val2.x && !val2.y && (val2.x || i <= bodyIndex) && bodyIndex != i)
				{
					double3 val3 = 0.5 * (PositionNow[i] + PositionBefore[i]);
					double num4 = (double)num2 * 0.5 * math.length(PositionNow[i] - PositionBefore[i]) + Radius[i];
					double num5 = math.length(val - val3);
					double num6 = num3 + num4;
					if (num5 < num6)
					{
						BroadPhaseCollisions.Enqueue(new BroadPhaseCollision
						{
							Body0 = bodyIndex,
							Body1 = i,
							PrectionSteps = num2
						});
					}
				}
			}
		}
	}

	[BurstCompile(/*Could not decode attribute arguments.*/)]
	public struct BuildBroadPhaseTreeJob : IJob
	{
		private NativeList<KdNode> nodes;

		[ReadOnly]
		private NativeList<int> permutation;

		[ReadOnly]
		private NativeArray<double3> positions;

		[ReadOnly]
		private NativeArray<double3> velocities;

		[ReadOnly]
		private NativeArray<double> radii;

		[ReadOnly]
		private NativeArray<BodyFlags> flags;

		[ReadOnly]
		private NativeArray<double> dt;

		private double maxRelativeTravelDistance;

		public void Update(NativeList<KdNode> nodes, NativeList<int> permutation, NativeArray<double> radii, NativeArray<BodyFlags> flags, NativeArray<double3> positions, NativeArray<double3> velocities, NativeArray<double> dt, double maxRelativeTravelDistance)
		{
			//IL_0001: Unknown result type (might be due to invalid IL or missing references)
			//IL_0002: Unknown result type (might be due to invalid IL or missing references)
			//IL_0008: Unknown result type (might be due to invalid IL or missing references)
			//IL_0009: Unknown result type (might be due to invalid IL or missing references)
			//IL_000f: Unknown result type (might be due to invalid IL or missing references)
			//IL_0010: Unknown result type (might be due to invalid IL or missing references)
			//IL_0016: Unknown result type (might be due to invalid IL or missing references)
			//IL_0018: Unknown result type (might be due to invalid IL or missing references)
			//IL_001e: Unknown result type (might be due to invalid IL or missing references)
			//IL_0020: Unknown result type (might be due to invalid IL or missing references)
			//IL_0026: Unknown result type (might be due to invalid IL or missing references)
			//IL_0028: Unknown result type (might be due to invalid IL or missing references)
			//IL_002e: Unknown result type (might be due to invalid IL or missing references)
			//IL_0030: Unknown result type (might be due to invalid IL or missing references)
			this.nodes = nodes;
			this.permutation = permutation;
			this.radii = radii;
			this.flags = flags;
			this.positions = positions;
			this.velocities = velocities;
			this.dt = dt;
			this.maxRelativeTravelDistance = maxRelativeTravelDistance;
		}

		public void Execute()
		{
			BuildSubTree(0);
		}

		public void BuildSubTree(int nodeIndex)
		{
			ref KdNode reference = ref nodes.ElementAt(nodeIndex);
			if (reference.Leaf)
			{
				BuildLeaf(nodeIndex);
				return;
			}
			BuildSubTree(reference.PositiveChildIndex);
			BuildSubTree(reference.NegativeChildIndex);
			BuildInner(nodeIndex);
		}

		public void BuildLeaf(int nodeIndex)
		{
			//IL_0017: Unknown result type (might be due to invalid IL or missing references)
			//IL_001c: Unknown result type (might be due to invalid IL or missing references)
			//IL_002b: Unknown result type (might be due to invalid IL or missing references)
			//IL_0030: Unknown result type (might be due to invalid IL or missing references)
			//IL_0098: Unknown result type (might be due to invalid IL or missing references)
			//IL_009f: Unknown result type (might be due to invalid IL or missing references)
			//IL_00a4: Unknown result type (might be due to invalid IL or missing references)
			//IL_00ad: Unknown result type (might be due to invalid IL or missing references)
			//IL_00b2: Unknown result type (might be due to invalid IL or missing references)
			//IL_00b3: Unknown result type (might be due to invalid IL or missing references)
			//IL_00bf: Unknown result type (might be due to invalid IL or missing references)
			//IL_00c4: Unknown result type (might be due to invalid IL or missing references)
			//IL_00c6: Unknown result type (might be due to invalid IL or missing references)
			//IL_00cb: Unknown result type (might be due to invalid IL or missing references)
			//IL_00cc: Unknown result type (might be due to invalid IL or missing references)
			//IL_00d1: Unknown result type (might be due to invalid IL or missing references)
			//IL_00d6: Unknown result type (might be due to invalid IL or missing references)
			//IL_00df: Unknown result type (might be due to invalid IL or missing references)
			//IL_00e4: Unknown result type (might be due to invalid IL or missing references)
			//IL_00e5: Unknown result type (might be due to invalid IL or missing references)
			//IL_00f1: Unknown result type (might be due to invalid IL or missing references)
			//IL_00f6: Unknown result type (might be due to invalid IL or missing references)
			//IL_00f8: Unknown result type (might be due to invalid IL or missing references)
			//IL_00fd: Unknown result type (might be due to invalid IL or missing references)
			//IL_00fe: Unknown result type (might be due to invalid IL or missing references)
			//IL_0103: Unknown result type (might be due to invalid IL or missing references)
			//IL_0108: Unknown result type (might be due to invalid IL or missing references)
			//IL_010c: Unknown result type (might be due to invalid IL or missing references)
			//IL_0111: Unknown result type (might be due to invalid IL or missing references)
			//IL_0113: Unknown result type (might be due to invalid IL or missing references)
			//IL_0118: Unknown result type (might be due to invalid IL or missing references)
			//IL_011f: Unknown result type (might be due to invalid IL or missing references)
			//IL_0124: Unknown result type (might be due to invalid IL or missing references)
			//IL_0126: Unknown result type (might be due to invalid IL or missing references)
			//IL_012b: Unknown result type (might be due to invalid IL or missing references)
			ref KdNode reference = ref nodes.ElementAt(nodeIndex);
			reference.ColliderMin = new double3(double.MaxValue);
			reference.ColliderMax = new double3(double.MinValue);
			if (reference.Count == 0)
			{
				return;
			}
			for (int i = reference.Start; i < reference.End; i++)
			{
				int num = permutation[i];
				if (!flags[num].IsNotSet(BodyFlags.Collider) && !flags[num].IsSet(BodyFlags.ColliderGas))
				{
					double3 val = new double3(radii[num]);
					double3 val2 = CalculateTravel(num);
					double3 val3 = math.min(positions[num] - val, positions[num] + val2 - val);
					double3 val4 = math.max(positions[num] + val, positions[num] + val2 + val);
					reference.ColliderMin = math.min(reference.ColliderMin, val3);
					reference.ColliderMax = math.max(reference.ColliderMax, val4);
				}
			}
		}

		public void BuildInner(int nodeIndex)
		{
			//IL_0033: Unknown result type (might be due to invalid IL or missing references)
			//IL_0039: Unknown result type (might be due to invalid IL or missing references)
			//IL_003e: Unknown result type (might be due to invalid IL or missing references)
			//IL_0043: Unknown result type (might be due to invalid IL or missing references)
			//IL_004a: Unknown result type (might be due to invalid IL or missing references)
			//IL_0050: Unknown result type (might be due to invalid IL or missing references)
			//IL_0055: Unknown result type (might be due to invalid IL or missing references)
			//IL_005a: Unknown result type (might be due to invalid IL or missing references)
			ref KdNode reference = ref nodes.ElementAt(nodeIndex);
			ref KdNode reference2 = ref nodes.ElementAt(reference.PositiveChildIndex);
			ref KdNode reference3 = ref nodes.ElementAt(reference.NegativeChildIndex);
			reference.ColliderMin = math.min(reference3.ColliderMin, reference2.ColliderMin);
			reference.ColliderMax = math.max(reference3.ColliderMax, reference2.ColliderMax);
		}

		private double3 CalculateTravel(int index)
		{
			//IL_0007: Unknown result type (might be due to invalid IL or missing references)
			//IL_0018: Unknown result type (might be due to invalid IL or missing references)
			//IL_001d: Unknown result type (might be due to invalid IL or missing references)
			//IL_0032: Unknown result type (might be due to invalid IL or missing references)
			//IL_004a: Unknown result type (might be due to invalid IL or missing references)
			//IL_003d: Unknown result type (might be due to invalid IL or missing references)
			//IL_003e: Unknown result type (might be due to invalid IL or missing references)
			//IL_0044: Unknown result type (might be due to invalid IL or missing references)
			//IL_0049: Unknown result type (might be due to invalid IL or missing references)
			double3 val = velocities[index] * dt[0];
			double num = maxRelativeTravelDistance * radii[index];
			if (math.lengthsq(val) > num * num)
			{
				val = math.normalize(val) * num;
			}
			return val;
		}
	}

	[BurstCompile(/*Could not decode attribute arguments.*/)]
	private struct TreeBroadPhaseJob : IJobParallelForDefer
	{
		[ReadOnly]
		private NativeList<KdNode> nodes;

		[ReadOnly]
		private NativeList<int> leafIndices;

		[ReadOnly]
		private NativeList<int> permutation;

		[ReadOnly]
		private NativeArray<double3> position;

		[ReadOnly]
		private NativeArray<double3> velocity;

		[ReadOnly]
		private NativeArray<double> radius;

		[ReadOnly]
		private NativeArray<BodyFlags> flags;

		[ReadOnly]
		private NativeArray<double> dt;

		[WriteOnly]
		private ParallelWriter<BroadPhaseCollision> collisions;

		private double maxRelativeTravelDistance;

		public void Update(NativeList<KdNode> nodes, NativeList<int> leafIndices, NativeList<int> permutation, NativeArray<double3> position, NativeArray<double3> velocity, NativeArray<double> radius, NativeArray<BodyFlags> flags, NativeArray<double> dt, double maxRelativeTravelDistance, ParallelWriter<BroadPhaseCollision> collisions)
		{
			//IL_0001: Unknown result type (might be due to invalid IL or missing references)
			//IL_0002: Unknown result type (might be due to invalid IL or missing references)
			//IL_0008: Unknown result type (might be due to invalid IL or missing references)
			//IL_0009: Unknown result type (might be due to invalid IL or missing references)
			//IL_000f: Unknown result type (might be due to invalid IL or missing references)
			//IL_0010: Unknown result type (might be due to invalid IL or missing references)
			//IL_0016: Unknown result type (might be due to invalid IL or missing references)
			//IL_0018: Unknown result type (might be due to invalid IL or missing references)
			//IL_001e: Unknown result type (might be due to invalid IL or missing references)
			//IL_0020: Unknown result type (might be due to invalid IL or missing references)
			//IL_0026: Unknown result type (might be due to invalid IL or missing references)
			//IL_0028: Unknown result type (might be due to invalid IL or missing references)
			//IL_002e: Unknown result type (might be due to invalid IL or missing references)
			//IL_0030: Unknown result type (might be due to invalid IL or missing references)
			//IL_0036: Unknown result type (might be due to invalid IL or missing references)
			//IL_0038: Unknown result type (might be due to invalid IL or missing references)
			//IL_0046: Unknown result type (might be due to invalid IL or missing references)
			//IL_0048: Unknown result type (might be due to invalid IL or missing references)
			this.nodes = nodes;
			this.leafIndices = leafIndices;
			this.permutation = permutation;
			this.position = position;
			this.velocity = velocity;
			this.radius = radius;
			this.flags = flags;
			this.dt = dt;
			this.maxRelativeTravelDistance = maxRelativeTravelDistance;
			this.collisions = collisions;
		}

		public void Execute(int index)
		{
			//IL_001b: Unknown result type (might be due to invalid IL or missing references)
			//IL_0020: Unknown result type (might be due to invalid IL or missing references)
			//IL_0058: Unknown result type (might be due to invalid IL or missing references)
			//IL_005e: Unknown result type (might be due to invalid IL or missing references)
			//IL_007f: Unknown result type (might be due to invalid IL or missing references)
			//IL_0085: Unknown result type (might be due to invalid IL or missing references)
			//IL_00ea: Unknown result type (might be due to invalid IL or missing references)
			//IL_00f2: Unknown result type (might be due to invalid IL or missing references)
			//IL_00f7: Unknown result type (might be due to invalid IL or missing references)
			//IL_0101: Unknown result type (might be due to invalid IL or missing references)
			//IL_0106: Unknown result type (might be due to invalid IL or missing references)
			//IL_0108: Unknown result type (might be due to invalid IL or missing references)
			//IL_0115: Unknown result type (might be due to invalid IL or missing references)
			//IL_011a: Unknown result type (might be due to invalid IL or missing references)
			//IL_011c: Unknown result type (might be due to invalid IL or missing references)
			//IL_0121: Unknown result type (might be due to invalid IL or missing references)
			//IL_0123: Unknown result type (might be due to invalid IL or missing references)
			//IL_0128: Unknown result type (might be due to invalid IL or missing references)
			//IL_012d: Unknown result type (might be due to invalid IL or missing references)
			//IL_0137: Unknown result type (might be due to invalid IL or missing references)
			//IL_013c: Unknown result type (might be due to invalid IL or missing references)
			//IL_013e: Unknown result type (might be due to invalid IL or missing references)
			//IL_014b: Unknown result type (might be due to invalid IL or missing references)
			//IL_0150: Unknown result type (might be due to invalid IL or missing references)
			//IL_0152: Unknown result type (might be due to invalid IL or missing references)
			//IL_0157: Unknown result type (might be due to invalid IL or missing references)
			//IL_0159: Unknown result type (might be due to invalid IL or missing references)
			//IL_015e: Unknown result type (might be due to invalid IL or missing references)
			//IL_0163: Unknown result type (might be due to invalid IL or missing references)
			//IL_01b2: Unknown result type (might be due to invalid IL or missing references)
			//IL_01ba: Unknown result type (might be due to invalid IL or missing references)
			//IL_01bf: Unknown result type (might be due to invalid IL or missing references)
			//IL_01c9: Unknown result type (might be due to invalid IL or missing references)
			//IL_01ce: Unknown result type (might be due to invalid IL or missing references)
			//IL_01d0: Unknown result type (might be due to invalid IL or missing references)
			//IL_01dd: Unknown result type (might be due to invalid IL or missing references)
			//IL_01e2: Unknown result type (might be due to invalid IL or missing references)
			//IL_01e4: Unknown result type (might be due to invalid IL or missing references)
			//IL_01e9: Unknown result type (might be due to invalid IL or missing references)
			//IL_01eb: Unknown result type (might be due to invalid IL or missing references)
			//IL_01f0: Unknown result type (might be due to invalid IL or missing references)
			//IL_01f5: Unknown result type (might be due to invalid IL or missing references)
			//IL_01ff: Unknown result type (might be due to invalid IL or missing references)
			//IL_0204: Unknown result type (might be due to invalid IL or missing references)
			//IL_0206: Unknown result type (might be due to invalid IL or missing references)
			//IL_0213: Unknown result type (might be due to invalid IL or missing references)
			//IL_0218: Unknown result type (might be due to invalid IL or missing references)
			//IL_021a: Unknown result type (might be due to invalid IL or missing references)
			//IL_021f: Unknown result type (might be due to invalid IL or missing references)
			//IL_0221: Unknown result type (might be due to invalid IL or missing references)
			//IL_0226: Unknown result type (might be due to invalid IL or missing references)
			//IL_022b: Unknown result type (might be due to invalid IL or missing references)
			//IL_022d: Unknown result type (might be due to invalid IL or missing references)
			//IL_022f: Unknown result type (might be due to invalid IL or missing references)
			//IL_0234: Unknown result type (might be due to invalid IL or missing references)
			//IL_0236: Unknown result type (might be due to invalid IL or missing references)
			//IL_0238: Unknown result type (might be due to invalid IL or missing references)
			//IL_023d: Unknown result type (might be due to invalid IL or missing references)
			//IL_023f: Unknown result type (might be due to invalid IL or missing references)
			KdNode kdNode = nodes[leafIndices[index]];
			NativeQueue<int> val = new NativeQueue<int>(AllocatorHandle.op_Implicit((Allocator)2));
			val.Enqueue(0);
			while (!val.IsEmpty())
			{
				int num = val.Dequeue();
				KdNode kdNode2 = nodes[num];
				if (!kdNode2.Leaf)
				{
					if (ColliderOverlaps(kdNode2.PositiveChildIndex, kdNode.ColliderMin, kdNode.ColliderMax))
					{
						val.Enqueue(kdNode2.PositiveChildIndex);
					}
					if (ColliderOverlaps(kdNode2.NegativeChildIndex, kdNode.ColliderMin, kdNode.ColliderMax))
					{
						val.Enqueue(kdNode2.NegativeChildIndex);
					}
					continue;
				}
				for (int i = kdNode.Start; i < kdNode.End; i++)
				{
					int num2 = permutation[i];
					if (flags[num2].IsNotSet(BodyFlags.Collider))
					{
						continue;
					}
					double3 val2 = new double3(radius[num2]);
					double3 val3 = CalculateTravel(num2);
					double3 val4 = math.min(position[num2] - val2, position[num2] + val3 - val2);
					double3 val5 = math.max(position[num2] + val2, position[num2] + val3 + val2);
					for (int j = kdNode2.Start; j < kdNode2.End; j++)
					{
						int num3 = permutation[j];
						if (num3 < num2 && !flags[num3].IsNotSet(BodyFlags.Collider))
						{
							double3 val6 = new double3(radius[num3]);
							double3 val7 = CalculateTravel(num3);
							double3 val8 = math.min(position[num3] - val6, position[num3] + val7 - val6);
							double3 val9 = math.max(position[num3] + val6, position[num3] + val7 + val6);
							double3 val10 = math.max(val8, val4);
							if (math.all(math.min(val9, val5) > val10))
							{
								collisions.Enqueue(new BroadPhaseCollision
								{
									Body0 = num2,
									Body1 = num3,
									PrectionSteps = 1
								});
							}
						}
					}
				}
			}
			val.Dispose();
		}

		private bool ColliderOverlaps(int nodeIndex, double3 queryMin, double3 queryMax)
		{
			//IL_000d: Unknown result type (might be due to invalid IL or missing references)
			//IL_0012: Unknown result type (might be due to invalid IL or missing references)
			//IL_0013: Unknown result type (might be due to invalid IL or missing references)
			//IL_0018: Unknown result type (might be due to invalid IL or missing references)
			//IL_0019: Unknown result type (might be due to invalid IL or missing references)
			//IL_001e: Unknown result type (might be due to invalid IL or missing references)
			//IL_001f: Unknown result type (might be due to invalid IL or missing references)
			//IL_0024: Unknown result type (might be due to invalid IL or missing references)
			//IL_0025: Unknown result type (might be due to invalid IL or missing references)
			KdNode kdNode = nodes[nodeIndex];
			double3 val = math.max(kdNode.ColliderMin, queryMin);
			return math.all(math.min(kdNode.ColliderMax, queryMax) >= val);
		}

		private double3 CalculateTravel(int index)
		{
			//IL_0007: Unknown result type (might be due to invalid IL or missing references)
			//IL_0018: Unknown result type (might be due to invalid IL or missing references)
			//IL_001d: Unknown result type (might be due to invalid IL or missing references)
			//IL_0032: Unknown result type (might be due to invalid IL or missing references)
			//IL_004a: Unknown result type (might be due to invalid IL or missing references)
			//IL_003d: Unknown result type (might be due to invalid IL or missing references)
			//IL_003e: Unknown result type (might be due to invalid IL or missing references)
			//IL_0044: Unknown result type (might be due to invalid IL or missing references)
			//IL_0049: Unknown result type (might be due to invalid IL or missing references)
			double3 val = velocity[index] * dt[0];
			double num = maxRelativeTravelDistance * radius[index];
			if (math.lengthsq(val) > num * num)
			{
				val = math.normalize(val) * num;
			}
			return val;
		}
	}

	[BurstCompile(/*Could not decode attribute arguments.*/)]
	private struct NarrowPhaseJob : IJob
	{
		[ReadOnly]
		public NativeArray<double3> PositionNow;

		[ReadOnly]
		public NativeArray<double3> PositionBefore;

		[ReadOnly]
		public NativeArray<double> Radius;

		[ReadOnly]
		public NativeArray<int> Id;

		[ReadOnly]
		public int ProjectedStepCount;

		public NativeQueue<BroadPhaseCollision> BroadPhaseCollisions;

		public NativeQueue<CollisionDescriptorSimple> NarrowPhaseCollisions;

		[ReadOnly]
		public NativeArray<double> Dt;

		public void Execute()
		{
			int count = BroadPhaseCollisions.Count;
			if (count == 0)
			{
				return;
			}
			for (int i = 0; i < count; i++)
			{
				BroadPhaseCollision broadPhaseCollision = BroadPhaseCollisions.Dequeue();
				if (narrowPhase(broadPhaseCollision, Dt[0], out var collisionInfo))
				{
					NarrowPhaseCollisions.Enqueue(collisionInfo);
				}
			}
		}

		private bool narrowPhase(BroadPhaseCollision broadPhaseCollision, double dt, out CollisionDescriptorSimple collisionInfo)
		{
			//IL_000c: Unknown result type (might be due to invalid IL or missing references)
			//IL_0011: Unknown result type (might be due to invalid IL or missing references)
			//IL_001e: Unknown result type (might be due to invalid IL or missing references)
			//IL_0023: Unknown result type (might be due to invalid IL or missing references)
			//IL_0030: Unknown result type (might be due to invalid IL or missing references)
			//IL_0041: Unknown result type (might be due to invalid IL or missing references)
			//IL_0046: Unknown result type (might be due to invalid IL or missing references)
			//IL_004c: Unknown result type (might be due to invalid IL or missing references)
			//IL_0051: Unknown result type (might be due to invalid IL or missing references)
			//IL_005e: Unknown result type (might be due to invalid IL or missing references)
			//IL_006f: Unknown result type (might be due to invalid IL or missing references)
			//IL_0074: Unknown result type (might be due to invalid IL or missing references)
			//IL_007a: Unknown result type (might be due to invalid IL or missing references)
			//IL_007f: Unknown result type (might be due to invalid IL or missing references)
			//IL_00a4: Unknown result type (might be due to invalid IL or missing references)
			//IL_00a5: Unknown result type (might be due to invalid IL or missing references)
			//IL_00a6: Unknown result type (might be due to invalid IL or missing references)
			//IL_00ab: Unknown result type (might be due to invalid IL or missing references)
			//IL_00ad: Unknown result type (might be due to invalid IL or missing references)
			//IL_00ae: Unknown result type (might be due to invalid IL or missing references)
			//IL_00af: Unknown result type (might be due to invalid IL or missing references)
			//IL_00b4: Unknown result type (might be due to invalid IL or missing references)
			//IL_00bd: Unknown result type (might be due to invalid IL or missing references)
			//IL_0119: Unknown result type (might be due to invalid IL or missing references)
			//IL_011b: Unknown result type (might be due to invalid IL or missing references)
			//IL_012d: Unknown result type (might be due to invalid IL or missing references)
			//IL_012f: Unknown result type (might be due to invalid IL or missing references)
			//IL_0139: Unknown result type (might be due to invalid IL or missing references)
			//IL_013b: Unknown result type (might be due to invalid IL or missing references)
			double3 val = PositionNow[broadPhaseCollision.Body0];
			double3 val2 = PositionNow[broadPhaseCollision.Body1];
			double3 val3 = (PositionNow[broadPhaseCollision.Body0] - PositionBefore[broadPhaseCollision.Body0]) / dt;
			double3 val4 = (PositionNow[broadPhaseCollision.Body1] - PositionBefore[broadPhaseCollision.Body1]) / dt;
			double num = Radius[broadPhaseCollision.Body0];
			double num2 = Radius[broadPhaseCollision.Body1];
			double3 val5 = val - val2;
			double3 val6 = val3 - val4;
			double num3 = num2 + num;
			double num4 = num3 * num3;
			if (math.lengthsq(val5) == 0.0)
			{
				collisionInfo = new CollisionDescriptorSimple(Id[broadPhaseCollision.Body0], Id[broadPhaseCollision.Body1], 0.0, 0.0, 0.0);
				return true;
			}
			double num5 = math.dot(val6, val6);
			double num6 = 2.0 * math.dot(val6, val5);
			double num7 = math.dot(val5, val5) - num4;
			double num8 = num6 * num6 - 4.0 * num5 * num7;
			if (num8 < 0.0 || num5 == 0.0)
			{
				collisionInfo = default;
				return false;
			}
			double num9 = math.sqrt(num8);
			double num10 = (0.0 - num6 + num9) / (2.0 * num5);
			double num11 = (0.0 - num6 - num9) / (2.0 * num5);
			double num12 = math.min(num10, num11);
			double end = math.max(num10, num11);
			if (num12 > dt * (double)broadPhaseCollision.PrectionSteps)
			{
				collisionInfo = default;
				return false;
			}
			collisionInfo = new CollisionDescriptorSimple(Id[broadPhaseCollision.Body0], Id[broadPhaseCollision.Body1], num12, end, num12 / dt);
			return true;
		}
	}

	private NativeQueue<BroadPhaseCollision> broadPhaseCollisions = new NativeQueue<BroadPhaseCollision>(AllocatorHandle.op_Implicit((Allocator)4));

	private NativeQueue<CollisionDescriptorSimple> narrowPhaseCollisions = new NativeQueue<CollisionDescriptorSimple>(AllocatorHandle.op_Implicit((Allocator)4));

	private BruteForceBroadPhaseJob bruteForceBroadPhaseJob;

	private BuildBroadPhaseTreeJob buildTreeJob;

	private TreeBroadPhaseJob treeBroadPhaseJob;

	private NarrowPhaseJob narrowPhaseJob;

	private KnnRebuildJob rebuildJob;

	public CollisionDetector()
	{
		//IL_0002: Unknown result type (might be due to invalid IL or missing references)
		//IL_0007: Unknown result type (might be due to invalid IL or missing references)
		//IL_000c: Unknown result type (might be due to invalid IL or missing references)
		//IL_0013: Unknown result type (might be due to invalid IL or missing references)
		//IL_0018: Unknown result type (might be due to invalid IL or missing references)
		//IL_001d: Unknown result type (might be due to invalid IL or missing references)
	}

	public JobHandle DetectCollisions(NativeArray<double3> positionsFrom, BodyIterator now, KnnContainer tree, NativeArray<double> fullStepDt, int projectedStepCount = 1, JobHandle dependsOn = default(JobHandle))
	{
		//IL_014a: Unknown result type (might be due to invalid IL or missing references)
		//IL_014f: Unknown result type (might be due to invalid IL or missing references)
		//IL_016c: Unknown result type (might be due to invalid IL or missing references)
		//IL_016d: Unknown result type (might be due to invalid IL or missing references)
		//IL_017a: Unknown result type (might be due to invalid IL or missing references)
		//IL_017f: Unknown result type (might be due to invalid IL or missing references)
		//IL_0182: Unknown result type (might be due to invalid IL or missing references)
		//IL_0187: Unknown result type (might be due to invalid IL or missing references)
		//IL_0194: Unknown result type (might be due to invalid IL or missing references)
		//IL_0199: Unknown result type (might be due to invalid IL or missing references)
		//IL_019c: Unknown result type (might be due to invalid IL or missing references)
		//IL_01a1: Unknown result type (might be due to invalid IL or missing references)
		//IL_01ae: Unknown result type (might be due to invalid IL or missing references)
		//IL_01b3: Unknown result type (might be due to invalid IL or missing references)
		//IL_01b6: Unknown result type (might be due to invalid IL or missing references)
		//IL_01bb: Unknown result type (might be due to invalid IL or missing references)
		//IL_01db: Unknown result type (might be due to invalid IL or missing references)
		//IL_01dd: Unknown result type (might be due to invalid IL or missing references)
		//IL_01e2: Unknown result type (might be due to invalid IL or missing references)
		//IL_01eb: Unknown result type (might be due to invalid IL or missing references)
		//IL_01f0: Unknown result type (might be due to invalid IL or missing references)
		//IL_01fb: Unknown result type (might be due to invalid IL or missing references)
		//IL_01fd: Unknown result type (might be due to invalid IL or missing references)
		//IL_020a: Unknown result type (might be due to invalid IL or missing references)
		//IL_020f: Unknown result type (might be due to invalid IL or missing references)
		//IL_0213: Unknown result type (might be due to invalid IL or missing references)
		//IL_0218: Unknown result type (might be due to invalid IL or missing references)
		//IL_0225: Unknown result type (might be due to invalid IL or missing references)
		//IL_022a: Unknown result type (might be due to invalid IL or missing references)
		//IL_022d: Unknown result type (might be due to invalid IL or missing references)
		//IL_0232: Unknown result type (might be due to invalid IL or missing references)
		//IL_023e: Unknown result type (might be due to invalid IL or missing references)
		//IL_0243: Unknown result type (might be due to invalid IL or missing references)
		//IL_024e: Unknown result type (might be due to invalid IL or missing references)
		//IL_024f: Unknown result type (might be due to invalid IL or missing references)
		//IL_025c: Unknown result type (might be due to invalid IL or missing references)
		//IL_0261: Unknown result type (might be due to invalid IL or missing references)
		//IL_0264: Unknown result type (might be due to invalid IL or missing references)
		//IL_0269: Unknown result type (might be due to invalid IL or missing references)
		//IL_0281: Unknown result type (might be due to invalid IL or missing references)
		//IL_0283: Unknown result type (might be due to invalid IL or missing references)
		//IL_0038: Unknown result type (might be due to invalid IL or missing references)
		//IL_0048: Unknown result type (might be due to invalid IL or missing references)
		//IL_004a: Unknown result type (might be due to invalid IL or missing references)
		//IL_004f: Unknown result type (might be due to invalid IL or missing references)
		//IL_0059: Unknown result type (might be due to invalid IL or missing references)
		//IL_0060: Unknown result type (might be due to invalid IL or missing references)
		//IL_0067: Unknown result type (might be due to invalid IL or missing references)
		//IL_006c: Unknown result type (might be due to invalid IL or missing references)
		//IL_006f: Unknown result type (might be due to invalid IL or missing references)
		//IL_0076: Unknown result type (might be due to invalid IL or missing references)
		//IL_007b: Unknown result type (might be due to invalid IL or missing references)
		//IL_007e: Unknown result type (might be due to invalid IL or missing references)
		//IL_0085: Unknown result type (might be due to invalid IL or missing references)
		//IL_008a: Unknown result type (might be due to invalid IL or missing references)
		//IL_008d: Unknown result type (might be due to invalid IL or missing references)
		//IL_0094: Unknown result type (might be due to invalid IL or missing references)
		//IL_0099: Unknown result type (might be due to invalid IL or missing references)
		//IL_009c: Unknown result type (might be due to invalid IL or missing references)
		//IL_00a1: Unknown result type (might be due to invalid IL or missing references)
		//IL_00af: Unknown result type (might be due to invalid IL or missing references)
		//IL_00b1: Unknown result type (might be due to invalid IL or missing references)
		//IL_00b6: Unknown result type (might be due to invalid IL or missing references)
		//IL_00c0: Unknown result type (might be due to invalid IL or missing references)
		//IL_00c7: Unknown result type (might be due to invalid IL or missing references)
		//IL_00ce: Unknown result type (might be due to invalid IL or missing references)
		//IL_00d5: Unknown result type (might be due to invalid IL or missing references)
		//IL_00da: Unknown result type (might be due to invalid IL or missing references)
		//IL_00dd: Unknown result type (might be due to invalid IL or missing references)
		//IL_00e4: Unknown result type (might be due to invalid IL or missing references)
		//IL_00e9: Unknown result type (might be due to invalid IL or missing references)
		//IL_00ec: Unknown result type (might be due to invalid IL or missing references)
		//IL_00f3: Unknown result type (might be due to invalid IL or missing references)
		//IL_00f8: Unknown result type (might be due to invalid IL or missing references)
		//IL_00fb: Unknown result type (might be due to invalid IL or missing references)
		//IL_0102: Unknown result type (might be due to invalid IL or missing references)
		//IL_0107: Unknown result type (might be due to invalid IL or missing references)
		//IL_010a: Unknown result type (might be due to invalid IL or missing references)
		//IL_010f: Unknown result type (might be due to invalid IL or missing references)
		//IL_0118: Unknown result type (might be due to invalid IL or missing references)
		//IL_012a: Unknown result type (might be due to invalid IL or missing references)
		//IL_0130: Unknown result type (might be due to invalid IL or missing references)
		//IL_0132: Unknown result type (might be due to invalid IL or missing references)
		//IL_0137: Unknown result type (might be due to invalid IL or missing references)
		if (Sandbox.ActiveSimulation.UseCollisionTree)
		{
			double maxRelativeTravelDistance = ((now.NumBodies < 100) ? 1E+20 : 10.0);
			rebuildJob.Update(tree, now.Position);
			dependsOn = IJobExtensions.Schedule<KnnRebuildJob>(rebuildJob, dependsOn);
			buildTreeJob.Update(tree.Nodes, tree.Permutation, now.Radius.AsArray(), now.ObjectBodyFlags.AsArray(), now.Position.AsArray(), now.Velocity.AsArray(), fullStepDt, maxRelativeTravelDistance);
			dependsOn = IJobExtensions.Schedule<BuildBroadPhaseTreeJob>(buildTreeJob, dependsOn);
			treeBroadPhaseJob.Update(tree.Nodes, tree.Leafs, tree.Permutation, now.Position.AsArray(), now.Velocity.AsArray(), now.Radius.AsArray(), now.ObjectBodyFlags.AsArray(), fullStepDt, maxRelativeTravelDistance, broadPhaseCollisions.AsParallelWriter());
			dependsOn = IJobParallelForDeferExtensions.Schedule<TreeBroadPhaseJob, int>(treeBroadPhaseJob, tree.Leafs, 1, dependsOn);
		}
		else
		{
			bruteForceBroadPhaseJob.BroadPhaseCollisions = broadPhaseCollisions.AsParallelWriter();
			bruteForceBroadPhaseJob.NumMajors = now.NumMajors;
			bruteForceBroadPhaseJob.PositionBefore = positionsFrom;
			bruteForceBroadPhaseJob.PositionNow = now.Position.AsArray();
			bruteForceBroadPhaseJob.Radius = now.Radius.AsArray();
			bruteForceBroadPhaseJob.Flags = now.ObjectBodyFlags.AsArray();
			bruteForceBroadPhaseJob.ProjectedStepCount = projectedStepCount;
			dependsOn = IJobParallelForExtensions.Schedule<BruteForceBroadPhaseJob>(bruteForceBroadPhaseJob, now.NumBodies, 1, dependsOn);
		}
		narrowPhaseJob.BroadPhaseCollisions = broadPhaseCollisions;
		narrowPhaseJob.Dt = fullStepDt;
		narrowPhaseJob.Id = now.Id.AsArray();
		narrowPhaseJob.Radius = now.Radius.AsArray();
		narrowPhaseJob.NarrowPhaseCollisions = narrowPhaseCollisions;
		narrowPhaseJob.PositionBefore = positionsFrom;
		narrowPhaseJob.PositionNow = now.Position.AsArray();
		narrowPhaseJob.ProjectedStepCount = projectedStepCount;
		return IJobExtensions.Schedule<NarrowPhaseJob>(narrowPhaseJob, dependsOn);
	}

	public int GetDetectedCollisions(ref CollisionDescriptorSimple[] collisionArraySimple)
	{
		int count = narrowPhaseCollisions.Count;
		if (count > collisionArraySimple.Length)
		{
			Array.Resize(ref collisionArraySimple, count);
		}
		for (int i = 0; i < count; i++)
		{
			CollisionDescriptorSimple collisionDescriptorSimple = narrowPhaseCollisions.Dequeue();
			collisionArraySimple[i] = collisionDescriptorSimple;
		}
		return count;
	}

	public void Dispose()
	{
		broadPhaseCollisions.Dispose();
		narrowPhaseCollisions.Dispose();
	}
}
