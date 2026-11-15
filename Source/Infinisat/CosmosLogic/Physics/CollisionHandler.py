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

	[BurstCompile()]
	private struct BruteForceBroadPhaseJob : IJobParallelFor
	{
		[ReadOnly]
		public NativeArray<double3> PositionBefore;

		[ReadOnly]
		public NativeArray<double3> PositionNow;

		[ReadOnly]
		public NativeArray<double> Radius;

		[ReadOnly]
		public NativeArray<EntityFlags> Flags;

		[ReadOnly]
		public int ProjectedStepCount;

		[WriteOnly]
		public ParallelWriter<BroadPhaseCollision> BroadPhaseCollisions;

		[ReadOnly]
		public int NumMajors;

		public void Execute(int bodyIndex)
		{
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			if (Flags[bodyIndex].IsNotSet(EntityFlags.Collider))
			{
				return;
			}
			double num = math.length(PositionNow[bodyIndex] - PositionBefore[bodyIndex]);
			double3 val = 0.5 * (PositionNow[bodyIndex] + PositionBefore[bodyIndex]);
			int num2 = ((bodyIndex >= NumMajors) ? 1 : ProjectedStepCount);
			double num3 = (double)num2 * num + Radius[bodyIndex];
			for (int i = 0; i < NumMajors; i++)
			{
				bool2 val2 = new bool2(Flags[i].IsNotSet(EntityFlags.ColliderGroupMajor), Flags[i].IsNotSet(EntityFlags.Collider));
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

	[BurstCompile()]
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
		private NativeArray<EntityFlags> flags;

		[ReadOnly]
		private NativeArray<double> dt;

		private double maxRelativeTravelDistance;

		public void Update(NativeList<KdNode> nodes, NativeList<int> permutation, NativeArray<double> radii, NativeArray<EntityFlags> flags, NativeArray<double3> positions, NativeArray<double3> velocities, NativeArray<double> dt, double maxRelativeTravelDistance)
		{
			
			
			
			
			
			
			
			
			
			
			
			
			
			
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
				if (!flags[num].IsNotSet(EntityFlags.Collider) && !flags[num].IsSet(EntityFlags.ColliderGas))
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
			
			
			
			
			
			
			
			
			ref KdNode reference = ref nodes.ElementAt(nodeIndex);
			ref KdNode reference2 = ref nodes.ElementAt(reference.PositiveChildIndex);
			ref KdNode reference3 = ref nodes.ElementAt(reference.NegativeChildIndex);
			reference.ColliderMin = math.min(reference3.ColliderMin, reference2.ColliderMin);
			reference.ColliderMax = math.max(reference3.ColliderMax, reference2.ColliderMax);
		}

		private double3 CalculateTravel(int index)
		{
			
			
			
			
			
			
			
			
			
			double3 val = velocities[index] * dt[0];
			double num = maxRelativeTravelDistance * radii[index];
			if (math.lengthsq(val) > num * num)
			{
				val = math.normalize(val) * num;
			}
			return val;
		}
	}

	[BurstCompile()]
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
		private NativeArray<EntityFlags> flags;

		[ReadOnly]
		private NativeArray<double> dt;

		[WriteOnly]
		private ParallelWriter<BroadPhaseCollision> collisions;

		private double maxRelativeTravelDistance;

		public void Update(NativeList<KdNode> nodes, NativeList<int> leafIndices, NativeList<int> permutation, NativeArray<double3> position, NativeArray<double3> velocity, NativeArray<double> radius, NativeArray<EntityFlags> flags, NativeArray<double> dt, double maxRelativeTravelDistance, ParallelWriter<BroadPhaseCollision> collisions)
		{
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
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
					if (flags[num2].IsNotSet(EntityFlags.Collider))
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
						if (num3 < num2 && !flags[num3].IsNotSet(EntityFlags.Collider))
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
			
			
			
			
			
			
			
			
			
			KdNode kdNode = nodes[nodeIndex];
			double3 val = math.max(kdNode.ColliderMin, queryMin);
			return math.all(math.min(kdNode.ColliderMax, queryMax) >= val);
		}

		private double3 CalculateTravel(int index)
		{
			
			
			
			
			
			
			
			
			
			double3 val = velocity[index] * dt[0];
			double num = maxRelativeTravelDistance * radius[index];
			if (math.lengthsq(val) > num * num)
			{
				val = math.normalize(val) * num;
			}
			return val;
		}
	}

	[BurstCompile()]
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
		
		
		
		
		
		
	}

	public JobHandle DetectCollisions(NativeArray<double3> positionsFrom, BodyIterator now, KnnContainer tree, NativeArray<double> fullStepDt, int projectedStepCount = 1, JobHandle dependsOn = default(JobHandle))
	{
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		if (WorldMgr.ActiveSimulation.UseCollisionTree)
		{
			double maxRelativeTravelDistance = ((now.NumBodies < 100) ? 1E+20 : 10.0);
			rebuildJob.Update(tree, now.Position);
			dependsOn = IJobExtensions.Schedule<KnnRebuildJob>(rebuildJob, dependsOn);
			buildTreeJob.Update(tree.Nodes, tree.Permutation, now.Radius.AsArray(), now.ObjectEntityFlags.AsArray(), now.Position.AsArray(), now.Velocity.AsArray(), fullStepDt, maxRelativeTravelDistance);
			dependsOn = IJobExtensions.Schedule<BuildBroadPhaseTreeJob>(buildTreeJob, dependsOn);
			treeBroadPhaseJob.Update(tree.Nodes, tree.Leafs, tree.Permutation, now.Position.AsArray(), now.Velocity.AsArray(), now.Radius.AsArray(), now.ObjectEntityFlags.AsArray(), fullStepDt, maxRelativeTravelDistance, broadPhaseCollisions.AsParallelWriter());
			dependsOn = IJobParallelForDeferExtensions.Schedule<TreeBroadPhaseJob, int>(treeBroadPhaseJob, tree.Leafs, 1, dependsOn);
		}
		else
		{
			bruteForceBroadPhaseJob.BroadPhaseCollisions = broadPhaseCollisions.AsParallelWriter();
			bruteForceBroadPhaseJob.NumMajors = now.NumMajors;
			bruteForceBroadPhaseJob.PositionBefore = positionsFrom;
			bruteForceBroadPhaseJob.PositionNow = now.Position.AsArray();
			bruteForceBroadPhaseJob.Radius = now.Radius.AsArray();
			bruteForceBroadPhaseJob.Flags = now.ObjectEntityFlags.AsArray();
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
