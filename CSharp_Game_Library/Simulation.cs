using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Diagnostics;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Threading;
using Cysharp.Threading.Tasks;
using Cysharp.Threading.Tasks.CompilerServices;
using DOTS;
using GiantArmy.Analytics;
using GiantArmy.ECS;
using GiantArmy.Localization;
using Mathematics;
using NBody;
using NBody.Tethers;
using CosmosLogic.Engine;
using CosmosLogic.Threading;
using CosmosLogic.Math;
using CosmosLogic.Core;

public class Simulation : ISerialize, IDisposable
{
	public static class Strings
	{
		[UIStringInfo("ffbb71515624497fb17d1cdbb57f95e5", "")]
		public static UIString simSavedMsg;

		[UIStringInfo("c113a846a21b4a23acdf70cf94c77ac9", "")]
		public static UIString reopenMsg;
	}

	public struct MajorCollision
	{
		public double EstimatedTime;

		public Vector3D Position1;

		public Vector3D Position2;

		public Body Primary;

		public Body Secondary;
	}

	private class DateTarget
	{
		private const double ACCEL_DECEL_FACTOR = 10.0;

		private const double STOP_TICKS_BEFORE_TARGET = 1.0;

		private const double MIN_STEP_SEC = 120.0;

		private readonly Simulation sim;

		private DateTime targetDateTime;

		private double targetTimestep;

		private double restoreTimestep;

		private Action targetReachedCallback;

		private bool wasMaxSpeed;

		private bool changeTimeDirectionOnTargetReached;

		public bool isAlive { get; private set; }

		private double secondsUntilTarget
		{
			get
			{
				if (!isAlive)
				{
					return 0.0;
				}
				return sim.Date.SecondsUntil(targetDateTime);
			}
		}

		public DateTarget(Simulation sim)
		{
			this.sim = sim;
			MessageDispatcher.AddListener("UserModifiedTimestep", (Action<Message>)OnUserModifiedTimestep);
		}

		private void OnUserModifiedTimestep(Message m)
		{
			OnUserChangedTimestep();
		}

		public void Dispose()
		{
			MessageDispatcher.RemoveListener("UserModifiedTimestep", (Action<Message>)OnUserModifiedTimestep);
		}

		public bool Set(Action targetReachedCallback)
		{
			isAlive = timeControlBehavior.isValid;
			if (sim.Date.isTimeReversed)
			{
				sim.Date.isTimeReversed = false;
				changeTimeDirectionOnTargetReached = true;
			}
			else
			{
				changeTimeDirectionOnTargetReached = false;
			}
			if (isAlive)
			{
				targetDateTime = timeControlBehavior.validatedTargetDateTime.dateTime;
				this.targetReachedCallback = targetReachedCallback;
				restoreTimestep = sim.TargetTimeStepPerRealSec;
				if (timeControlBehavior.maxSpeed)
				{
					SetTargetTimestep(Math.Max(120.0, sim.TargetTimeStepPerRealSec));
				}
				Sandbox.Paused = false;
			}
			return isAlive;
		}

		public void Update()
		{
			if (!sim.dateTargetActivated)
			{
				isAlive = false;
			}
			if (isAlive)
			{
				SetTarget();
				FindTarget();
			}
		}

		public void Cancel()
		{
			if (isAlive)
			{
				SetTargetTimestep(restoreTimestep);
				isAlive = false;
				targetReachedCallback = null;
			}
		}

		private void OnUserChangedTimestep()
		{
			restoreTimestep = sim.TargetTimeStepPerRealSec;
		}

		private void SetTarget()
		{
			if (timeControlBehavior.onArrival == TimeControlBehavior.ArrivalBehavior.Pause)
			{
				targetTimestep = 120.0;
			}
			else if (timeControlBehavior.onArrival == TimeControlBehavior.ArrivalBehavior.ChangeTimeStep)
			{
				targetTimestep = timeControlBehavior.targetTimestep;
			}
			else
			{
				targetTimestep = restoreTimestep;
			}
		}

		private void FindTarget()
		{
			if (sim.TimeDelta >= Math.Abs(secondsUntilTarget) / 10.0)
			{
				SetTargetTimestep(Math.Max(sim.TargetTimeStepPerRealSec / 2.0, targetTimestep));
			}
			else if (!timeControlBehavior.maxSpeed && wasMaxSpeed)
			{
				SetTargetTimestep(restoreTimestep);
			}
			else if (timeControlBehavior.maxSpeed)
			{
				double val = Math.Max(sim.TargetTimeStepPerRealSec, Math.Min(sim.ActualStepPerSecond * 2.0, double.MaxValue));
				SetTargetTimestep(Math.Min(val, secondsUntilTarget * 10.0));
			}
			if (IsAtOrPastTarget(targetDateTime))
			{
				OnTargetReached();
			}
			wasMaxSpeed = timeControlBehavior.maxSpeed;
		}

		private bool IsAtOrPastTarget(DateTime target)
		{
			if (0 == 0)
			{
				return (double)(target.Ticks - sim.Date.LimitedRangeDate.Ticks) < 1.0;
			}
			return true;
		}

		private void SetTargetTimestep(double newTarget)
		{
			sim.TargetTimeStepPerRealSec = Math.Max(Math.Abs(newTarget), 1E-05);
		}

		private void OnTargetReached()
		{
			isAlive = false;
			sim.Date.isTimeReversed ^= changeTimeDirectionOnTargetReached;
			if (timeControlBehavior.onArrival == TimeControlBehavior.ArrivalBehavior.Pause)
			{
				SetTargetTimestep(restoreTimestep);
				Sandbox.Paused = true;
			}
			if (timeControlBehavior.onArrival == TimeControlBehavior.ArrivalBehavior.ChangeTimeStep)
			{
				SetTargetTimestep(timeControlBehavior.targetTimestep);
			}
			if (targetReachedCallback != null)
			{
				targetReachedCallback();
			}
			targetReachedCallback = null;
		}
	}

	[CompilerGenerated]
	private sealed class <>c__DisplayClass343_0
	{
		public bool wasSaving;

		internal async void <ChangeTimeAndExplode>b__1()
		{
			if (!wasSaving || string.IsNullOrEmpty(Preferences.Instance.LastBackupSave))
			{
				await Sandbox.ReloadSimulationAsync();
			}
			else
			{
				await Sandbox.LoadSimAsync(Preferences.Instance.LastBackupSave, wasReload: true);
			}
		}
	}

	[StructLayout(LayoutKind.Auto)]
	[CompilerGenerated]
	private struct <ChangeTimeAndExplode>d__343 : IAsyncStateMachine
	{
		public int <>1__state;

		public AsyncUniTaskMethodBuilder <>t__builder;

		public Simulation <>4__this;

		private <>c__DisplayClass343_0 <>8__1;

		private double <maxSpeed>5__2;

		private double <minSpeed>5__3;

		private Awaiter <>u__1;

		private Awaiter <>u__2;

		private void MoveNext()
		{
			//IL_0078: Unknown result type (might be due to invalid IL or missing references)
			//IL_007d: Unknown result type (might be due to invalid IL or missing references)
			//IL_0084: Unknown result type (might be due to invalid IL or missing references)
			//IL_0145: Unknown result type (might be due to invalid IL or missing references)
			//IL_014a: Unknown result type (might be due to invalid IL or missing references)
			//IL_0152: Unknown result type (might be due to invalid IL or missing references)
			//IL_0284: Unknown result type (might be due to invalid IL or missing references)
			//IL_0289: Unknown result type (might be due to invalid IL or missing references)
			//IL_0290: Unknown result type (might be due to invalid IL or missing references)
			//IL_003c: Unknown result type (might be due to invalid IL or missing references)
			//IL_0041: Unknown result type (might be due to invalid IL or missing references)
			//IL_0045: Unknown result type (might be due to invalid IL or missing references)
			//IL_004a: Unknown result type (might be due to invalid IL or missing references)
			//IL_005e: Unknown result type (might be due to invalid IL or missing references)
			//IL_005f: Unknown result type (might be due to invalid IL or missing references)
			//IL_02cb: Unknown result type (might be due to invalid IL or missing references)
			//IL_02d7: Expected Obj, but got Unknown
			//IL_0107: Unknown result type (might be due to invalid IL or missing references)
			//IL_010c: Unknown result type (might be due to invalid IL or missing references)
			//IL_0110: Unknown result type (might be due to invalid IL or missing references)
			//IL_0115: Unknown result type (might be due to invalid IL or missing references)
			//IL_012a: Unknown result type (might be due to invalid IL or missing references)
			//IL_012c: Unknown result type (might be due to invalid IL or missing references)
			//IL_0248: Unknown result type (might be due to invalid IL or missing references)
			//IL_024d: Unknown result type (might be due to invalid IL or missing references)
			//IL_0251: Unknown result type (might be due to invalid IL or missing references)
			//IL_0256: Unknown result type (might be due to invalid IL or missing references)
			//IL_026a: Unknown result type (might be due to invalid IL or missing references)
			//IL_026b: Unknown result type (might be due to invalid IL or missing references)
			int num = <>1__state;
			Simulation simulation = <>4__this;
			try
			{
				UniTask val3;
				Awaiter val;
				Awaiter val2;
				switch (num)
				{
				default:
					<>8__1 = new <>c__DisplayClass343_0();
					val3 = UniTask.Delay(500, false, (PlayerLoopTiming)8, default(CancellationToken), false);
					val = val3.GetAwaiter();
					if (!val.IsCompleted)
					{
						num = (<>1__state = 0);
						<>u__1 = val;
						<>t__builder.AwaitUnsafeOnCompleted<Awaiter, <ChangeTimeAndExplode>d__343>(ref val, ref this);
						return;
					}
					goto IL_0093;
				case 0:
					val = <>u__1;
					<>u__1 = default;
					num = (<>1__state = -1);
					goto IL_0093;
				case 1:
					val2 = <>u__2;
					<>u__2 = default;
					num = (<>1__state = -1);
					goto IL_0161;
				case 2:
					{
						val = <>u__1;
						<>u__1 = default;
						num = (<>1__state = -1);
						goto IL_029f;
					}
					IL_029f:
					val.GetResult();
					break;
					IL_0093:
					val.GetResult();
					simulation.AutoSpeedSettings.Enabled = false;
					<maxSpeed>5__2 = 7200.0;
					<minSpeed>5__3 = 120.0;
					if (simulation.TargetTimeStepPerRealSec > <maxSpeed>5__2)
					{
						simulation.TargetTimeStepPerRealSec = <maxSpeed>5__2;
					}
					if (simulation.TargetTimeStepPerRealSec < <minSpeed>5__3)
					{
						simulation.TargetTimeStepPerRealSec = <minSpeed>5__3;
					}
					if (Sandbox.Paused)
					{
						Sandbox.Paused = false;
					}
					goto IL_0168;
					IL_0161:
					val2.GetResult();
					goto IL_0168;
					IL_0168:
					if (simulation.AverageStepPerSecond > 1.1 * <maxSpeed>5__2 || simulation.AverageStepPerSecond < 0.9 * <minSpeed>5__3)
					{
						YieldAwaitable val4 = UniTask.Yield();
						val2 = val4.GetAwaiter();
						if (!val2.IsCompleted)
						{
							num = (<>1__state = 1);
							<>u__2 = val2;
							<>t__builder.AwaitUnsafeOnCompleted<Awaiter, <ChangeTimeAndExplode>d__343>(ref val2, ref this);
							return;
						}
						goto IL_0161;
					}
					for (int i = 0; i < simulation.Bodies.Count; i++)
					{
						double num2 = simulation.TargetTimeStepPerRealSec * (double)i * 0.5;
						Body body = simulation.Bodies[i];
						if (body.Particle != null)
						{
							num2 *= 0.01;
						}
						body.SetBodyToExplode(num2);
					}
					<>8__1.wasSaving = FileTools.IsSavingAsync;
					if (!<>8__1.wasSaving)
					{
						break;
					}
					val3 = UniTask.WaitUntil((Func<bool>)(() => !FileTools.IsSavingAsync), (PlayerLoopTiming)8, default(CancellationToken), false);
					val = val3.GetAwaiter();
					if (!val.IsCompleted)
					{
						num = (<>1__state = 2);
						<>u__1 = val;
						<>t__builder.AwaitUnsafeOnCompleted<Awaiter, <ChangeTimeAndExplode>d__343>(ref val, ref this);
						return;
					}
					goto IL_029f;
				}
				Snackbar.ShowPrompt("", Strings.simSavedMsg, Strings.reopenMsg, async () =>
				{
					if (!<>8__1.wasSaving || string.IsNullOrEmpty(Preferences.Instance.LastBackupSave))
					{
						await Sandbox.ReloadSimulationAsync();
					}
					else
					{
						await Sandbox.LoadSimAsync(Preferences.Instance.LastBackupSave, wasReload: true);
					}
				});
			}
			catch (Exception exception)
			{
				<>1__state = -2;
				<>8__1 = null;
				<>t__builder.SetException(exception);
				return;
			}
			<>1__state = -2;
			<>8__1 = null;
			<>t__builder.SetResult();
		}

		void IAsyncStateMachine.MoveNext()
		{
			//ILSpy generated this explicit interface implementation from .override directive in MoveNext
			this.MoveNext();
		}

		[DebuggerHidden]
		private void SetStateMachine(IAsyncStateMachine stateMachine)
		{
			<>t__builder.SetStateMachine(stateMachine);
		}

		void IAsyncStateMachine.SetStateMachine(IAsyncStateMachine stateMachine)
		{
			//ILSpy generated this explicit interface implementation from .override directive in SetStateMachine
			this.SetStateMachine(stateMachine);
		}
	}

	private List<Body> _stars = new List<Body>();

	private ConcurrentBag<Body> StarBag = new ConcurrentBag<Body>();

	private int _rockParticleCount;

	private int _dustCloudCount;

	private bool wasDeserialized;

	public bool UserCreated;

	private bool _changed;

	private double AdvanceTimeStepPerSecond;

	private bool averageTimestepPerSecValuesAreTimesEachFrames;

	private float averageTimestepPerSecTimeSpan = 1f;

	private int averageTimestepPerSecMaxFrameCount = 120;

	private int averageTimestepPerSecMinFrameCount = 20;

	private AverageOverTime _averageStepPerSecond = new AverageOverTime(valuesAreTimesEachFrame: false);

	private double simTimeDelta;

	private float lastRealTime;

	public float realTimeDelta;

	private double physTimeDelta;

	public double activeTimeElapsed;

	[PropertyInfo("26e065b07032416f9cdce3725ad24aba", "")]
	public bool AutomaticTolerance = true;

	[PropertyInfo("18db8564fce14a2d9645ce4ca7c5e90f", "")]
	[Unit(UnitType.Multiplier)]
	public double Tolerance = 1.0;

	[PropertyInfo("df40447821a840d99610f37709c3d36e", "")]
	[Unit(UnitType.Length)]
	public double EstimatedBaseTolerance = 10000000.0;

	[PropertyInfo("a8eda5ea9b1a4b7cb2e718c7b98f12ff", "")]
	public double AccuracySensitivity = 1000.0;

	[PropertyInfo("ce794b976323461d97e50882d8d56cbf", "")]
	public bool ParticleStream;

	[PropertyInfo("c79f697fa7994641862e0cd29b1f5676", "")]
	public Integrator.ID IntegratorId = Integrator.ID.Hermite;

	public bool UserChangedAdaptiveIntegration;

	private double savedGravity = 6.6740831E-11;

	[PropertyInfo("ea48678ca653434ebcec37fe915c1e37", "")]
	[Icon("icon_label")]
	public string Name;

	public string BuildVersion;

	public int BuildCommit;

	public string NameToken;

	[PropertyInfo("e90153a95b5a4fbda3c238a1c7754f1c", "")]
	public string Description;

	public Guide DescriptionGuide;

	public bool IsActive;

	[NonSerialized]
	public bool SuspendTimestepAveraging;

	private DateTarget dateTarget;

	private bool _dateTargetActivated;

	public DateUniversal Date;

	public double TimePassed;

	private bool needsFixedTimeStep;

	private bool isFirstFrame = true;

	public AutoSpeedSettings AutoSpeedSettings = new AutoSpeedSettings();

	public AutoCameraSettings AutoCameraSettings = new AutoCameraSettings();

	public double targetTimeStepPerRealSec;

	public double MaximalTimeStepPerRealSec = double.MaxValue;

	public double AutomaticTimeStepChangeIndicator;

	[Unit(UnitType.TimeStep)]
	public double AdjustedTargetTimeStepPerSecond;

	[Unit(UnitType.Time)]
	public double TargetSimTimeElapsed;

	[Unit(UnitType.Time)]
	public double ActualSimTimeElapsed;

	private bool realtimeSyncronization;

	public List<MajorCollision> MajorCollisions = new List<MajorCollision>();

	public bool EnableBudgetReduction = true;

	private bool isDisposed;

	private List<Group> _groups = new List<Group>();

	private BodySet _bodies = new BodySet();

	private EntitySet _entities = new EntitySet();

	private EntitySet _entitiesToInitialize = new EntitySet();

	public SimulationSettings Settings = new SimulationSettings();

	public AmbienceSettings Ambience = new AmbienceSettings();

	public ViewSettings View = ViewSettings.Instance;

	public Guide OpenGuideOnLoad;

	[PropertyInfo("c26aa5920b2548b8978dd04e52304f2c", "")]
	[ReadOnly(true)]
	[Chartable]
	public int ParticlesRemoved;

	private bool useMaxErrorPerTime;

	private double tolerancePerSec;

	private static EntityQuery allMembers;

	private static EntityQuery attractors;

	private static EntityQuery nonAttractors;

	private static EntityQuery nonMembers;

	public static EntityQuery stateQuery;

	public static EntityQuery simPropertiesQuery;

	private PropertyFlags relevantProperties = PropertyFlags.Position | PropertyFlags.Velocity | PropertyFlags.Acceleration | PropertyFlags.Orientation | PropertyFlags.AngularVelocity | PropertyFlags.StrongestAttractor | PropertyFlags.OrbitalParent | PropertyFlags.RocheLobeRadius;

	private int stepCounter;

	private int lastStepCount;

	public double SmallestRadius;

	public double LargestRadius;

	public int NumberOfNewEntities;

	public int NumberOfLastEntities;

	private AverageOverTime averageEntitySpawnRate = new AverageOverTime(valuesAreTimesEachFrame: false, 2f);

	public bool WasDeserialized => wasDeserialized;

	public bool Changed
	{
		get
		{
			return _changed;
		}
		set
		{
			if (!value || (!GuideManager.IsGuideActive && !global::Settings.Global.BenchmarkMode))
			{
				_changed = value;
			}
		}
	}

	public DateTime startTime { get; private set; }

	public bool AverageTimestepPerSecValuesAreTimesEachFrames
	{
		get
		{
			return averageTimestepPerSecValuesAreTimesEachFrames;
		}
		set
		{
			if (averageTimestepPerSecValuesAreTimesEachFrames != value)
			{
				_averageStepPerSecond = new AverageOverTime(value, averageTimestepPerSecTimeSpan, averageTimestepPerSecMaxFrameCount, averageTimestepPerSecMinFrameCount);
			}
			averageTimestepPerSecValuesAreTimesEachFrames = value;
		}
	}

	public float AverageTimestepPerSecTimeSpan
	{
		get
		{
			return averageTimestepPerSecTimeSpan;
		}
		set
		{
			if (averageTimestepPerSecTimeSpan != value)
			{
				_averageStepPerSecond = new AverageOverTime(averageTimestepPerSecValuesAreTimesEachFrames, value, averageTimestepPerSecMaxFrameCount, averageTimestepPerSecMinFrameCount);
			}
			averageTimestepPerSecTimeSpan = value;
		}
	}

	public int AverageTimestepPerSecMaxFrameCount
	{
		get
		{
			return averageTimestepPerSecMaxFrameCount;
		}
		set
		{
			if (averageTimestepPerSecMaxFrameCount != value)
			{
				_averageStepPerSecond = new AverageOverTime(averageTimestepPerSecValuesAreTimesEachFrames, averageTimestepPerSecTimeSpan, value, averageTimestepPerSecMinFrameCount);
			}
			averageTimestepPerSecMaxFrameCount = value;
		}
	}

	public int AverageTimestepPerSecMinFrameCount
	{
		get
		{
			return averageTimestepPerSecMinFrameCount;
		}
		set
		{
			if (averageTimestepPerSecMinFrameCount != value)
			{
				_averageStepPerSecond = new AverageOverTime(averageTimestepPerSecValuesAreTimesEachFrames, averageTimestepPerSecTimeSpan, averageTimestepPerSecMaxFrameCount, value);
			}
			averageTimestepPerSecMinFrameCount = value;
		}
	}

	[PropertyInfo("16367937272f4b98a5ffe018d6c9495a", "")]
	[ReadOnly(true)]
	public Body SlowestBody { get; private set; }

	public Body LargestBody { get; set; }

	[PropertyInfo("dd7e1af773a144959add526d84699500", "")]
	[ValueRange(-1.0, 1.0)]
	public double Accuracy
	{
		get
		{
			return (0.0 - Math.Log10(Tolerance / EstimatedBaseTolerance)) / Math.Log10(AccuracySensitivity);
		}
		set
		{
			UpdateTolerance(value);
		}
	}

	[PropertyInfo("b1dbd50ed4604d378fbc5ac5f8a497d6", "")]
	public bool AdaptiveIntegration { get; set; }

	public double Gravity
	{
		get
		{
			if (Settings.EnableGravity && Settings.EnableSimGravity)
			{
				if (!Settings.EnableNBodyGravity || this != Sandbox.ActiveSimulation)
				{
					return savedGravity;
				}
				return NBodyGlobalOptions.Instance.GravitationalConstant;
			}
			return 0.0;
		}
		set
		{
			savedGravity = value;
			if (this == Sandbox.ActiveSimulation)
			{
				NBodyGlobalOptions.Instance.GravitationalConstant = value;
			}
		}
	}

	[PropertyInfo("1078aa20bc5d449ca91e85a3936697e6", "")]
	public string GravityModeDesc
	{
		get
		{
			if (!UseTree)
			{
				return GravityMode.BruteForce.ToString();
			}
			return GravityMode.Tree.ToString();
		}
	}

	public bool UseTree
	{
		get
		{
			if (Settings.GravityMode != GravityMode.Tree)
			{
				if (Settings.GravityMode == GravityMode.Auto)
				{
					return Sandbox.ActiveSimulation.Attracting >= 100;
				}
				return false;
			}
			return true;
		}
	}

	public bool UseGravityTree
	{
		get
		{
			if (UseTree)
			{
				return Settings.EnableGravityTree;
			}
			return false;
		}
	}

	public bool UseCollisionTree
	{
		get
		{
			if (UseTree)
			{
				return Settings.EnableCollisionTree;
			}
			return false;
		}
	}

	[PropertyInfo("16367937272f4b98a5ffe018d6c9495a", "")]
	public Body TimestepLimitingObject
	{
		get
		{
			//IL_0027: Unknown result type (might be due to invalid IL or missing references)
			//IL_002c: Unknown result type (might be due to invalid IL or missing references)
			//IL_003d: Unknown result type (might be due to invalid IL or missing references)
			//IL_0042: Unknown result type (might be due to invalid IL or missing references)
			//IL_0046: Unknown result type (might be due to invalid IL or missing references)
			//IL_0055: Unknown result type (might be due to invalid IL or missing references)
			SimulationState simulationState = default;
			if (!stateQuery.IsEmpty && World.DefaultGameObjectInjectionWorld.IsCreated && stateQuery.TryGetSingleton<SimulationState>(ref simulationState) && simulationState.SlowestEntity != Entity.Null)
			{
				EntityManager entityManager = World.DefaultGameObjectInjectionWorld.EntityManager;
				if (entityManager.HasComponent<BodyIndex>(simulationState.SlowestEntity))
				{
					return EntityRegister.GetById(entityManager.GetComponentData<BodyIndex>(simulationState.SlowestEntity).id) as Body;
				}
			}
			return null;
		}
	}

	[Unit(UnitType.Time)]
	public double SlowestSafeTimestep
	{
		get
		{
			SimulationState simulationState = default;
			if (!stateQuery.IsEmpty && World.DefaultGameObjectInjectionWorld.IsCreated && stateQuery.TryGetSingleton<SimulationState>(ref simulationState))
			{
				return simulationState.LowestSafeTimestep;
			}
			return 0.0;
		}
	}

	[Unit(UnitType.Time)]
	[Chartable]
	public double LowestSafeTimestep
	{
		get
		{
			SimulationProperties simulationProperties = default;
			if (!simPropertiesQuery.IsEmpty && World.DefaultGameObjectInjectionWorld.IsCreated && simPropertiesQuery.TryGetSingleton<SimulationProperties>(ref simulationProperties))
			{
				return Math.Pow(2.0, simulationProperties.SafeTimestepRange.Min);
			}
			return 0.0;
		}
	}

	[Unit(UnitType.Time)]
	[Chartable]
	public double AverageSafeTimestep
	{
		get
		{
			SimulationProperties simulationProperties = default;
			if (!simPropertiesQuery.IsEmpty && World.DefaultGameObjectInjectionWorld.IsCreated && simPropertiesQuery.TryGetSingleton<SimulationProperties>(ref simulationProperties))
			{
				return Math.Pow(2.0, simulationProperties.SafeTimestepRange.Average);
			}
			return 0.0;
		}
	}

	[Unit(UnitType.Time)]
	[Chartable]
	public double HighestSafeTimestep
	{
		get
		{
			SimulationProperties simulationProperties = default;
			if (!simPropertiesQuery.IsEmpty && World.DefaultGameObjectInjectionWorld.IsCreated && simPropertiesQuery.TryGetSingleton<SimulationProperties>(ref simulationProperties))
			{
				return Math.Pow(2.0, simulationProperties.SafeTimestepRange.Max);
			}
			return 0.0;
		}
	}

	[Unit(UnitType.Count)]
	[Chartable]
	public double LowestCorrectionIterations
	{
		get
		{
			SimulationProperties simulationProperties = default;
			if (!simPropertiesQuery.IsEmpty && World.DefaultGameObjectInjectionWorld.IsCreated && simPropertiesQuery.TryGetSingleton<SimulationProperties>(ref simulationProperties))
			{
				return simulationProperties.CorrectionIterationRange.Min;
			}
			return 0.0;
		}
	}

	[Unit(UnitType.Count)]
	[Chartable]
	public double HighestCorrectionIterations
	{
		get
		{
			SimulationProperties simulationProperties = default;
			if (!simPropertiesQuery.IsEmpty && World.DefaultGameObjectInjectionWorld.IsCreated && simPropertiesQuery.TryGetSingleton<SimulationProperties>(ref simulationProperties))
			{
				return simulationProperties.CorrectionIterationRange.Max;
			}
			return 0.0;
		}
	}

	[Unit(UnitType.Count)]
	[Chartable]
	public double AverageCorrectionIterations
	{
		get
		{
			SimulationProperties simulationProperties = default;
			if (!simPropertiesQuery.IsEmpty && World.DefaultGameObjectInjectionWorld.IsCreated && simPropertiesQuery.TryGetSingleton<SimulationProperties>(ref simulationProperties))
			{
				return simulationProperties.CorrectionIterationRange.Average;
			}
			return 0.0;
		}
	}

	[Unit(UnitType.Percent)]
	[Chartable]
	public double AdaptiveCorrectorEfficiency => 1.0 - Math.Max(0.0, AverageCorrectionIterations - 1.0) / (double)Math.Max(1, Settings.MaxCorrectionIterations);

	[Chartable]
	[PropertyInfo("e8cc34c58f6b48d79990e20e77b31dbd", "")]
	public int ObjectCount => Bodies.Count;

	[Chartable]
	[PropertyInfo("a8211fb08c504a949cac239a7b916b42", "")]
	public float FPS => FPSCounter.FPS;

	[Chartable]
	[PropertyInfo("85ccd9321aed4716b399a92f56df4391", "")]
	public float PhysFPS => FpsPhysics.FPS;

	[Chartable]
	[PropertyInfo("bc90b50a15d94caeac96db1e968d32c7", "")]
	public float PhysSteps => FpsPhysics.SubStepCount;

	[NonChartable]
	[Unit(UnitType.Gravity)]
	[PropertyInfo("ecdc00b3137544abbb1a8291c4205d9c", "")]
	public double DisplayGravity
	{
		get
		{
			return savedGravity;
		}
		set
		{
			Gravity = value;
		}
	}

	[Unit(UnitType.Generic)]
	[PropertyInfo("d98fbc64e1504d9195929bb55cbe0663", "")]
	public double GravityFactor
	{
		get
		{
			return Gravity / 6.6740831E-11;
		}
		set
		{
			Gravity = value * 6.6740831E-11;
		}
	}

	[PropertyInfo("5a3c5f25aa2c49798a7646ecf93bbaa7", "")]
	public bool ConstantCenterOfMass
	{
		get
		{
			return NBodyGlobalOptions.Instance.MaintainCenterOfMass;
		}
		set
		{
			NBodyGlobalOptions.Instance.MaintainCenterOfMass = value;
		}
	}

	public NBodyOptions NBodyOptions => NBodyGlobalOptions.Instance;

	public static TimeControlBehavior timeControlBehavior { get; private set; }

	public bool dateTargetActivated
	{
		get
		{
			return _dateTargetActivated;
		}
		set
		{
			bool num = _dateTargetActivated && !value;
			_dateTargetActivated = value;
			if (num)
			{
				dateTarget.Cancel();
			}
		}
	}

	public bool dateTargetAlive => dateTarget.isAlive;

	[Chartable]
	[Unit(UnitType.Time)]
	public double TimeDelta
	{
		get
		{
			return simTimeDelta;
		}
		set
		{
			simTimeDelta = value;
			needsFixedTimeStep = true;
		}
	}

	[Chartable]
	[Unit(UnitType.TimeStep)]
	[PropertyInfo("864c447430744fc396d5e16f4bf6b7bf", "")]
	public double TargetTimeStepPerRealSec
	{
		get
		{
			return targetTimeStepPerRealSec;
		}
		set
		{
			targetTimeStepPerRealSec = value;
			MaximalTimeStepPerRealSec = value;
		}
	}

	public double AutomaticTimeStepPerRealSec
	{
		get
		{
			return targetTimeStepPerRealSec;
		}
		set
		{
			targetTimeStepPerRealSec = value;
			if (AutoSpeedSettings.Enabled)
			{
				OnSimulationSpeedChanged?.Invoke();
			}
		}
	}

	[Unit(UnitType.TimeStep)]
	[PropertyInfo("dd476deb59b046829ec895781d166ced", "")]
	public double ActualStepPerSecond { get; private set; } = 1.0;

	public double TimeStepPrecision => ActualStepPerSecond / TargetTimeStepPerRealSec;

	public double TimeStepAvgPrecision => AverageStepPerSecond / TargetTimeStepPerRealSec;

	public double PerformanceReduction => math.min(Settings.PhysicsBreakoutTime / (double)realTimeDelta, 1.0);

	[Unit(UnitType.Time)]
	public double DeltaSimTimeElapsed => ActualSimTimeElapsed - TargetSimTimeElapsed;

	public double AccumulatedSimTimeError => ActualSimTimeElapsed / TargetSimTimeElapsed;

	public bool StabilizeSimSpeed { get; set; }

	public bool RealtimeSyncronization
	{
		get
		{
			return realtimeSyncronization;
		}
		set
		{
			if (value && !realtimeSyncronization)
			{
				ActualSimTimeElapsed = 0.0;
				TargetSimTimeElapsed = 0.0;
			}
			realtimeSyncronization = value;
		}
	}

	[PropertyInfo("e7dc8a71387c473cb550ae78bd4929db", "")]
	[Unit(UnitType.TimeStep)]
	[Chartable]
	public double AverageStepPerSecond
	{
		get
		{
			if (SuspendTimestepAveraging)
			{
				_averageStepPerSecond.Reset(0f);
				return TargetTimeStepPerRealSec;
			}
			if (IsRunning)
			{
				if (double.IsNaN(_averageStepPerSecond.AverageResult) || _averageStepPerSecond.AverageResult == 0.0)
				{
					return TargetTimeStepPerRealSec;
				}
				return _averageStepPerSecond.AverageResult;
			}
			return TargetTimeStepPerRealSec;
		}
	}

	public double TimeStepPerFrameIgnorePause { get; private set; }

	public bool IsRunning
	{
		get
		{
			if (Sandbox.SingleStep)
			{
				return true;
			}
			if (!Sandbox.Paused)
			{
				return !Sandbox.SystemPause;
			}
			return false;
		}
	}

	public bool IsRunningSlow { get; private set; }

	[Chartable]
	[PropertyInfo("f0c0b88318c4447888dcc5e6035d5394", "")]
	public int Attracting { get; private set; }

	[Chartable]
	[PropertyInfo("9660a06a035d4f1c9ba6a04567612628", "")]
	public int NonAttracting { get; private set; }

	[Chartable]
	[PropertyInfo("54d2991f06e14033b102120f8f88b505", "")]
	public int Simulated { get; private set; }

	[Chartable]
	[PropertyInfo("691facef373e4eeb911b95bc9a31bce2", "")]
	public int NonSimulated { get; private set; }

	public Vector3D CenterOfMass { get; set; }

	public List<Body> Stars => _stars;

	public Body FirstStar
	{
		get
		{
			if (_stars.Count <= 0)
			{
				return null;
			}
			return _stars[0];
		}
	}

	public Body LastBodyAdded { get; set; }

	public bool IsBodyListValid => _bodies != null;

	public ReadOnlyCollection<Group> Groups => _groups.AsReadOnly();

	public ReadOnlyCollection<Body> Bodies => _bodies.ReadOnlyCollection;

	public ReadOnlyCollection<Entity> Entities => _entities.ReadOnlyCollection;

	public List<Body> PhysicsBodies
	{
		get
		{
			List<Body> list = new List<Body>();
			foreach (Body body in Bodies)
			{
				if (!body.Suspended && !body.IsMathematicalPoint && body.DummyCamera == null)
				{
					list.Add(body);
				}
			}
			return list;
		}
	}

	[PropertyInfo("0df2e4c214204d78ae069321c86306bd", "")]
	[Chartable]
	public int NumberOfSimulatedObjects => Simulated;

	[PropertyInfo("0328c4abef1846e995fe0e311d09f09a", "")]
	[Chartable]
	public int StarCount => Stars.Count;

	[PropertyInfo("0d285f9386d54b27a032a23e6a465d34", "")]
	[Chartable]
	public int RockParticleCount => _rockParticleCount;

	[PropertyInfo("afb9840845664810bf679846692a6391", "")]
	[Chartable]
	public int DustCloudCount => _dustCloudCount;

	[Unit(UnitType.Mass)]
	[Chartable]
	[PropertyInfo("48e1ae73e04d4843b06abc949110c152", "")]
	[ReadOnly(true)]
	public double Mass { get; set; }

	[Unit(UnitType.Momentum)]
	[Chartable]
	[PropertyInfo("7c879583073449149c3d229cc944967b", "")]
	[ReadOnly(true)]
	public double Momentum { get; set; }

	[Unit(UnitType.Velocity)]
	[Chartable]
	[PropertyInfo("70735797b70a4c37b82449968392b7f9", "")]
	[ReadOnly(true)]
	public double Velocity { get; set; }

	[Unit(UnitType.Mass)]
	[Chartable]
	[PropertyInfo("57b2b9821bbd4d759720f4d50c454794", "")]
	public double MassLost { get; private set; }

	[Unit(UnitType.Momentum)]
	[Chartable]
	[PropertyInfo("4e27f174e82540fcb3704bdbb98d73d6", "")]
	public double MomentumLost { get; private set; }

	[PropertyInfo("62f8df9a804e42f89f506ad010075934", "")]
	[Unit(UnitType.Length)]
	[Chartable]
	public double MaxPositionalErrorPerStep
	{
		get
		{
			if (useMaxErrorPerTime)
			{
				return tolerancePerSec * AverageStepPerSecond;
			}
			return Tolerance;
		}
		set
		{
			useMaxErrorPerTime = false;
			Tolerance = value;
		}
	}

	[PropertyInfo("8583d9fb9e534801b15f704e3cf4a679", "")]
	[Unit(UnitType.Velocity)]
	[Chartable]
	public double MaxPositionalErrorPerSec
	{
		get
		{
			if (useMaxErrorPerTime && !AutomaticTolerance)
			{
				Tolerance = tolerancePerSec * AverageStepPerSecond;
				return tolerancePerSec;
			}
			tolerancePerSec = Tolerance / AverageStepPerSecond;
			return tolerancePerSec;
		}
		set
		{
			useMaxErrorPerTime = true;
			tolerancePerSec = value;
			Tolerance = value * AverageStepPerSecond;
		}
	}

	public uint StepsTaken { get; private set; }

	public bool Synced { get; private set; }

	public float EntitySpawnRate => (float)averageEntitySpawnRate.AverageResult;

	public event Action OnSimulationSpeedChanged;

	public static event Action TimestepScaled;

	public string GetNameToken()
	{
		if (string.IsNullOrEmpty(NameToken))
		{
			return Name;
		}
		return NameToken;
	}

	public void UpdateDateTarget(Action targetReachedCallback, Action<bool> setResultCallback)
	{
		bool obj = dateTarget.Set(targetReachedCallback);
		setResultCallback?.Invoke(obj);
	}

	static Simulation()
	{
		//IL_000f: Unknown result type (might be due to invalid IL or missing references)
		//IL_0014: Unknown result type (might be due to invalid IL or missing references)
		//IL_001f: Unknown result type (might be due to invalid IL or missing references)
		//IL_0024: Unknown result type (might be due to invalid IL or missing references)
		//IL_002b: Unknown result type (might be due to invalid IL or missing references)
		//IL_0030: Unknown result type (might be due to invalid IL or missing references)
		//IL_0035: Unknown result type (might be due to invalid IL or missing references)
		//IL_003a: Unknown result type (might be due to invalid IL or missing references)
		//IL_0049: Unknown result type (might be due to invalid IL or missing references)
		//IL_004e: Unknown result type (might be due to invalid IL or missing references)
		//IL_0055: Unknown result type (might be due to invalid IL or missing references)
		//IL_005a: Unknown result type (might be due to invalid IL or missing references)
		//IL_0061: Unknown result type (might be due to invalid IL or missing references)
		//IL_0066: Unknown result type (might be due to invalid IL or missing references)
		//IL_006b: Unknown result type (might be due to invalid IL or missing references)
		//IL_0070: Unknown result type (might be due to invalid IL or missing references)
		//IL_007f: Unknown result type (might be due to invalid IL or missing references)
		//IL_0084: Unknown result type (might be due to invalid IL or missing references)
		//IL_008b: Unknown result type (might be due to invalid IL or missing references)
		//IL_0090: Unknown result type (might be due to invalid IL or missing references)
		//IL_0095: Unknown result type (might be due to invalid IL or missing references)
		//IL_009a: Unknown result type (might be due to invalid IL or missing references)
		//IL_00a9: Unknown result type (might be due to invalid IL or missing references)
		//IL_00ae: Unknown result type (might be due to invalid IL or missing references)
		//IL_00b5: Unknown result type (might be due to invalid IL or missing references)
		//IL_00ba: Unknown result type (might be due to invalid IL or missing references)
		//IL_00bf: Unknown result type (might be due to invalid IL or missing references)
		//IL_00c4: Unknown result type (might be due to invalid IL or missing references)
		//IL_00d3: Unknown result type (might be due to invalid IL or missing references)
		//IL_00d8: Unknown result type (might be due to invalid IL or missing references)
		//IL_00dd: Unknown result type (might be due to invalid IL or missing references)
		//IL_00e2: Unknown result type (might be due to invalid IL or missing references)
		//IL_00f1: Unknown result type (might be due to invalid IL or missing references)
		//IL_00f6: Unknown result type (might be due to invalid IL or missing references)
		//IL_00fb: Unknown result type (might be due to invalid IL or missing references)
		//IL_0100: Unknown result type (might be due to invalid IL or missing references)
		timeControlBehavior = new TimeControlBehavior();
		EntityManager entityManager = World.DefaultGameObjectInjectionWorld.EntityManager;
		attractors = entityManager.CreateEntityQuery(new ComponentType[2]
		{
			ComponentType.ReadOnly<NBodyMember>(),
			ComponentType.ReadOnly<Attractor>()
		});
		nonAttractors = entityManager.CreateEntityQuery(new ComponentType[3]
		{
			ComponentType.ReadOnly<NBodyMember>(),
			ComponentType.ReadOnly<Attracted>(),
			ComponentType.Exclude<Attractor>()
		});
		allMembers = entityManager.CreateEntityQuery(new ComponentType[2]
		{
			ComponentType.ReadOnly<NBodyMember>(),
			ComponentType.ReadOnly<Attracted>()
		});
		nonMembers = entityManager.CreateEntityQuery(new ComponentType[2]
		{
			ComponentType.Exclude<NBodyMember>(),
			ComponentType.ReadOnly<Attracted>()
		});
		stateQuery = entityManager.CreateEntityQuery(new ComponentType[1] { ComponentType.ReadOnly<SimulationState>() });
		simPropertiesQuery = entityManager.CreateEntityQuery(new ComponentType[1] { ComponentType.ReadOnly<SimulationProperties>() });
	}

	public Simulation(DateTime? dt = null)
	{
		UGCMain.CurrentUGCItem = null;
		if (BuildInfo.DEBUG_SIMULATIONSTART)
		{
			Debug.Log((object)"Started Simulation constructor");
		}
		_entities = new EntitySet();
		_bodies = new BodySet();
		if (BuildInfo.DEBUG_SIMULATIONSTART)
		{
			Debug.Log((object)"Creating N-Body system ");
		}
		if (BuildInfo.DEBUG_SIMULATIONSTART)
		{
			Debug.Log((object)"Setting simulation defaults");
		}
		TargetTimeStepPerRealSec = 86400.0;
		if (BuildInfo.DEBUG_SIMULATIONSTART)
		{
			Debug.Log((object)"Assigning default integration mode and integratorID ");
		}
		IntegratorId = Integrator.ID.Hermite;
		Debug.Log((object)("Setting Default Integrator: " + Integrator.Description(IntegratorId)));
		AdaptiveIntegration = Constants.DefaultIntegrationIsAdaptive;
		UserChangedAdaptiveIntegration = false;
		if (!GuideManager.IsGuideActive)
		{
			AutoSpeedSettings.Enabled = Preferences.Instance.AutoSimulationSpeed;
		}
		Date = new DateUniversal(dt);
		dateTarget = new DateTarget(this);
		NBodyGlobalOptions.ResetToDefault();
		Gravity = 6.6740831E-11;
		global::Settings.View.Mode = ViewMode.SpaceGoggles;
		startTime = DateTime.Now;
		if (BuildInfo.DEBUG_SIMULATIONSTART)
		{
			Debug.Log((object)"Finished simulation constructor");
		}
	}

	public void Initialize()
	{
		if (BuildInfo.DEBUG_SIMULATIONSTART)
		{
			Debug.Log((object)"Starting Initialize : setting physics engine ");
		}
		isFirstFrame = true;
		AfterInitialize();
		foreach (Group group in _groups)
		{
			group.Initialize(this);
		}
		if (BuildInfo.DEBUG_SIMULATIONSTART)
		{
			Debug.Log((object)"Setting tolerances. ");
		}
		InitializeParenting();
		if (BuildInfo.DEBUG_SIMULATIONSTART)
		{
			Debug.Log((object)("Complete Initialization of Simulation " + Language.Get(Name) + " Using " + IntegratorId.ToString() + " adaptive " + AdaptiveIntegration));
		}
		ApplySettings();
	}

	public void InitializeParenting()
	{
		//IL_0005: Unknown result type (might be due to invalid IL or missing references)
		//IL_0042: Unknown result type (might be due to invalid IL or missing references)
		//IL_0048: Unknown result type (might be due to invalid IL or missing references)
		//IL_0049: Unknown result type (might be due to invalid IL or missing references)
		//IL_004e: Unknown result type (might be due to invalid IL or missing references)
		//IL_0058: Unknown result type (might be due to invalid IL or missing references)
		//IL_005e: Unknown result type (might be due to invalid IL or missing references)
		//IL_005f: Unknown result type (might be due to invalid IL or missing references)
		//IL_0064: Unknown result type (might be due to invalid IL or missing references)
		_ = World.DefaultGameObjectInjectionWorld.EntityManager;
		IntegratorBaseSystem integratorBaseSystem = World.DefaultGameObjectInjectionWorld.GetExistingSystemManaged(typeof(IntegratorBaseSystem)) as IntegratorBaseSystem;
		ReadECSDataSystem obj = World.DefaultGameObjectInjectionWorld.GetExistingSystemManaged(typeof(ReadECSDataSystem)) as ReadECSDataSystem;
		JobHandle val = default;
		val = integratorBaseSystem.GravityValidation(forceUpdate: true, val);
		val.Complete();
		val = default;
		val = obj.ReadOrbitalData(val);
		val.Complete();
		Debug.Log((object)"Initialize Orbital Parents");
	}

	private void ApplySettings()
	{
		NBodyOptions.GravityEnabled = Sandbox.ActiveSimulation.Settings.EnableNBodyGravity;
		NBodyOptions.CollisionsEnabled = Sandbox.ActiveSimulation.Settings.EnableCollisions;
		NBodyOptions.ImpactFragmentationEnabled = Sandbox.ActiveSimulation.Settings.EnableCollisionFragments && Sandbox.ActiveSimulation.Settings.EnableFragments;
		NBodyOptions.SpinFragmentationEnabled = Sandbox.ActiveSimulation.Settings.EnableSpinFragmentation && Sandbox.ActiveSimulation.Settings.EnableFragments;
		NBodyOptions.TotalFragmentationEnabled = Sandbox.ActiveSimulation.Settings.EnableTotalBodyFragmentation && Sandbox.ActiveSimulation.Settings.EnableFragments;
		NBodyOptions.RocheFragmentationEnabled = Sandbox.ActiveSimulation.Settings.EnableRocheFragmentation && Sandbox.ActiveSimulation.Settings.EnableFragments;
		NBodyOptions.FrictionFragmentationEnabled = Sandbox.ActiveSimulation.Settings.EnableSurfaceFragmentation && Sandbox.ActiveSimulation.Settings.EnableFragments;
	}

	public void Update()
	{
		if (lastRealTime == 0f)
		{
			lastRealTime = Time.time;
		}
		ApplySettings();
		Synced = false;
		IsRunningSlow = (double)(Time.time - lastRealTime) > Settings.PhysicsBreakoutTime;
		TryInitBodies();
		EntityQuery.Flush();
		Synced = AdvanceEnd();
		AdjustTimeStep();
		if (IsRunning)
		{
			activeTimeElapsed += Time.deltaTime;
			if (!Changed && activeTimeElapsed > 300.0)
			{
				Changed = true;
			}
		}
		ScanBodies();
		UpdateStatistics();
		DebugEditor();
		dateTarget.Update();
		if (isFirstFrame)
		{
			isFirstFrame = false;
			MessageDispatcher.Send("SimFirstFrame");
		}
	}

	public void AdvancePhysics()
	{
		if (Sandbox.SingleStep)
		{
			if (Sandbox.FirstSingleStepCycle)
			{
				Sandbox.FirstSingleStepCycle = false;
				simTimeDelta = 0.0;
			}
			Sandbox.Paused = true;
			if (simTimeDelta > 0.0)
			{
				Sandbox.SingleStep = false;
			}
		}
		if (Synced && IsRunning)
		{
			AdvanceBegin();
		}
		if (!IsRunning)
		{
			Synced = false;
		}
	}

	public void LateUpdate()
	{
		DebugTools.DrawLines(this);
	}

	public void AddTether(Tether tether)
	{
		Debug.Log((object)"Setting ComputationDevice to Managed, as not supported yet by tethers");
	}

	public void AddEntity(Entity entity)
	{
		if (IsActive)
		{
			EntityRegister.Add(entity);
		}
		else
		{
			_entities.Add(entity);
		}
	}

	public void AddBody(Body body, bool addToNBody = true, bool forceUpdateTolerance = false)
	{
		if (body == null)
		{
			return;
		}
		if (body.Simulation != this)
		{
			body.Simulation = this;
			_entities.Add(body);
			_bodies.Add(body);
			_entitiesToInitialize.Add(body);
			if (!body.Suspended)
			{
				LastBodyAdded = body;
			}
			if (body.Origin == BodyOrigin.Default && !Sandbox.IsLoading)
			{
				GameplayHooks.RecordBodyAdded(body);
			}
			if (IsActive)
			{
				EntityRegister.Add(body);
			}
			if (addToNBody)
			{
				_ = body.Suspended;
			}
		}
		foreach (Body item in body)
		{
			AddBody(item, addToNBody, forceUpdateTolerance);
		}
	}

	public void RemoveBody(Body body, bool removePhysicsBody = true)
	{
		if (body != null)
		{
			UpdateLossStats(body);
			EntityRegister.Remove(body);
			if (LastBodyAdded == body)
			{
				LastBodyAdded = null;
			}
			_entities.Remove(body);
			_bodies.Remove(body);
			_entitiesToInitialize.Remove(body);
		}
	}

	public void RemoveAll(Body body)
	{
		RemoveBody(body);
		foreach (Body item in body)
		{
			RemoveAll(item);
		}
	}

	private void UpdateLossStats(Body body)
	{
		if (body.KillReason == KillReason.Timeout)
		{
			MassLost += body.Mass;
			MomentumLost += body.Momentum.Length;
		}
	}

	public void Clear()
	{
		if (!IsBodyListValid)
		{
			return;
		}
		foreach (Body body in Bodies)
		{
			body.Destroy();
		}
		_bodies.Clear();
		TimePassed = 0.0;
		Date.Reset();
		_bodies = null;
	}

	public void Merge(Simulation sim)
	{
		int num = 0;
		foreach (Body body in Bodies)
		{
			num = Math.Max(num, body.Id);
		}
		foreach (Body body2 in sim.Bodies)
		{
			num = (body2.Id = num + 1);
		}
		Body.SetCounter(num);
		foreach (Body body3 in sim.Bodies)
		{
			AddBody(body3);
		}
	}

	public void AddGroup(Group group)
	{
		if (group != null && !_groups.Contains(group))
		{
			Body cameraTarget = group.GetCameraTarget();
			if (cameraTarget != null && !cameraTarget.IsDead)
			{
				MessageDispatcher.SendToMainThread("AddNameplate", (object)cameraTarget);
			}
			_groups.Add(group);
		}
	}

	public void RemoveGroup(Group group)
	{
		if (group != null)
		{
			Body cameraTarget = group.GetCameraTarget();
			if (cameraTarget != null)
			{
				MessageDispatcher.SendToMainThread("RemoveNameplate", (object)cameraTarget);
			}
			_groups.Remove(group);
			Changed = true;
		}
	}

	public int GetGroupCount()
	{
		return _groups.Count;
	}

	public Group GetGroupByIndex(int index)
	{
		return _groups[index];
	}

	public Group GetGroupById(int id)
	{
		for (int i = 0; i < _groups.Count; i++)
		{
			if (_groups[i].Id == id)
			{
				return _groups[i];
			}
		}
		return null;
	}

	private void AfterInitialize()
	{
		if (wasDeserialized)
		{
			foreach (Body body in Bodies)
			{
				body.Simulation = this;
			}
			UserCreated = true;
			wasDeserialized = false;
		}
		int num = 0;
		foreach (Body body2 in Bodies)
		{
			if (body2.Id > num)
			{
				num = body2.Id;
			}
		}
		Body.SetCounter(1 + num);
		if (BuildInfo.DEBUG_SIMULATIONSTART)
		{
			Debug.Log((object)("Highest Body Id after initialize:" + Body.IdCounter));
		}
		num = 0;
		foreach (Group group in _groups)
		{
			if (group.Id > num)
			{
				num = group.Id;
			}
		}
		Group.SetCounter(1 + num);
		List<Body> list = new List<Body>(Bodies);
		foreach (Body item in list)
		{
			item.CreateEntity();
		}
		foreach (Body item2 in list)
		{
			item2.Initialize();
		}
		Date.onTimeReversed += () =>
		{
			Utilities.ScaleGroupVelocity(Sandbox.ActiveSimulation.Bodies, -1.0);
			Utilities.ScaleGroupAngularVelocity(Sandbox.ActiveSimulation.Bodies, -1.0);
		};
	}

	private bool AdvanceEnd()
	{
		bool result = false;
		double availableFragments = Budget.GetMax(BudgetContext.CollisionFragmentation);
		NBodyGlobalOptions.Instance.AvailableFragments = availableFragments;
		if (!needsFixedTimeStep)
		{
			simTimeDelta = 0.0;
		}
		if (Screenshots.TakeSeriesSoTempPauseSim)
		{
			return false;
		}
		_averageStepPerSecond.Update();
		SimulationState singleton = stateQuery.GetSingleton<SimulationState>();
		lastStepCount = (int)StepsTaken;
		int stepCount = singleton.StepCount;
		if (singleton.StepCount > 0 || singleton.AdvanceTime == 0.0)
		{
			result = true;
			if (!IsRunning && !Sandbox.SingleStep)
			{
				lastRealTime = Time.time;
				return true;
			}
			if (!needsFixedTimeStep)
			{
				simTimeDelta = singleton.AdvanceTime;
			}
			realTimeDelta = Time.time - lastRealTime;
			lastRealTime = Time.time;
			physTimeDelta = realTimeDelta;
			StepsTaken = (uint)stepCount;
			FpsPhysics.Update(realTimeDelta, realTimeDelta, StepsTaken);
			if (realTimeDelta > 0f)
			{
				ActualStepPerSecond = simTimeDelta / (double)realTimeDelta;
				_averageStepPerSecond.Add((float)ActualStepPerSecond);
			}
			if (SuspendTimestepAveraging)
			{
				_averageStepPerSecond.Reset(0f);
				ActualStepPerSecond = TargetTimeStepPerRealSec;
			}
			TargetSimTimeElapsed += TargetTimeStepPerRealSec * (double)realTimeDelta;
			ActualSimTimeElapsed += simTimeDelta;
			lastRealTime = Time.time;
		}
		needsFixedTimeStep = false;
		return result;
	}

	private void AdvanceBegin()
	{
		if (!Sandbox.SingleStep && !Video.IsActive)
		{
			double num = (StabilizeSimSpeed ? (TargetTimeStepPerRealSec / AverageStepPerSecond) : 1.0);
			AdjustedTargetTimeStepPerSecond = (RealtimeSyncronization ? (0.0 - DeltaSimTimeElapsed + TargetTimeStepPerRealSec * num) : (TargetTimeStepPerRealSec * num));
		}
	}

	private void AdjustTimeStep()
	{
		TimeStepPerFrameIgnorePause = simTimeDelta;
		if (simTimeDelta > 0.0 && math.isfinite(simTimeDelta))
		{
			TimePassed += simTimeDelta;
			Date.AddSeconds(simTimeDelta);
		}
		if (Settings.PauseOnDate && !Sandbox.Paused && Date.SecondsUntil(Settings.PauseDate) <= 0.0)
		{
			Sandbox.Paused = true;
			Settings.PauseOnDate = false;
		}
	}

	public void TryInitBodies()
	{
		if (_entitiesToInitialize.Count < 1)
		{
			return;
		}
		foreach (Entity item in _entitiesToInitialize)
		{
			if (item is Body body)
			{
				body.Initialize();
			}
		}
		_entitiesToInitialize.Clear();
	}

	private void DebugEditor()
	{
		int i = 0;
		if (!Application.isEditor)
		{
			return;
		}
		if (Tweakables.Instance.VerifyBodies)
		{
			Debug.Log((object)"Beginning Verification of bodies..");
			int num = 0;
			while (i < Bodies.Count)
			{
				Utilities.VerifyBodyParameters(_bodies[i]);
				i++;
				num++;
			}
			Debug.LogWarning((object)("Finishing verification : Checked " + num + " bodies"));
			Tweakables.Instance.VerifyBodies = false;
		}
		else if (Tweakables.Instance.dumpAllFlags)
		{
			Debug.Log((object)"Beginning Dump of flags..");
			int num2 = 0;
			while (i < Bodies.Count)
			{
				Debug.Log((object)("Name : " + _bodies[i].Name + " Engine Flags : " + _bodies[i].BodyFlags.ToString() + " " + (int)_bodies[i].BodyFlags + " Physics Flags : " + _bodies[i].BodyFlags.ToString() + " " + (int)_bodies[i].BodyFlags));
				i++;
				num2++;
			}
			Debug.LogWarning((object)("Finishing dump of flags : count " + num2 + " bodies"));
			Tweakables.Instance.dumpAllFlags = false;
		}
		else
		{
			if (!Tweakables.Instance.dumpTargetFlags)
			{
				return;
			}
			Debug.Log((object)("Beginning search for bodies named " + Tweakables.Instance.targetBodyName));
			int num3 = 0;
			for (; i < Bodies.Count; i++)
			{
				if (_bodies[i].Name == Tweakables.Instance.targetBodyName)
				{
					Debug.Log((object)("Found " + num3 + " Flags : " + _bodies[i].BodyFlags));
					num3++;
				}
			}
			Debug.LogWarning((object)("Finishing dump of flags : found " + num3 + " bodies"));
			Tweakables.Instance.dumpTargetFlags = false;
		}
	}

	private void ScanBodies()
	{
		int rockCount = 0;
		int dustCount = 0;
		Bodies.For((Body b) =>
		{
			if (!b.IsDead && !b.Suspended)
			{
				if (b.Celestial != null && b.Celestial.Category == CelestialCategory.Star)
				{
					StarBag.Add(b);
				}
				else if (b.Particle != null && b.Particle.Type == ParticleType.None)
				{
					if (b.IsGas)
					{
						Interlocked.Increment(ref dustCount);
					}
					else
					{
						Interlocked.Increment(ref rockCount);
					}
				}
			}
		}, parallel: true);
		if (_stars.Count > 0)
		{
			_stars.Clear();
		}
		while (StarBag.Count > 0)
		{
			StarBag.TryTake(out var result);
			if (result != null)
			{
				_stars.Add(result);
			}
		}
		_rockParticleCount = rockCount;
		_dustCloudCount = dustCount;
	}

	private void UpdateStatistics()
	{
		Simulated = allMembers.CalculateEntityCount();
		NonSimulated = nonMembers.CalculateEntityCount();
		Attracting = attractors.CalculateEntityCount();
		NonAttracting = nonAttractors.CalculateEntityCount();
		SmallestRadius = double.MaxValue;
		LargestRadius = double.MinValue;
		if (IsRunning)
		{
			int count = EntityRegister.GetAll().Count;
			NumberOfNewEntities = Mathf.Max(0, count - NumberOfLastEntities);
			NumberOfLastEntities = count;
			averageEntitySpawnRate.Add(NumberOfNewEntities);
			averageEntitySpawnRate.Update();
		}
	}

	public Body GetLargestBody(bool includeBarycenters = false)
	{
		return Utilities.GroupMostMassive(Bodies, includeBarycenters);
	}

	public Body GetLargestBodybyRadius(bool includeBarycenters = false)
	{
		return Utilities.GroupMostMassivebyRadius(Bodies, includeBarycenters);
	}

	public Body GetBodyByName(string name)
	{
		if (string.IsNullOrEmpty(name))
		{
			return null;
		}
		name = name.ToLower();
		foreach (Body body in Bodies)
		{
			if (Language.Get(body.Name, LanguageCode.DEV).ToLower() == name)
			{
				return body;
			}
		}
		return null;
	}

	public Body GetBodyByNamePartial(string name)
	{
		if (string.IsNullOrEmpty(name))
		{
			return null;
		}
		name = name.ToLower();
		foreach (Body body in Bodies)
		{
			if (body.Name.ToLower().Contains(name))
			{
				return body;
			}
		}
		return null;
	}

	public Body GetBodyById(int id)
	{
		foreach (Body body in Bodies)
		{
			if (body.Id == id)
			{
				return body;
			}
		}
		return null;
	}

	public Body GetBodyByDataId(Guid guid)
	{
		foreach (Body body in Bodies)
		{
			if (body.DatabaseID == guid)
			{
				return body;
			}
		}
		return null;
	}

	public void ZeroMomentum()
	{
		foreach (Body body in Bodies)
		{
			body.Velocity = Vector3D.Zero;
		}
	}

	public void ScaleTimeStep(double factor)
	{
		if (factor > 1.0 && TargetTimeStepPerRealSec == 0.0)
		{
			TargetTimeStepPerRealSec = double.MinValue;
		}
		double num = 1f / FPSCounter.FPS * 60f;
		double num2 = 1.0 - num * (1.0 - factor);
		TargetTimeStepPerRealSec *= num2;
		if (TimestepScaled != null)
		{
			TimestepScaled();
		}
	}

	public void UpdateTolerance(double accuracy)
	{
		Tolerance = accuracy;
	}

	public void ExplodeAll()
	{
		//IL_0013: Unknown result type (might be due to invalid IL or missing references)
		UniTaskExtensions.Forget(FileTools.AutosaveAsync(waitForCompletion: false, "", () =>
		{
			//IL_0001: Unknown result type (might be due to invalid IL or missing references)
			UniTaskExtensions.Forget(ChangeTimeAndExplode());
		}));
	}

	[AsyncStateMachine(typeof(<ChangeTimeAndExplode>d__343))]
	private UniTask ChangeTimeAndExplode()
	{
		//IL_0002: Unknown result type (might be due to invalid IL or missing references)
		//IL_0007: Unknown result type (might be due to invalid IL or missing references)
		//IL_0031: Unknown result type (might be due to invalid IL or missing references)
		<ChangeTimeAndExplode>d__343 obj = default;
		obj.<>t__builder = AsyncUniTaskMethodBuilder.Create();
		obj.<>4__this = this;
		obj.<>1__state = -1;
		obj.<>t__builder.Start<<ChangeTimeAndExplode>d__343>(ref obj);
		return obj.<>t__builder.Task;
	}

	public void FromJson(JSON json, int version)
	{
		//IL_0644: Unknown result type (might be due to invalid IL or missing references)
		//IL_066d: Unknown result type (might be due to invalid IL or missing references)
		//IL_0696: Unknown result type (might be due to invalid IL or missing references)
		Name = json.ToString("Name");
		Description = json.ToString("Description");
		BuildCommit = version;
		TargetTimeStepPerRealSec = json.ToDouble("TargetTimeStepPerRealSec");
		MaximalTimeStepPerRealSec = json.ToDouble("MaximalTimeStepPerRealSec", double.MaxValue);
		Tolerance = json.ToDouble("Tolerance");
		TimePassed = json.ToDouble("TimePassed");
		Accuracy = json.ToDouble("Accuracy");
		Integrator.ID iD = (Integrator.ID)json.ToInt("IntegratorId");
		if (iD == Integrator.ID.PEFRL && version <= 27604)
		{
			IntegratorId = Integrator.ID.Hermite;
		}
		else
		{
			IntegratorId = iD;
		}
		AdaptiveIntegration = json.ToInt("IntegrationMode") != 1;
		UserChangedAdaptiveIntegration = json.ToBoolean("UserChangedAdaptiveIntegration");
		Gravity = json.ToDouble("Gravity");
		ConstantCenterOfMass = json.ToBoolean("ConstantCenterOfMass");
		if (json.Contains("AdaptiveIntegration"))
		{
			AdaptiveIntegration = json.ToBoolean("AdaptiveIntegration");
		}
		if (json.Contains("NBodySystemTolerance"))
		{
			Tolerance = json.ToDouble("NBodySystemTolerance");
		}
		if (json.Contains("Date"))
		{
			Date = new DateUniversal(json.ToString("Date"));
			Date.Active = json.ToBoolean("DateActive");
		}
		Dictionary<int, Body> dictionary = new Dictionary<int, Body>();
		JSON[] array = json.ToArray<JSON>("Groups");
		if (array != null)
		{
			JSON[] array2 = array;
			foreach (JSON jSON in array2)
			{
				if (jSON == null)
				{
					continue;
				}
				string text = jSON.ToString("$type");
				if (!(text == "ConstellationGroup"))
				{
					Type groupType = EntitySerialize.GetGroupType(text);
					if (groupType != null)
					{
						Group obj = Activator.CreateInstance(groupType) as Group;
						obj.FromJson(jSON, version);
						_groups.Add(obj);
					}
				}
			}
		}
		JSON[] array3 = null;
		array3 = ((!json.Contains("Entities")) ? json.ToArray<JSON>("Bodies") : json.ToArray<JSON>("Entities"));
		if (array3 != null)
		{
			int num = 0;
			int num2 = 0;
			JSON[] array2 = array3;
			for (int i = 0; i < array2.Length; i++)
			{
				num2 = array2[i].ToInt("Id");
				if (num2 > num)
				{
					num = num2;
				}
			}
			if (BuildInfo.DEBUG_SIMULATIONSTART)
			{
				Debug.Log((object)("Max body Id in file is:" + num));
			}
			List<BodyTypeFix> list = new List<BodyTypeFix>();
			array2 = array3;
			foreach (JSON jSON2 in array2)
			{
				string name = jSON2.ToString("$type");
				Type entityType = EntitySerialize.GetEntityType(name);
				if (entityType != null)
				{
					Entity entity = Activator.CreateInstance(entityType) as Entity;
					Body body = entity as Body;
					if (body != null)
					{
						body.Simulation = this;
					}
					entity.FromJson(jSON2, version);
					_entities.Add(entity);
					if (body == null)
					{
						continue;
					}
					if (body.Particle != null && body.Particle.Type == (ParticleType)7)
					{
						_entities.Remove(entity);
						continue;
					}
					if (dictionary.ContainsKey(body.Id))
					{
						num++;
						if (BuildInfo.DEBUG_SIMULATIONSTART)
						{
							Debug.Log((object)(body.Id + " duplicate. Assigning: " + num));
						}
						body.Id = num;
					}
					dictionary.Add(body.Id, body);
					_bodies.Add(body);
					LastBodyAdded = body;
				}
				else
				{
					Type bodyTypeFix = EntitySerialize.GetBodyTypeFix(name);
					if (bodyTypeFix != null)
					{
						BodyTypeFix bodyTypeFix2 = Activator.CreateInstance(bodyTypeFix) as BodyTypeFix;
						bodyTypeFix2.json = jSON2;
						list.Add(bodyTypeFix2);
					}
				}
			}
			foreach (BodyTypeFix item in list)
			{
				item.Fix(Bodies, dictionary);
			}
			foreach (Body body2 in Bodies)
			{
				if (body2.ParentId > -1 && dictionary.TryGetValue(body2.ParentId, out var value))
				{
					body2.Parent = value;
				}
				if (body2.SourceId > -1 && dictionary.TryGetValue(body2.SourceId, out var value2))
				{
					body2.Source = value2;
				}
				if (body2.CustomOrbitParentId > -1 && dictionary.TryGetValue(body2.CustomOrbitParentId, out var value3))
				{
					body2.CustomOrbitParent = value3;
				}
			}
		}
		JSON jSON3 = json.ToJSON("Settings");
		Settings = new SimulationSettings();
		Settings.FromJson(jSON3, version);
		View.UsePresetForMode(ViewMode.SpaceGoggles);
		if (jSON3.Contains("View"))
		{
			View.FromJson(jSON3.ToJSON("View"), version);
		}
		if (json.Contains("Pause"))
		{
			Settings.StartPaused = json.ToBoolean("Pause");
		}
		JSON json2 = json.ToJSON("Ambience");
		Ambience = new AmbienceSettings();
		Ambience.FromJson(json2, version);
		JSON json3 = json.ToJSON("AutoSpeed");
		AutoSpeedSettings = new AutoSpeedSettings();
		AutoSpeedSettings.FromJson(json3, version);
		JSON json4 = json.ToJSON("AutoCamera");
		AutoCameraSettings = new AutoCameraSettings();
		AutoCameraSettings.FromJson(json4, version);
		if (json.Contains("CustomMaterialColors"))
		{
			foreach (JSON item2 in json.ToList<JSON>("CustomMaterialColors"))
			{
				MaterialData materialByName = ResourceDB.GetMaterialByName(item2.ToString("MaterialName"));
				if (!((Object)(object)materialByName == (Object)null))
				{
					if (item2.Contains("CustomSolidColor"))
					{
						materialByName.SolidColorMode = ColorMode.Custom;
						materialByName.SolidColor = item2.ToColor("CustomSolidColor");
					}
					if (item2.Contains("CustomLiquidColor"))
					{
						materialByName.LiquidColorMode = ColorMode.Custom;
						materialByName.LiquidColor = item2.ToColor("CustomLiquidColor");
					}
					if (item2.Contains("CustomGasColor"))
					{
						materialByName.GasColorMode = ColorMode.Custom;
						materialByName.GasColor = item2.ToColor("CustomGasColor");
					}
				}
			}
		}
		if (json.Contains("CameraPath"))
		{
			CameraEditor.Instance.Deserialize(json.ToString("CameraPath"));
		}
		wasDeserialized = true;
	}

	public JSON ToJson()
	{
		//IL_0072: Unknown result type (might be due to invalid IL or missing references)
		//IL_00ad: Unknown result type (might be due to invalid IL or missing references)
		//IL_0093: Unknown result type (might be due to invalid IL or missing references)
		//IL_0098: Unknown result type (might be due to invalid IL or missing references)
		//IL_009c: Unknown result type (might be due to invalid IL or missing references)
		//IL_04c3: Unknown result type (might be due to invalid IL or missing references)
		//IL_04f1: Unknown result type (might be due to invalid IL or missing references)
		//IL_051f: Unknown result type (might be due to invalid IL or missing references)
		JSON jSON = new JSON();
		jSON["Header"] = JSONHeader.GetJSON(UboxManifest.BaseType.Simulation, "simulation.json");
		Settings.CameraTarget = (DeviceInfo.IsVR ? Camera6DOF.GetAttachedBody() : CameraBase.Target);
		Settings.CameraTargetDistance = (DeviceInfo.IsVR ? (1.0 / VirtualSpace.ScaleFactor) : CameraBase.Zoom.Value);
		Settings.CameraRootRotation = Camera6DOF.rootRotation.eulerAngles.ToVector3D();
		SimulationSettings settings = Settings;
		Vector3D cameraAngle;
		if (!DeviceInfo.IsVR)
		{
			Quaternion quat = CameraBase.Rotation.Quat;
			cameraAngle = quat.eulerAngles.ToVector3D();
		}
		else
		{
			cameraAngle = Camera6DOF.rootRotation.eulerAngles.ToVector3D();
		}
		settings.CameraAngle = cameraAngle;
		Settings.CameraPosition = (DeviceInfo.IsVR ? VirtualSpace.CameraPosition : CameraBase.Position.Vector);
		Settings.IsCameraAttached = Camera6DOF.Instance.CameraState is Attached;
		Settings.CameraAttachPoint = CameraState.AttachPoint;
		jSON["Settings"] = Settings.ToJson();
		jSON["Ambience"] = global::Settings.Ambience.ToJson();
		jSON["Name"] = Name;
		jSON["Description"] = Description;
		jSON["Date"] = Date.GetReadableString();
		jSON["DateActive"] = Date.Active;
		jSON["TargetTimeStepPerRealSec"] = TargetTimeStepPerRealSec;
		jSON["MaximalTimeStepPerRealSec"] = MaximalTimeStepPerRealSec;
		jSON["Tolerance"] = Tolerance;
		jSON["TimePassed"] = TimePassed;
		jSON["Accuracy"] = Accuracy;
		jSON["IntegratorId"] = (int)IntegratorId;
		jSON["IntegrationMode"] = (AdaptiveIntegration ? 1 : 0);
		jSON["Gravity"] = savedGravity;
		jSON["ConstantCenterOfMass"] = ConstantCenterOfMass;
		bool flag = (Settings.StartPaused = Sandbox.Paused);
		jSON["Pause"] = flag;
		jSON["AdaptiveIntegration"] = AdaptiveIntegration;
		jSON["UserChangedAdaptiveIntegration"] = UserChangedAdaptiveIntegration;
		jSON["NBodySystemTolerance"] = Tolerance;
		jSON["AutoSpeed"] = AutoSpeedSettings.ToJson();
		jSON["AutoCamera"] = AutoCameraSettings.ToJson();
		int num = 0;
		for (int i = 0; i < _groups.Count; i++)
		{
			if (!(_groups[i] is ConstellationGroup))
			{
				num++;
			}
		}
		if (num > 0)
		{
			JSON[] array = new JSON[num];
			int num2 = 0;
			for (int j = 0; j < _groups.Count; j++)
			{
				if (!(_groups[j] is ConstellationGroup))
				{
					array[num2++] = _groups[j].ToJson();
				}
			}
			jSON["Groups"] = array;
		}
		List<JSON> list = new List<JSON>();
		foreach (Entity item in EntityRegister.GetAll())
		{
			if (!item.entityFlags.Contains(EntityFlags.NonSerialized))
			{
				list.Add(item.ToJson());
			}
		}
		jSON["Entities"] = list.ToArray();
		List<MaterialData> materials = ResourceDB.GetMaterials();
		int num3 = 0;
		for (int k = 0; k < materials.Count; k++)
		{
			if (materials[k].SolidColorMode == ColorMode.Custom || materials[k].GasColorMode == ColorMode.Custom || materials[k].LiquidColorMode == ColorMode.Custom)
			{
				num3++;
			}
		}
		if (num3 > 0)
		{
			List<JSON> list2 = new List<JSON>();
			for (int l = 0; l < materials.Count; l++)
			{
				if (materials[l].SolidColorMode == ColorMode.Custom || materials[l].GasColorMode == ColorMode.Custom || materials[l].LiquidColorMode == ColorMode.Custom)
				{
					JSON jSON2 = new JSON();
					jSON2["MaterialName"] = materials[l].name;
					if (materials[l].SolidColorMode == ColorMode.Custom)
					{
						jSON2["CustomSolidColor"] = materials[l].SolidColor.ToJson();
					}
					if (materials[l].LiquidColorMode == ColorMode.Custom)
					{
						jSON2["CustomLiquidColor"] = materials[l].LiquidColor.ToJson();
					}
					if (materials[l].GasColorMode == ColorMode.Custom)
					{
						jSON2["CustomGasColor"] = materials[l].GasColor.ToJson();
					}
					list2.Add(jSON2);
				}
			}
			if (list2.Count > 0)
			{
				jSON["CustomMaterialColors"] = list2.ToArray();
			}
		}
		if (CameraEditor.ActivePath != null && !CameraEditor.ActivePath.Empty)
		{
			jSON["CameraPath"] = CameraEditor.Instance.Serialize();
		}
		return jSON;
	}

	public void Dispose()
	{
		if (!isDisposed)
		{
			isDisposed = true;
			if (dateTarget != null)
			{
				dateTarget.Dispose();
			}
			if (_entities != null)
			{
				_entities.Clear();
			}
			if (_entitiesToInitialize != null)
			{
				_entitiesToInitialize.Clear();
			}
			if (_groups != null)
			{
				_groups.Clear();
			}
		}
	}
}
