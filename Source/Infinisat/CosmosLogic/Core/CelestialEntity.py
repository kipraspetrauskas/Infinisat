using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Threading;
using Cysharp.Threading.Tasks;
using Cysharp.Threading.Tasks.CompilerServices;
using DOTS;
using DOTS.Integrators;
using CosmosLogic.Analytics;
using CosmosLogic.ECS;
using CosmosLogic.Localization;
using Mathematics;
using NBody;
using CosmosLogic.Data;
using CosmosLogic.Engine;
using CosmosLogic.Math;
using CosmosLogic.Core;
using CosmosLogic.Events;

public sealed class Body : Entity, ISerialize, IPropertyLock
{
	[CompilerGenerated]
	private sealed class <>c__DisplayClass553_0
	{
		public UIDialogResult msgBoxResult;

		internal bool <OnDropped>b__0()
		{
			return msgBoxResult != UIDialogResult.None;
		}
	}

	[StructLayout(LayoutKind.Auto)]
	[CompilerGenerated]
	private struct <FadeOutAndKill>d__36 : IAsyncStateMachine
	{
		public int <>1__state;

		public AsyncUniTaskMethodBuilder <>t__builder;

		public Body <>4__this;

		private Awaiter <>u__1;

		private Awaiter <>u__2;

		private void MoveNext()
		{
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			
			int num = <>1__state;
			Body body = <>4__this;
			try
			{
				Awaiter val;
				if (num != 0)
				{
					if (num != 1)
					{
						body.IsGhost = true;
						body.EntityFlags &= ~(EntityFlags.Attracting | EntityFlags.Attracted | EntityFlags.Collider | EntityFlags.ColliderGroupMajor | EntityFlags.Fragmenting | EntityFlags.ColliderGas);
						SetTag<RadiationSource>(body.dotsEntity, value: false);
						SetTag<Luminosity>(body.dotsEntity, value: false);
						HologramSystem.SetHoloMode(body, HologramMode.FadeOutCompletely);
						goto IL_0053;
					}
					val = <>u__2;
					<>u__2 = default;
					num = (<>1__state = -1);
					goto IL_0131;
				}
				Awaiter val2 = <>u__1;
				<>u__1 = default;
				num = (<>1__state = -1);
				goto IL_00cb;
				IL_0053:
				HologramComponent hologram = body.Hologram;
				if (hologram != null && hologram.Opacity >= 1f)
				{
					SwitchToMainThreadAwaitable val3 = UniTask.SwitchToMainThread(default(CancellationToken));
					val2 = val3.GetAwaiter();
					if (!val2.IsCompleted)
					{
						num = (<>1__state = 0);
						<>u__1 = val2;
						<>t__builder.AwaitUnsafeOnCompleted<Awaiter, <FadeOutAndKill>d__36>(ref val2, ref this);
						return;
					}
					goto IL_00cb;
				}
				YieldAwaitable val4 = UniTask.Yield();
				val = val4.GetAwaiter();
				if (!val.IsCompleted)
				{
					num = (<>1__state = 1);
					<>u__2 = val;
					<>t__builder.AwaitUnsafeOnCompleted<Awaiter, <FadeOutAndKill>d__36>(ref val, ref this);
					return;
				}
				goto IL_0131;
				IL_00cb:
				val2.GetResult();
				body.FinalizeKill();
				goto end_IL_000e;
				IL_0131:
				val.GetResult();
				goto IL_0053;
				end_IL_000e:;
			}
			catch (Exception exception)
			{
				<>1__state = -2;
				<>t__builder.SetException(exception);
				return;
			}
			<>1__state = -2;
			<>t__builder.SetResult();
		}

		void IAsyncStateMachine.MoveNext()
		{
			
			this.MoveNext();
		}

		[DebuggerHidden]
		private void SetStateMachine(IAsyncStateMachine stateMachine)
		{
			<>t__builder.SetStateMachine(stateMachine);
		}

		void IAsyncStateMachine.SetStateMachine(IAsyncStateMachine stateMachine)
		{
			
			this.SetStateMachine(stateMachine);
		}
	}

	[StructLayout(LayoutKind.Auto)]
	[CompilerGenerated]
	private struct <HandleSuspendedChanged>d__482 : IAsyncStateMachine
	{
		public int <>1__state;

		public AsyncUniTaskMethodBuilder <>t__builder;

		public Body <>4__this;

		private EntityManager <em>5__2;

		private Awaiter <>u__1;

		private void MoveNext()
		{
			
			
			
			
			
			
			
			
			
			
			
			
			
			int num = <>1__state;
			Body body = <>4__this;
			try
			{
				Awaiter val;
				if (num == 0)
				{
					val = <>u__1;
					<>u__1 = default;
					num = (<>1__state = -1);
					goto IL_008e;
				}
				if (body.SystemEngine != null)
				{
					<em>5__2 = World.DefaultGameObjectInjectionWorld.EntityManager;
					SwitchToMainThreadAwaitable val2 = UniTask.SwitchToMainThread(default(CancellationToken));
					val = val2.GetAwaiter();
					if (!val.IsCompleted)
					{
						num = (<>1__state = 0);
						<>u__1 = val;
						<>t__builder.AwaitUnsafeOnCompleted<Awaiter, <HandleSuspendedChanged>d__482>(ref val, ref this);
						return;
					}
					goto IL_008e;
				}
				goto end_IL_000e;
				IL_008e:
				val.GetResult();
				if (body.Suspended)
				{
					<em>5__2.RemoveComponent<NBodyMember>(body.dotsEntity);
				}
				else
				{
					<em>5__2.AddComponentData<NBodyMember>(body.dotsEntity, new NBodyMember
					{
						timestepScalar = ((body.Particle != null) ? double.MaxValue : body.TimeStepScalar)
					});
				}
				end_IL_000e:;
			}
			catch (Exception exception)
			{
				<>1__state = -2;
				<>t__builder.SetException(exception);
				return;
			}
			<>1__state = -2;
			<>t__builder.SetResult();
		}

		void IAsyncStateMachine.MoveNext()
		{
			
			this.MoveNext();
		}

		[DebuggerHidden]
		private void SetStateMachine(IAsyncStateMachine stateMachine)
		{
			<>t__builder.SetStateMachine(stateMachine);
		}

		void IAsyncStateMachine.SetStateMachine(IAsyncStateMachine stateMachine)
		{
			
			this.SetStateMachine(stateMachine);
		}
	}

	[StructLayout(LayoutKind.Auto)]
	[CompilerGenerated]
	private struct <OnDropped>d__553 : IAsyncStateMachine
	{
		public int <>1__state;

		public AsyncUniTaskMethodBuilder<string> <>t__builder;

		public string filename;

		public string targetFolder;

		public UboxManifest.Entry entryPoint;

		private <>c__DisplayClass553_0 <>8__1;

		private string <targetFilePath>5__2;

		private string <nameToken>5__3;

		private Awaiter <>u__1;

		private void MoveNext()
		{
			
			
			
			
			
			
			
			
			
			
			int num = <>1__state;
			string result;
			try
			{
				Awaiter val2;
				if (num != 0)
				{
					<>8__1 = new <>c__DisplayClass553_0();
					string fileName = Path.GetFileName(filename);
					<targetFilePath>5__2 = targetFolder + "/" + fileName;
					<nameToken>5__3 = entryPoint.Name.StripLiteralPrefix();
					<>8__1.msgBoxResult = UIDialogResult.None;
					if (new FileInfo(<targetFilePath>5__2).Exists)
					{
						<>c__DisplayClass553_0 obj = <>8__1;
						UIMessageBox uIMessageBox = UIMessageBox.Show(new Vector2(300f, 200f), BodyTags.Strings.userObject, "'" + <nameToken>5__3 + "'" + Language.Get(Constants.UI.STRING_REPLACE_SIM_BODY), UIMessageBoxButtons.OKCancel, UIMessageBox.Strings.btnReplace, UIMessageBox.Strings.btnCancel);
						UIDialogResult tmpResult = UIDialogResult.None;
						((CosmosEvent<UIDialogResult>)uIMessageBox.onResult).AddListener((CosmosAction<UIDialogResult>)((UIDialogResult uIDialogResult) =>
						{
							tmpResult = uIDialogResult;
						}));
						((CosmosEvent<UIBasePanel>)uIMessageBox.onAfterClose).AddListener((CosmosAction<UIBasePanel>)((UIBasePanel panel) =>
						{
							obj.msgBoxResult = tmpResult;
						}));
					}
					else
					{
						<>8__1.msgBoxResult = UIDialogResult.OK;
					}
					UniTask val = UniTask.WaitUntil((Func<bool>)(() => <>8__1.msgBoxResult != UIDialogResult.None), (PlayerLoopTiming)8, default(CancellationToken), false);
					val2 = val.GetAwaiter();
					if (!val2.IsCompleted)
					{
						num = (<>1__state = 0);
						<>u__1 = val2;
						<>t__builder.AwaitUnsafeOnCompleted<Awaiter, <OnDropped>d__553>(ref val2, ref this);
						return;
					}
				}
				else
				{
					val2 = <>u__1;
					<>u__1 = default;
					num = (<>1__state = -1);
				}
				val2.GetResult();
				if (<>8__1.msgBoxResult != UIDialogResult.OK)
				{
					result = string.Empty;
				}
				else if (!FileTools.CopyFile(filename, <targetFilePath>5__2))
				{
					Snackbar.ShowPrompt("icon_description", string.Format(BodyTags.Strings.failedImportFmt.Get(), <nameToken>5__3))?.SetAutoCloseTimer(5f, keepOpenOnHover: true);
					Debug.LogError((object)("Failed to copy User Object '" + filename + "' to '" + <targetFilePath>5__2 + "'"));
					result = string.Empty;
				}
				else
				{
					Debug.LogError((object)("Successfully copied User Object '" + filename + "' to '" + <targetFilePath>5__2 + "'"));
					Snackbar.ShowPrompt("icon_description", string.Format(BodyTags.Strings.importedSimFmt.Get(), <nameToken>5__3))?.SetAutoCloseTimer(5f, keepOpenOnHover: true);
					entryPoint.Location = <targetFilePath>5__2;
					result = <targetFilePath>5__2;
				}
			}
			catch (Exception exception)
			{
				<>1__state = -2;
				<>8__1 = null;
				<targetFilePath>5__2 = null;
				<nameToken>5__3 = null;
				<>t__builder.SetException(exception);
				return;
			}
			<>1__state = -2;
			<>8__1 = null;
			<targetFilePath>5__2 = null;
			<nameToken>5__3 = null;
			<>t__builder.SetResult(result);
		}

		void IAsyncStateMachine.MoveNext()
		{
			
			this.MoveNext();
		}

		[DebuggerHidden]
		private void SetStateMachine(IAsyncStateMachine stateMachine)
		{
			<>t__builder.SetStateMachine(stateMachine);
		}

		void IAsyncStateMachine.SetStateMachine(IAsyncStateMachine stateMachine)
		{
			
			this.SetStateMachine(stateMachine);
		}
	}

	[ThreadStatic]
	private static volatile int _idCounter;

	private Body _parent;

	private Body _sourceBody;

	private List<Body> Children = new List<Body>(1);

	public Vector3D _rotationAxis = Vector3D.Down;

	public double _rotationPeriod;

	private double? _cameraDistance;

	private Body _setAttractor;

	private Body _orbitParent;

	private Body _customOrbitParent;

	private bool defaultFlagsSet;

	private int tempCustomReferenceBody = -1;

	public Entity dotsEntity = Entity.Null;

	public bool isPooled;

	public static ObjectPool<Body> Pool = new ObjectPool<Body>();

	public ObjectPool<Body> SourcePool;

	public SmoothDouble smoothSpinRate = new SmoothDouble();

	public SmoothDouble smoothObliquity = new SmoothDouble();

	public Body Successor;

	public int BuildCommit;

	public float ScaleFactor;

	public Guid DatabaseID;

	public string HorizonID;

	private List<string> lockedProperties = new List<string>(3);

	public string Category = string.Empty;

	public bool HideLabel;

	public bool NameIsLiteral;

	private string literalName;

	public BudgetContext BudgetType;

	[PropertyInfo("6c40d4f3669e49c3b745002b6a08ccbb", "")]
	[ExposedDropdown(null)]
	public OrbitParentType OrbitParentType;

	public bool AttractorAsOrbitFallback = true;

	private double _age;

	public double TimeStepScalar = 1.0;

	private ColorMode _colorMode;

	private ColorMode _guiColorMode;

	public ColorPalette ColorPalette = ColorPalette.Composition;

	private double surfaceSoftness = 1.0;

	private Color color;

	public Color geometryColor;

	public Color CustomColor;

	public Color CustomGUIColor;

	public Color DefaultGUIColor;

	public bool UserChangedColor;

	public bool UserChangedGUIColor;

	public double VelocityLengthSquared;

	public double AccelerationLengthSquared;

	private int _groupId;

	private Group _group;

	public int CustomOrbitParentId = -1;

	private int gravityGroupID;

	private Vector3D angularVelocity;

	private double density;

	private double radius;

	private double maximumExpansionRadius;

	private double physicsMass;

	private PropertyFlags dirtyPhysicsProperties;

	private bool _isGalaxyBlackHole;

	private double _mass;

	private bool j2Enabled;

	private SimulatedMode j2Mode;

	private double _j2;

	[NonChartable]
	[Unit(UnitType.Length)]
	[PropertyInfo("fb10f38b5a3d4b0b891e05d6a341f73b", "")]
	public double BHSpin;

	public Vector3D prevPosition;

	[PropertyInfo("9a9c8019abd64db7a3dd36284530ba9f", "")]
	[ExposedDropdown(null)]
	public ReferenceBodyType RelativeTo = ReferenceBodyType.OrbitParent;

	private Vector3D velocity;

	private string name = string.Empty;

	[PropertyInfo("d9e57642af70410790e1c8f28a7aec2a", "")]
	public RotationReferenceType RotationRelativeTo = RotationReferenceType.Orbit;

	public bool ArgOfOblUndefined;

	private bool lockDeformation;

	public double ChartRadius;

	public double GravityRadius;

	private Vector3 offset;

	private double length;

	private Quaternion rotEnd = Quaternion.identity;

	private Quaternion rotCurrent = Quaternion.identity;

	private Vector3D velocityForGizmo;

	private Vector3D moveOffset;

	private Quaternion rotStart = Quaternion.identity;

	private bool isMoving;

	private bool wasPositionLocked;

	private EntityFlags savedFlags;

	private EntityFlags bodyFlags;

	public Vector3D CameraVector;

	[PropertyInfo("b845417719aa42e791039c71174a565b", "")]
	[Unit(UnitType.Length)]
	[Icon("icon_distance")]
	public double DistanceToCamera;

	public int MissedUpdates;

	internal static int IdCounter => _idCounter;

	public bool IsDead { get; private set; }

	public KillReason KillReason { get; private set; }

	public List<string> LockedProperties => lockedProperties;

	public string LocalizedName
	{
		get
		{
			if (Group != null && Group.SelectWholeGroup)
			{
				return Group.LocalizedName;
			}
			if (!NameIsLiteral)
			{
				return Name;
			}
			return literalName;
		}
	}

	public bool InCollision { get; set; }

	public bool InMajorCollision { get; private set; }

	public bool IsInitialized { get; private set; }

	public bool Suspended { get; private set; }

	public SystemEngine SystemEngine { get; set; }

	public BodyOrigin Origin { get; set; }

	[ReadOnly(true)]
	[PropertyInfo("df8092c2ebff40cf848cf6b035d23fdd", "")]
	public Body Attractor { get; set; }

	[PropertyInfo("6c40d4f3669e49c3b745002b6a08ccbb", "")]
	public Body OrbitParent
	{
		get
		{
			if (CustomOrbitParent != null)
			{
				return CustomOrbitParent;
			}
			Body body = ((GalaxyMember == null) ? (_orbitParent ?? Parent) : Parent);
			if (body == null || body.IsDead)
			{
				return null;
			}
			return body;
		}
		set
		{
			_orbitParent = value;
		}
	}

	public Body CustomOrbitParent
	{
		get
		{
			if (_customOrbitParent != null && !_customOrbitParent.IsDead)
			{
				return _customOrbitParent;
			}
			return null;
		}
		set
		{
			_customOrbitParent = value;
			CustomOrbitParentId = value?.Id ?? (-1);
		}
	}

	[NonChartable]
	[Chartable]
	[PropertyInfo("055979c4eb9d43788f988c2c2d617451", "")]
	public double RandomForChart => MathS.Random.NextDouble();

	[Lockable]
	[Chartable]
	[Unit(UnitType.Time)]
	[Icon("icon_clock_age")]
	[PropertyInfo("9ded8efdb6c9489196a2980f53f1b8be", "")]
	public double Age
	{
		get
		{
			
			
			
			
			
			
			if (dotsEntity != Entity.Null)
			{
				EntityManager entityManager = World.DefaultGameObjectInjectionWorld.EntityManager;
				if (entityManager.HasComponent<Age>(dotsEntity))
				{
					return entityManager.GetComponentData<Age>(dotsEntity).value;
				}
			}
			return _age;
		}
		set
		{
			
			
			
			
			
			
			_age = value;
			if (dotsEntity != Entity.Null)
			{
				EntityManager entityManager = World.DefaultGameObjectInjectionWorld.EntityManager;
				if (entityManager.HasComponent<Age>(dotsEntity))
				{
					entityManager.SetComponentData<Age>(dotsEntity, new Age
					{
						value = value
					});
				}
			}
		}
	}

	[Chartable]
	[Unit(UnitType.Time)]
	[PropertyInfo("29c792f6cbbc46e9b7020f0ac68ba419", "")]
	public double TimePassed => SystemEngine.TimePassed;

	public int Id { get; set; }

	[PropertyInfo("5f814ba44df24ade8617ce4837433697", "")]
	public string Description { get; set; }

	public ColorMode ColorMode
	{
		get
		{
			return _colorMode;
		}
		set
		{
			_colorMode = value;
			dirtyPhysicsProperties |= PropertyFlags.Color;
		}
	}

	[PropertyInfo("3fa2912ca55043c78e735ecb8d49f83f", "")]
	public ColorMode GUIColorMode
	{
		get
		{
			return _guiColorMode;
		}
		set
		{
			_guiColorMode = value;
			dirtyPhysicsProperties |= PropertyFlags.Color;
		}
	}

	public double SurfaceSoftness
	{
		get
		{
			return surfaceSoftness;
		}
		set
		{
			if (value != surfaceSoftness)
			{
				surfaceSoftness = value;
				dirtyPhysicsProperties |= PropertyFlags.SurfaceSoftness;
			}
		}
	}

	public bool CustomColorBool => ColorMode != ColorMode.Default;

	[PropertyInfo("5d9083d941a34a15bdecc410d5fcc136", "")]
	public Color Color
	{
		get
		{
			
			return color;
		}
		set
		{
			
			
			color = value;
			dirtyPhysicsProperties |= PropertyFlags.Color;
		}
	}

	public Color GeometryColor
	{
		get
		{
			
			return geometryColor;
		}
		set
		{
			
			
			geometryColor = value;
			dirtyPhysicsProperties |= PropertyFlags.Color;
		}
	}

	[Unit(UnitType.Velocity)]
	public Vector3D Momentum => Velocity * PhysicsMass;

	[Chartable]
	[Unit(UnitType.Volume)]
	[PropertyInfo("1a65fdb0bd9f45be803818c7a0f51ffd", "")]
	[Icon("icon_volume")]
	public double Volume => Utilities.Volume(Radius);

	public double SurfaceArea => Utilities.SurfaceAreaOfSphere(Radius);

	[Chartable]
	[Unit(UnitType.Acceleration)]
	[PropertyInfo("7aabebb512fb47e685ff978e1781851f", "")]
	[Icon("icon_surfacegravity")]
	public double SurfaceGravity => Utilities.SurfaceGravity(Mass, radius, Gravity);

	[Unit(UnitType.Velocity)]
	[PropertyInfo("b256a01e63224e66a1a653f7f59decaf", "")]
	[Icon("icon_escapevelocity")]
	public double EscapeVelocityFromSurface => Utilities.EscapeVelocity(Mass, radius, Gravity);

	[Chartable]
	[Unit(UnitType.Velocity)]
	[PropertyInfo("d24fb6ddfdbf4ed7b1ece32677ac59a4", "")]
	public double RotationSpeedAtEquator
	{
		get
		{
			return Utilities.RotationSpeedAtEquator(Radius, RotationPeriod);
		}
		set
		{
			if (value == 0.0)
			{
				RotationPeriod = 0.0;
			}
			else
			{
				RotationPeriod = Radius * (Math.PI * 2.0) / value;
			}
		}
	}

	[Chartable]
	[Unit(UnitType.Velocity)]
	[PropertyInfo("d7d96962fe4f48c7aff2a3e1a65af644", "")]
	public double RadialVelocity => Utilities.ComputeRadialVelocity(Velocity, CameraVector);

	[Chartable]
	[Unit(UnitType.Velocity)]
	[PropertyInfo("8fc0799a58c64d9e9661aa16e954d83c", "")]
	public double ProperMotionValue => ProperMotion.Length;

	public Vector3D ProperMotion => Utilities.ComputeProperMotion(VisualPos, Velocity, VirtualSpace.CameraPosition, Vector3D.Zero);

	public DisplayFlags DisplayFlags { get; set; }

	[Chartable]
	[Unit(UnitType.Percent)]
	[PropertyInfo("c0ef4971cad6487d9349341f4ee9e730", "")]
	public double NormalizedLightCurve
	{
		get
		{
			if (Celestial == null || SystemEngine == null)
			{
				return 0.0;
			}
			if ((Celestial.PresentType == CelestialType.Star || Celestial.PresentType == CelestialType.GasGiant || Celestial.PresentType == CelestialType.Planetoid || Celestial.PresentType == CelestialType.TerrestrialPlanet || Celestial.PresentType == CelestialType.TemplateObj) && !Celestial.SuperNovaInTransition)
			{
				return 1.0 - Utilities.FractionCircleBlocked(this, SystemEngine.Bodies);
			}
			return 0.0;
		}
	}

	[Chartable]
	[Unit(UnitType.Angle)]
	[PropertyInfo("e96a801a660b404f8c6baa6288058b2b", "")]
	public double CameraInclination => Math.Asin(CameraVector.y / DistanceToCamera) * (180.0 / Math.PI);

	[Chartable]
	[Unit(UnitType.Angle)]
	[PropertyInfo("0a1e96f899c44d6bbaf01f4c4db76f9a", "")]
	public double CameraAzimuth
	{
		get
		{
			Vector3D cameraVector = CameraVector;
			double num = Math.Atan2(cameraVector.x, cameraVector.z) * (180.0 / Math.PI);
			if (num < 0.0)
			{
				num = 360.0 + num;
			}
			return num;
		}
	}

	public int Generation { get; set; }

	public int ParentId { get; private set; }

	public int SourceId { get; private set; }

	public int GroupId
	{
		get
		{
			return _groupId;
		}
		set
		{
			_groupId = value;
			_group = null;
		}
	}

	public Group Group
	{
		get
		{
			if (_group == null && SystemEngine != null)
			{
				_group = SystemEngine.GetGroupById(_groupId);
			}
			return _group;
		}
	}

	public Body Source
	{
		get
		{
			return _sourceBody;
		}
		set
		{
			_sourceBody = value;
			SourceId = value?.Id ?? (-1);
		}
	}

	public Body Parent
	{
		get
		{
			return _parent;
		}
		set
		{
			if (value != this && value != _parent && (value == null || !value.IsChildOf(this)))
			{
				if (_parent != null)
				{
					_parent.Children.Remove(this);
				}
				_parent = value;
				if (_parent != null)
				{
					_parent.Children.Add(this);
				}
				ParentId = value?.Id ?? (-1);
			}
		}
	}

	public double Gravity
	{
		get
		{
			if (SystemEngine != null)
			{
				return SystemEngine.Gravity;
			}
			if (WorldMgr.ActiveSimulation != null)
			{
				return WorldMgr.ActiveSimulation.Gravity;
			}
			return 6.6740831E-11;
		}
	}

	[NonChartable]
	[PropertyInfo("dcb8298ba5464d20bb0e1050c4220a61", "")]
	[Unit(UnitType.Generic)]
	public int GravityGroup
	{
		get
		{
			return gravityGroupID;
		}
		set
		{
			dirtyPhysicsProperties |= PropertyFlags.GravityGroupID;
			gravityGroupID = value;
		}
	}

	[PropertyInfo("25ed88cc5b874900a196f70a59b2c868", "")]
	[Unit(UnitType.SpinRate)]
	public Vector3D AngularVelocity
	{
		get
		{
			return angularVelocity;
		}
		set
		{
			if (!value.IsNaN() && !value.IsInfinity())
			{
				dirtyPhysicsProperties |= PropertyFlags.AngularVelocity;
				angularVelocity = value;
				if (value.Length > 0.0)
				{
					_rotationPeriod = Utilities.RotationPeriodFromAngularVelocityLength(value.Length);
				}
				else
				{
					_rotationPeriod = 0.0;
				}
			}
		}
	}

	public Vector3D Acceleration { get; private set; }

	[PropertyInfo("f8d2df448ec04ac7bba1e12202abdaf6", "")]
	public Vector3D RotationAxis
	{
		get
		{
			return _rotationAxis;
		}
		set
		{
			_rotationAxis = value.UnitDirection;
			AngularVelocity = Utilities.AngularVelocityFromRotationPeriod(_rotationPeriod, _rotationAxis);
		}
	}

	[PropertyInfo("f8d2df448ec04ac7bba1e12202abdaf6", "")]
	public Vector3D RotationAxisWorldSpace
	{
		get
		{
			
			return RotationAxis.Multiply(EngineOrientation).UnitDirection;
		}
		set
		{
			
			
			
			
			EngineOrientation = OrbitalElementsUtils.NewRotation(EngineOrientation, RotationAxis.ToVector3(), value.UnitDirection.ToVector3());
		}
	}

	[PropertyInfo("bb3de18a13d24724bae4238492d49c79", "")]
	[Unit(UnitType.Angle, "deg")]
	[ValueRange(-90.0, 90.0)]
	public double RotationAxisLatitude
	{
		get
		{
			return Math.Asin(ToZUpFrame(RotationAxis).z) * 57.295780181884766;
		}
		set
		{
			double num = value * 0.01745329238474369;
			double num2 = RotationAxisLongitude * 0.01745329238474369;
			Vector3D v = new Vector3D(Math.Cos(num) * Math.Cos(num2), Math.Cos(num) * Math.Sin(num2), Math.Sin(num));
			RotationAxis = ToYUpFrame(v);
		}
	}

	[PropertyInfo("b393342494314b0aa0fae68091a03dbb", "")]
	[Unit(UnitType.Angle, "deg")]
	[ValueRange(0.0, 360.0)]
	public double RotationAxisLongitude
	{
		get
		{
			Vector3D vector3D = ToZUpFrame(RotationAxis);
			double num = Math.Atan2(vector3D.y, vector3D.x) * 57.295780181884766;
			if (num < 0.0)
			{
				num += 360.0;
			}
			return num;
		}
		set
		{
			double num = RotationAxisLatitude * 0.01745329238474369;
			double num2 = value * 0.01745329238474369;
			Vector3D v = new Vector3D(Math.Cos(num) * Math.Cos(num2), Math.Cos(num) * Math.Sin(num2), Math.Sin(num));
			RotationAxis = ToYUpFrame(v);
		}
	}

	[PropertyInfo("5c61ad1b12574b998220b10b847a0127", "")]
	[Icon("icon_rotation_time")]
	[Chartable]
	[Unit(UnitType.Time, "days")]
	public double RotationPeriod
	{
		get
		{
			return _rotationPeriod;
		}
		set
		{
			if (value < 0.0)
			{
				_rotationAxis *= -1.0;
				value *= -1.0;
			}
			_rotationPeriod = value;
			AngularVelocity = Utilities.AngularVelocityFromRotationPeriod(_rotationPeriod, _rotationAxis);
		}
	}

	[PropertyInfo("5c61ad1b12574b998220b10b847a0127", "")]
	[Icon("icon_rotation_time")]
	[Chartable]
	[Unit(UnitType.Time, "days")]
	public double VisualRotationPeriod
	{
		get
		{
			if (!smoothSpinRate.Active)
			{
				return RotationPeriod;
			}
			return Math.PI * 2.0 / smoothSpinRate.Goal;
		}
		set
		{
			smoothSpinRate.MoveInstantlyToGoal();
			RotationPeriod = value;
		}
	}

	public bool SpinDirection
	{
		get
		{
			return _rotationAxis.y < 0.0;
		}
		set
		{
			if (_rotationAxis.y < 0.0 && !value)
			{
				_rotationAxis.y *= -1.0;
			}
			else if (_rotationAxis.y > 0.0 && value)
			{
				_rotationAxis.y *= -1.0;
			}
			AngularVelocity = Utilities.AngularVelocityFromRotationPeriod(_rotationPeriod, _rotationAxis);
		}
	}

	[Unit(UnitType.SpinRate)]
	[PropertyInfo("32b1faed8bea402d89b594f780637b3f", "")]
	public double SpinRate
	{
		get
		{
			if (RotationPeriod != 0.0)
			{
				return Math.PI * 2.0 / RotationPeriod;
			}
			return 0.0;
		}
		set
		{
			if (value != 0.0)
			{
				RotationPeriod = Math.PI * 2.0 / value;
			}
			else
			{
				RotationPeriod = 0.0;
			}
		}
	}

	[Unit(UnitType.Acceleration)]
	[PropertyInfo("34f3300ee70341ed9e36bf923b6714d3", "")]
	public double EquatorialAcceleration => Utilities.EquatorialAcceleration(Mass, Radius, RotationPeriod, WorldMgr.ActiveSimulation.Gravity);

	public Quaternion EngineOrientationInverse
	{
		get
		{
			
			
			return Quaternion.Inverse(Transform.Rotation);
		}
	}

	public Quaternion EngineOrientation
	{
		get
		{
			
			return Transform.Rotation;
		}
		set
		{
			
			
			Transform.Rotation = value;
			dirtyPhysicsProperties |= PropertyFlags.Orientation;
		}
	}

	public Quaternion Orientation
	{
		get
		{
			
			return Transform.Rotation.ToQuaternion();
		}
		set
		{
			
			
			Transform.Rotation = value.ToQuaternion();
			dirtyPhysicsProperties |= PropertyFlags.Orientation;
		}
	}

	public Quaternion OrientationOfRotationAxis
	{
		get
		{
			
			
			
			
			return (Quaternion.LookRotation(RotationAxis.ToVector3()) * MathS.ROTATION_X_270).ToQuaternion();
		}
	}

	public Quaternion OrientationAndRotationAxis
	{
		get
		{
			
			
			
			
			
			
			return (EngineOrientation * Quaternion.LookRotation(RotationAxis.ToVector3()) * MathS.ROTATION_X_270).ToQuaternion();
		}
	}

	[PropertyInfo("b48a1fdb2abf416e84896eb8bd2052f0", "")]
	[Unit(UnitType.Angle)]
	public Vector3D OrientationReadable
	{
		get
		{
			
			
			
			
			
			
			
			Quaternion unityOrientation = EngineOrientation;
			Vector3 eulerAngles = unityOrientation.eulerAngles;
			return new Vector3D(eulerAngles.x, eulerAngles.y, eulerAngles.z);
		}
		set
		{
			
			EngineOrientation = Quaternion.Euler((float)value.x, (float)value.y, (float)value.z);
		}
	}

	[PropertyInfo("ec94b3499413438f8e413cfc8b214f9c", "")]
	[Unit(UnitType.Angle)]
	[ValueRange(0.0, 360.0)]
	public double Yaw
	{
		get
		{
			
			double num = 0f - Orientation.ToQuaternion().GetYaw();
			num = (num + 360.0) % 360.0;
			if (num < 0.001)
			{
				num = 0.0;
			}
			return num;
		}
		set
		{
			double num = value - Yaw;
			Orientation *= new Quaternion(0.0, (0.0 - num) * (double)MathS.Deg2Rad, 0.0);
		}
	}

	[DefaultValue(0.0)]
	[Icon("icon_density")]
	[PropertyInfo("5f22cd8e98cb435097793be463f11216", "")]
	[Lockable]
	[Chartable]
	[Unit(UnitType.Density)]
	public double DisplayDensity
	{
		get
		{
			return Composition?.TargetDensity ?? Density;
		}
		set
		{
			if (value < 0.0)
			{
				BodyFlagInvertedGravity = !BodyFlagInvertedGravity;
			}
			if (Composition != null)
			{
				Composition.TargetDensity = Math.Abs(value);
			}
			else
			{
				Density = Math.Abs(value);
			}
		}
	}

	public double Density
	{
		get
		{
			return density;
		}
		set
		{
			try
			{
				dirtyPhysicsProperties |= PropertyFlags.Density;
				if (Tweakables.Instance.ValidateMassRadiusDensity)
				{
					if (Utilities.IsNaNorInfinity(value) || value <= 0.0)
					{
						return;
					}
					if (value < 3.03E-111)
					{
						density = 3.03E-111;
						Debug.LogError((object)"Density is below minimum");
						return;
					}
					if (value > 1.7E+156)
					{
						density = 1.7E+156;
						Debug.LogError((object)"Density is above maximum");
						return;
					}
				}
				density = value;
			}
			catch (Exception ex)
			{
				Debug.LogException(ex);
				if (Tweakables.Instance.ValidateMassRadiusDensity)
				{
					if (value < 3.03E-111)
					{
						density = 3.03E-111;
						Debug.LogError((object)"Density is below minimum");
					}
					else if (value > 1.7E+156)
					{
						density = 1.7E+156;
						Debug.LogError((object)"Density is above maximum");
					}
				}
			}
		}
	}

	public bool IsGas
	{
		get
		{
			ParticleComponent particle = Particle;
			if (particle != null && particle.Type == ParticleType.None)
			{
				return Density < 2000.0;
			}
			return false;
		}
	}

	public bool IsStar => Mass >= 1.4898358999999998E+29;

	public bool IsMathematicalPoint
	{
		get
		{
			if (Barycenter == null)
			{
				return Lagrange != null;
			}
			return true;
		}
	}

	[DefaultValue(0.0)]
	[Icon("icon_radius")]
	[PropertyInfo("6004cf89b2ca4dbcbdf9799737266145", "")]
	[Lockable]
	[Chartable]
	[Unit(UnitType.Size)]
	public double DisplayRadius
	{
		get
		{
			if (Composition != null)
			{
				if (!Composition.RadiusTargetActive)
				{
					return Radius;
				}
				return Composition.TargetRadius;
			}
			return Radius;
		}
		set
		{
			if (Composition != null)
			{
				Composition.TargetRadius = Math.Abs(value);
			}
			else
			{
				Radius = Math.Abs(value);
			}
		}
	}

	public double Radius
	{
		get
		{
			return radius;
		}
		set
		{
			try
			{
				dirtyPhysicsProperties |= PropertyFlags.Radius;
				if (Tweakables.Instance.ValidateMassRadiusDensity)
				{
					if (Utilities.IsNaNorInfinity(value) || value <= 0.0)
					{
						return;
					}
					if (Mass <= 0.0)
					{
						if (value < 1.61619926E-35)
						{
							radius = 1.61619926E-35;
							Debug.LogError((object)("Radius is below minimum : Name : " + Name));
						}
						else
						{
							radius = value;
						}
						return;
					}
					if (value < 1.61619926E-35)
					{
						radius = 1.61619926E-35;
						Debug.LogError((object)("Radius is below minimum : Name : " + Name));
						return;
					}
					if (value > 8.70368613E+26)
					{
						radius = 8.70368613E+26;
						Debug.LogError((object)("Radius is above maximum : Name : " + Name));
						return;
					}
				}
				radius = value;
			}
			catch (Exception ex)
			{
				Debug.LogException(ex);
				if (Tweakables.Instance.ValidateMassRadiusDensity)
				{
					if (value < 1.61619926E-35)
					{
						radius = 1.61619926E-35;
						Debug.LogError((object)("Radius is below minimum : Name : " + Name));
					}
					if (value > 8.70368613E+26)
					{
						radius = 8.70368613E+26;
						Debug.LogError((object)("Radius is above maximum : Name : " + Name));
					}
				}
			}
		}
	}

	[Unit(UnitType.Size)]
	[PropertyInfo("6ff1feb5dee04838995d9e2253836ee0", "")]
	public double UncompressedRadius => Composition.CalculateUncompressedRadiusFromMassAndComposition();

	public double MaximumExpansionRadius
	{
		get
		{
			if (maximumExpansionRadius == 0.0)
			{
				maximumExpansionRadius = Radius * 5.0;
			}
			return maximumExpansionRadius;
		}
	}

	public double MomentOfInertia
	{
		get
		{
			Celestial celestial = Celestial;
			if (celestial != null && celestial.PresentType == CelestialType.Star)
			{
				return Polytrope.MomentOfInertia;
			}
			if (Composition != null)
			{
				return Composition.MomentOfInertia;
			}
			return Mathematics.Equations.Sphere.MomentOfInertia(Radius, Mass);
		}
	}

	[Unit(UnitType.GenericDouble)]
	public double MomentOfInertiaFactor => MomentOfInertia / (Mass * Radius * Radius);

	[Unit(UnitType.Percent)]
	[PropertyInfo("c43536db83eb40a7963d8d5dab76f983", "")]
	public double RadialMassGain => 1.0 - 2.5 * MomentOfInertiaFactor;

	public bool CompositionChangesRadius
	{
		get
		{
			if (Composition != null && !IsGhost && Celestial.PresentType != CelestialType.TemplateObj && Celestial.PresentType != CelestialType.BlackHole)
			{
				return !IsStar;
			}
			return false;
		}
	}

	public double RemainingFraction { get; private set; } = 1.0;

	public double InitialMass => Mass / RemainingFraction;

	public double InitialDensity => Density / RemainingFraction;

	[Chartable]
	[PropertyInfo("28eeb4170a244dc0aa3240e2d2a94ee0", "")]
	[Unit(UnitType.Size)]
	public double RocheLobeRadius { get; set; }

	public double AttractorRocheLimit
	{
		get
		{
			if (Attractor != null)
			{
				return Utilities.CalcRocheLimitFluid(Density, Attractor.Radius, Attractor.Density);
			}
			return 0.0;
		}
	}

	[Chartable]
	[PropertyInfo("fb6b942499d7443aaa3d427a963ab562", "")]
	[Unit(UnitType.Power)]
	public double TidalHeatingEffect { get; set; }

	[Chartable]
	[PropertyInfo("18d46446b77e4028ab0d490ba4f2aabe", "")]
	[Unit(UnitType.PowerPerMass)]
	public double TidalHeatingEffectPerMass { get; set; }

	public bool IsGhost { get; set; }

	public double PhysicsMass
	{
		get
		{
			return physicsMass;
		}
		set
		{
			try
			{
				dirtyPhysicsProperties |= PropertyFlags.Mass;
				if (!Utilities.IsNaNorInfinity(value))
				{
					if (value < 9.10938291E-31)
					{
						value *= -1.0;
					}
					if (Tweakables.Instance.ValidateMassRadiusDensity && value > 3E+52)
					{
						physicsMass = 3E+52;
						Debug.LogError((object)("Mass is above maximum " + Name));
					}
					else
					{
						physicsMass = value;
					}
				}
			}
			catch (Exception ex)
			{
				Debug.LogException(ex);
				if (Tweakables.Instance.ValidateMassRadiusDensity)
				{
					if (value < 9.10938291E-31)
					{
						physicsMass = 9.10938291E-31;
						Debug.LogError((object)("Mass is below minimum " + Name));
					}
					else if (value > 3E+52)
					{
						physicsMass = 3E+52;
						Debug.LogError((object)("Mass is above maximum " + Name));
					}
				}
			}
		}
	}

	public PropertyFlags PropertyFlags
	{
		get
		{
			return dirtyPhysicsProperties;
		}
		set
		{
			dirtyPhysicsProperties = value;
		}
	}

	public bool IsGalaxyBlackHole
	{
		get
		{
			return _isGalaxyBlackHole;
		}
		set
		{
			
			
			
			_isGalaxyBlackHole = value;
			if (dotsEntity != Entity.Null)
			{
				SetTag<GalaxyBlackHole>(dotsEntity, value);
			}
		}
	}

	private double galaxyBlackHoleMass
	{
		get
		{
			return _mass;
		}
		set
		{
			if (!Utilities.IsNaNorInfinity(value) && value != 0.0)
			{
				if (value < 9.10938291E-31)
				{
					_mass = 9.10938291E-31;
				}
				else if (value > 3E+52)
				{
					_mass = 3E+52;
				}
				else
				{
					_mass = value;
				}
				if (Group is Galaxy galaxy)
				{
					PhysicsMass = galaxy.TotalMass;
				}
			}
		}
	}

	[DefaultValue(0.0)]
	[Icon("icon_mass")]
	[PropertyInfo("ce966d0674f74e81b9056c2cb18363e6", "")]
	[Lockable]
	[Chartable]
	[Unit(UnitType.Mass)]
	public double DisplayMass
	{
		get
		{
			double num = Composition?.TargetMass ?? Mass;
			if (!BodyFlagInvertedGravity)
			{
				return num;
			}
			return -1.0 * num;
		}
		set
		{
			BodyFlagInvertedGravity = value < 0.0;
			if (Composition != null)
			{
				Composition.TargetMass = Math.Abs(value);
			}
			else
			{
				Mass = Math.Abs(value);
			}
		}
	}

	public double Mass
	{
		get
		{
			if (IsGalaxyBlackHole)
			{
				return galaxyBlackHoleMass;
			}
			return PhysicsMass;
		}
		set
		{
			if (IsGalaxyBlackHole)
			{
				galaxyBlackHoleMass = value;
			}
			else
			{
				PhysicsMass = value;
			}
		}
	}

	[Icon("icon_oblation_factor")]
	[PropertyInfo("50159b6de57b4993af806af93cbf0e6a", "")]
	public bool J2Enabled
	{
		get
		{
			return j2Enabled;
		}
		set
		{
			j2Enabled = value;
			dirtyPhysicsProperties |= PropertyFlags.Radius;
		}
	}

	[PropertyInfo("88aeb79a66594a93b14b969ddd41a2fd", "")]
	[DefaultValue(SimulatedMode.Simulated)]
	public SimulatedMode J2Mode
	{
		get
		{
			return j2Mode;
		}
		set
		{
			j2Mode = value;
			dirtyPhysicsProperties |= PropertyFlags.Radius;
		}
	}

	[DefaultValue(0.0)]
	[Icon("icon_oblate")]
	[PropertyInfo("9a1717c42a334abe9b7f30371d5fc389", "")]
	[Lockable]
	[Chartable]
	[Unit(UnitType.GenericDouble)]
	public double J2
	{
		get
		{
			if (!J2Enabled)
			{
				return 0.0;
			}
			return _j2;
		}
		set
		{
			_j2 = MathS.Clamp(value, 0.0, double.MaxValue);
			dirtyPhysicsProperties |= PropertyFlags.Radius;
		}
	}

	[NonChartable]
	[PropertyInfo("ce966d0674f74e81b9056c2cb18363e6", "")]
	[Chartable]
	[Unit(UnitType.Mass)]
	public double ChartValueMass
	{
		get
		{
			if (IsGalaxyBlackHole)
			{
				return (Group as Galaxy).TotalMass;
			}
			return Mass;
		}
	}

	[NonChartable]
	[PropertyInfo("6004cf89b2ca4dbcbdf9799737266145", "")]
	[Chartable]
	[Unit(UnitType.Size)]
	public double ChartValueRadius
	{
		get
		{
			if (IsGalaxyBlackHole)
			{
				return (Group as Galaxy).Radius;
			}
			return Radius;
		}
	}

	[NonChartable]
	[PropertyInfo("5f22cd8e98cb435097793be463f11216", "")]
	[Chartable]
	[Unit(UnitType.Density)]
	public double ChartValueDensity
	{
		get
		{
			if (IsGalaxyBlackHole)
			{
				Galaxy galaxy = Group as Galaxy;
				return galaxy.Radius * galaxy.Radius * galaxy.Radius / galaxy.TotalMass;
			}
			return Density;
		}
	}

	[Unit(UnitType.Length)]
	[PropertyInfo("cb7eb950559a48559c9b02a83cc88124", "")]
	public Vector3D Position
	{
		get
		{
			return Transform.Position;
		}
		set
		{
			prevPosition = Transform.Position;
			OnPositionSet?.Invoke(value);
			Transform.Position = value;
			dirtyPhysicsProperties |= PropertyFlags.Position;
		}
	}

	public Vector3D RawPosition
	{
		set
		{
			prevPosition = Transform.Position;
			Transform.Position = value;
		}
	}

	[ReadOnly(true)]
	[PropertyInfo("1961d1f767c841c8839ad6186e424851", "")]
	public Body ReferenceBody
	{
		get
		{
			Body body = RelativeTo switch
			{
				ReferenceBodyType.OrbitParent => OrbitParent, 
				ReferenceBodyType.Custom => (CustomReferenceBody == null || CustomReferenceBody.IsDead) ? null : CustomReferenceBody, 
				_ => this, 
			};
			if (body == null)
			{
				body = this;
				RelativeTo = ReferenceBodyType.Grid;
			}
			return body;
		}
	}

	public Body CustomReferenceBody { get; set; }

	[Unit(UnitType.Length)]
	[PropertyInfo("6e6e8ed3137841f69549c131edaa9b56", "")]
	public Vector3D RelativePosition
	{
		get
		{
			Vector3D position = Position;
			if (RelativeTo != ReferenceBodyType.Grid)
			{
				position -= ReferenceBody.Position;
			}
			return position;
		}
		set
		{
			Position = value;
			if (RelativeTo != ReferenceBodyType.Grid)
			{
				Position += ReferenceBody.Position;
			}
		}
	}

	[Chartable]
	[Unit(UnitType.Length)]
	[PropertyInfo("a66c718ead074eafa43c297894cfc5a4", "")]
	public double DistanceToHost
	{
		get
		{
			if (Attractor != null)
			{
				return Vector3D.Distance(Position, Attractor.Position);
			}
			return 0.0;
		}
	}

	[Chartable]
	[Unit(UnitType.Length)]
	public double DistanceToNextSelected
	{
		get
		{
			if (Selector.Selected.Count > 1)
			{
				int num = Selector.Selected.FindIndex((Body b) => b == this);
				num++;
				if (num >= Selector.Selected.Count)
				{
					num = 0;
				}
				return Vector3D.Distance(Position, Selector.Selected[num].Position);
			}
			return 0.0;
		}
	}

	[Chartable]
	[Unit(UnitType.Length)]
	[PropertyInfo("ec96bbd9094c4618bf7264a967c2595d", "")]
	public double DistanceToOrbitalParent
	{
		get
		{
			if (OrbitParent != null)
			{
				return Vector3D.Distance(Position, OrbitParent.Position) - Radius - OrbitParent.Radius;
			}
			return 0.0;
		}
	}

	[Icon("icon_distance")]
	[Chartable]
	[Unit(UnitType.Length)]
	[PropertyInfo("f5d96873e04842af8dcfd630e0bc2076", "")]
	public double RelativeDistance
	{
		get
		{
			return RelativePosition.Length - Radius - ((ReferenceBody == this) ? 0.0 : ReferenceBody.Radius);
		}
		set
		{
			Vector3D vector3D = RelativePosition.UnitDirection;
			if (vector3D == Vector3D.Zero)
			{
				vector3D = Vector3D.Right;
			}
			RelativePosition = vector3D * (value + Radius + ((ReferenceBody == this) ? 0.0 : ReferenceBody.Radius));
		}
	}

	[Unit(UnitType.Velocity)]
	[PropertyInfo("26b1b76bd7bb48049c56b8dad1403f85", "")]
	public Vector3D Velocity
	{
		get
		{
			return velocity;
		}
		set
		{
			dirtyPhysicsProperties |= PropertyFlags.Velocity;
			OnVelocitySet?.Invoke(value);
			velocity = value;
		}
	}

	public Vector3D RawVelocity
	{
		set
		{
			velocity = value;
		}
	}

	[Unit(UnitType.Velocity)]
	[PropertyInfo("276f00526fff46b0b2b7d4fdc6bdd49e", "")]
	public Vector3D RelativeVelocity
	{
		get
		{
			Vector3D result = Velocity;
			if (RelativeTo != ReferenceBodyType.Grid)
			{
				result -= ReferenceBody.Velocity;
			}
			return result;
		}
		set
		{
			Velocity = value;
			if (RelativeTo != ReferenceBodyType.Grid)
			{
				Velocity += ReferenceBody.Velocity;
			}
		}
	}

	[Chartable]
	[Icon("icon_speed")]
	[PropertyInfo("6ae6a503d5cf4cfab479c8353ec50b48", "")]
	[Unit(UnitType.Velocity)]
	public double VelocityLength
	{
		get
		{
			return Velocity.Length;
		}
		set
		{
			Velocity = Velocity.UnitDirection * value;
		}
	}

	[Icon("icon_label")]
	[PropertyInfo("c87a78f4af3244ccab4bb09919148cc1", "")]
	public override string Name
	{
		get
		{
			return name;
		}
		set
		{
			if (string.IsNullOrWhiteSpace(value))
			{
				literalName = (name = string.Empty);
				return;
			}
			literalName = "@" + value;
			name = value;
		}
	}

	[Unit(UnitType.Velocity)]
	[Icon("icon_motion")]
	[PropertyInfo("239956c6f85c451ba5422d5b36dfcae1", "")]
	public double VelocityRelativeLength
	{
		get
		{
			if (Attractor == null)
			{
				return VelocityLength;
			}
			return (Velocity - Attractor.Velocity).Length;
		}
		set
		{
			if (Attractor == null)
			{
				VelocityLength = value;
			}
			else
			{
				Velocity = (Velocity - Attractor.Velocity).UnitDirection * value + Attractor.Velocity;
			}
		}
	}

	[Icon("icon_speed")]
	[Chartable]
	[Unit(UnitType.Velocity)]
	[PropertyInfo("a6d7eb7800f34908a4cf44428fe25859", "")]
	public double RelativeSpeed
	{
		get
		{
			return RelativeVelocity.Length;
		}
		set
		{
			Vector3D vector3D = RelativeVelocity.UnitDirection;
			if (vector3D == Vector3D.Zero)
			{
				vector3D = Vector3D.Forward;
			}
			RelativeVelocity = vector3D * value;
		}
	}

	public Vector3D RelativeX
	{
		get
		{
			if (OrbitParent == null || Elements == null)
			{
				RotationRelativeTo = RotationReferenceType.Grid;
			}
			RotationReferenceType rotationRelativeTo = RotationRelativeTo;
			if (rotationRelativeTo == RotationReferenceType.Grid || rotationRelativeTo != RotationReferenceType.Orbit)
			{
				return new Vector3D(-1.0, 0.0, 0.0);
			}
			return (Elements.GetPericenterPointPosition() - OrbitParent.Position).Normalize();
		}
	}

	public Vector3D RelativeY
	{
		get
		{
			if (OrbitParent == null || Elements == null)
			{
				RotationRelativeTo = RotationReferenceType.Grid;
			}
			RotationReferenceType rotationRelativeTo = RotationRelativeTo;
			if (rotationRelativeTo == RotationReferenceType.Grid || rotationRelativeTo != RotationReferenceType.Orbit)
			{
				return new Vector3D(0.0, 1.0, 0.0);
			}
			return Elements.orbitNormal;
		}
	}

	public Vector3D RelativeZ
	{
		get
		{
			if (OrbitParent == null || Elements == null)
			{
				RotationRelativeTo = RotationReferenceType.Grid;
			}
			RotationReferenceType rotationRelativeTo = RotationRelativeTo;
			if (rotationRelativeTo == RotationReferenceType.Grid || rotationRelativeTo != RotationReferenceType.Orbit)
			{
				return new Vector3D(0.0, 0.0, -1.0);
			}
			return Vector3D.Cross(RelativeX, RelativeY);
		}
	}

	public double CameraDistance
	{
		get
		{
			double num = Radius;
			if (Group != null)
			{
				num = Group.GetCameraDistance(this);
			}
			else if (_cameraDistance.HasValue)
			{
				num = _cameraDistance.Value;
			}
			else if (WorldMgr.Mode == SandboxState.Chart)
			{
				num = VisualRadius;
			}
			if (Celestial != null && Celestial.PresentType == CelestialType.BlackHole && !IsGalaxyBlackHole)
			{
				num *= 10.0;
			}
			return num;
		}
		set
		{
			_cameraDistance = value;
		}
	}

	public bool IsCameraDistanceSet => _cameraDistance.HasValue;

	[Chartable]
	[Unit(UnitType.Angle)]
	[ValueRange(0.0, 180.0)]
	[PropertyInfo("d7940e2faa3c4b6eb3638f0f99e783ed", "")]
	public double RelativeObliquity
	{
		get
		{
			double num = (Math.PI - Math.Acos(Vector3D.Dot(RelativeY, RotationAxisWorldSpace))) * (180.0 / Math.PI);
			if (num < 0.001)
			{
				num = 0.0;
			}
			return num;
		}
		set
		{
			double num = value * (Math.PI / 180.0);
			Vector3D v = Vector3D.Cross(RelativeY, RotationAxisWorldSpace);
			if (v.LengthSquared < 1E-06)
			{
				v = Vector3D.GenerateOrthogonalVector(RelativeY);
			}
			v = v.Normalize();
			Vector3D vector3D = -1.0 * Vector3D.Cross(v, RelativeY);
			Vector3D v2 = Math.Sin(num) * vector3D + Math.Cos(num) * RelativeY;
			v2 = v2.Normalize();
			ArgOfOblUndefined = false;
			if (Math.Abs(num % (Math.PI * 2.0)) <= 1E-06)
			{
				ArgOfOblUndefined = true;
				v2 = RelativeY;
			}
			if ((Math.Abs(num) + Math.PI) % (Math.PI * 2.0) <= 1E-06)
			{
				ArgOfOblUndefined = true;
				v2 = -1.0 * RelativeY;
			}
			RotationAxisWorldSpace = v2;
		}
	}

	[Chartable]
	[Unit(UnitType.Angle)]
	[ValueRange(-180.0, 180.0)]
	[PropertyInfo("4d79dfb917754b41883c1ffabee9b6da", "")]
	public double RelativeArgOfObliquity
	{
		get
		{
			double x = Vector3D.Dot(RelativeX, RotationAxisWorldSpace);
			double num = Math.Atan2(Vector3D.Dot(RelativeZ, RotationAxisWorldSpace), x) * (180.0 / Math.PI);
			if (num < 0.001 && num > -0.001)
			{
				num = 0.0;
			}
			return num;
		}
		set
		{
			double num = value * (Math.PI / 180.0);
			double num2 = RelativeObliquity * (Math.PI / 180.0);
			Vector3D rotationAxisWorldSpace = -1.0 * RelativeX * (Math.Cos(num) * Math.Sin(num2)) + RelativeY * Math.Cos(num2) + -1.0 * RelativeZ * (Math.Sin(num) * Math.Sin(num2));
			RotationAxisWorldSpace = rotationAxisWorldSpace;
		}
	}

	[PropertyInfo("5060e619dcb944b79b333ee2ecb19623", "")]
	public bool LockRotation
	{
		get
		{
			return (EntityFlags & EntityFlags.LockRotation) != 0;
		}
		set
		{
			if (value)
			{
				EntityFlags |= EntityFlags.LockRotation;
			}
			else
			{
				EntityFlags &= ~EntityFlags.LockRotation;
			}
		}
	}

	[PropertyInfo("515e3a9ce75e49deb2af574c8bfea6aa", "")]
	public bool LockPosition
	{
		get
		{
			return (EntityFlags & EntityFlags.LockPosition) != 0;
		}
		set
		{
			if (value)
			{
				EntityFlags |= EntityFlags.LockPosition;
			}
			else
			{
				EntityFlags &= ~EntityFlags.LockPosition;
			}
		}
	}

	[PropertyInfo("bb8dcdf89ee64cf2b29374c6f4047d54", "")]
	public bool LockDeformation
	{
		get
		{
			return lockDeformation;
		}
		set
		{
			
			
			
			lockDeformation = value;
			if (dotsEntity != Entity.Null)
			{
				SetTag<LockDeformation>(dotsEntity, value);
			}
		}
	}

	[Icon("icon_attracting")]
	[PropertyInfo("d57d1caf261745e1a7aabf8f4d71d174", "")]
	public bool BodyFlagAttracting
	{
		get
		{
			return (EntityFlags & EntityFlags.Attracting) != 0;
		}
		set
		{
			if (IsInitialized)
			{
				if (value)
				{
					EntityFlags |= EntityFlags.Attracting;
				}
				else
				{
					EntityFlags &= ~EntityFlags.Attracting;
				}
				return;
			}
			if (savedFlags == EntityFlags.None)
			{
				savedFlags = CalculateDefaultFlags();
			}
			if (value)
			{
				savedFlags |= EntityFlags.Attracting;
			}
			else
			{
				savedFlags &= ~EntityFlags.Attracting;
			}
		}
	}

	[Icon("icon_attracted")]
	[PropertyInfo("40d21027d3564c6e983f8ce0583b7a47", "")]
	public bool BodyFlagAttracted
	{
		get
		{
			return (EntityFlags & EntityFlags.Attracted) != 0;
		}
		set
		{
			if (IsInitialized)
			{
				if (value)
				{
					EntityFlags |= EntityFlags.Attracted;
				}
				else
				{
					EntityFlags &= ~EntityFlags.Attracted;
				}
				return;
			}
			if (savedFlags == EntityFlags.None)
			{
				savedFlags = CalculateDefaultFlags();
			}
			if (value)
			{
				savedFlags |= EntityFlags.Attracted;
			}
			else
			{
				savedFlags &= ~EntityFlags.Attracted;
			}
		}
	}

	[Icon("icon_collision")]
	[PropertyInfo("2aaec5343b9045dab64ba9c66e8de79b", "")]
	public bool BodyFlagCollider
	{
		get
		{
			return (EntityFlags & EntityFlags.Collider) != 0;
		}
		set
		{
			if (IsInitialized)
			{
				if (value)
				{
					EntityFlags |= EntityFlags.Collider;
				}
				else
				{
					EntityFlags &= ~EntityFlags.Collider;
				}
				return;
			}
			if (savedFlags == EntityFlags.None)
			{
				savedFlags = CalculateDefaultFlags();
			}
			if (value)
			{
				savedFlags |= EntityFlags.Collider;
			}
			else
			{
				savedFlags &= ~EntityFlags.Collider;
			}
		}
	}

	[PropertyInfo("80f839e15a62485e886d56f2214106ce", "")]
	public bool BodyFlagMomentumless
	{
		get
		{
			return (EntityFlags & EntityFlags.MomentumLess) != 0;
		}
		set
		{
			if (IsInitialized)
			{
				if (value)
				{
					EntityFlags |= EntityFlags.MomentumLess;
				}
				else
				{
					EntityFlags &= ~EntityFlags.MomentumLess;
				}
				return;
			}
			if (savedFlags == EntityFlags.None)
			{
				savedFlags = CalculateDefaultFlags();
			}
			if (value)
			{
				savedFlags |= EntityFlags.MomentumLess;
			}
			else
			{
				savedFlags &= ~EntityFlags.MomentumLess;
			}
		}
	}

	[Icon("icon_momentum")]
	[PropertyInfo("ac628c3aad6844418a38e25a0f57b592", "")]
	public bool TransferMomentum
	{
		get
		{
			return !BodyFlagMomentumless;
		}
		set
		{
			BodyFlagMomentumless = !value;
		}
	}

	[Icon("icon_inverse")]
	[PropertyInfo("6e600e15e3034b07abc862f081fb769f", "")]
	public bool BodyFlagInvertedGravity
	{
		get
		{
			return (EntityFlags & EntityFlags.InvertedGravity) != 0;
		}
		set
		{
			if (IsInitialized)
			{
				if (value)
				{
					EntityFlags |= EntityFlags.InvertedGravity;
				}
				else
				{
					EntityFlags &= ~EntityFlags.InvertedGravity;
				}
				return;
			}
			if (savedFlags == EntityFlags.None)
			{
				savedFlags = CalculateDefaultFlags();
			}
			if (value)
			{
				savedFlags |= EntityFlags.InvertedGravity;
			}
			else
			{
				savedFlags &= ~EntityFlags.InvertedGravity;
			}
		}
	}

	[PropertyInfo("5af30120ec9d4e889feb4e509ae83e72", "")]
	public bool BodyFlagColliderGroupMajor
	{
		get
		{
			return (EntityFlags & EntityFlags.ColliderGroupMajor) != 0;
		}
		set
		{
			if (IsInitialized)
			{
				if (value)
				{
					EntityFlags |= EntityFlags.ColliderGroupMajor;
				}
				else
				{
					EntityFlags &= ~EntityFlags.ColliderGroupMajor;
				}
				return;
			}
			if (savedFlags == EntityFlags.None)
			{
				savedFlags = CalculateDefaultFlags();
			}
			if (value)
			{
				savedFlags |= EntityFlags.ColliderGroupMajor;
			}
			else
			{
				savedFlags &= ~EntityFlags.ColliderGroupMajor;
			}
		}
	}

	[Icon("icon_fragments")]
	[PropertyInfo("26066df6acc5489f9db78550489573c7", "")]
	public bool BodyFlagFragmenting
	{
		get
		{
			return (EntityFlags & EntityFlags.Fragmenting) != 0;
		}
		set
		{
			if (IsInitialized)
			{
				if (value)
				{
					EntityFlags |= EntityFlags.Fragmenting;
				}
				else
				{
					EntityFlags &= ~EntityFlags.Fragmenting;
				}
				return;
			}
			if (savedFlags == EntityFlags.None)
			{
				savedFlags = CalculateDefaultFlags();
			}
			if (value)
			{
				savedFlags |= EntityFlags.Fragmenting;
			}
			else
			{
				savedFlags &= ~EntityFlags.Fragmenting;
			}
		}
	}

	[PropertyInfo("d08859d830d64837a4c43497d9b5741c", "")]
	public bool BodyFlagWantOrbit
	{
		get
		{
			return (EntityFlags & EntityFlags.TidalEffects) != 0;
		}
		set
		{
			if (IsInitialized)
			{
				if (value)
				{
					EntityFlags |= EntityFlags.TidalEffects;
				}
				else
				{
					EntityFlags &= ~EntityFlags.TidalEffects;
				}
				return;
			}
			if (savedFlags == EntityFlags.None)
			{
				savedFlags = CalculateDefaultFlags();
			}
			if (value)
			{
				savedFlags |= EntityFlags.TidalEffects;
			}
			else
			{
				savedFlags &= ~EntityFlags.TidalEffects;
			}
		}
	}

	[PropertyInfo("4744b39fb8c042199593e45f1d1f3c89", "")]
	public bool ToggleTrailComponent
	{
		get
		{
			if (Trail != null)
			{
				return true;
			}
			return false;
		}
		set
		{
			if (value)
			{
				if (Trail == null)
				{
					AddComponent<TrailComponent>().Initialize();
				}
			}
			else if (Trail != null)
			{
				RemoveComponent(Trail);
			}
		}
	}

	public EntityFlags EntityFlags
	{
		get
		{
			return bodyFlags;
		}
		set
		{
			
			
			EntityFlags bodyFlags = this.bodyFlags ^ value;
			this.bodyFlags = value;
			dirtyPhysicsProperties |= PropertyFlags.EntityFlags;
			if (bodyFlags != EntityFlags.None && dotsEntity != Entity.Null)
			{
				SyncFlagTags(bodyFlags, value);
			}
		}
	}

	public static EntityFlags DefaultFragmentFlags
	{
		get
		{
			if (WorldMgr.ActiveSimulation.UseTree)
			{
				return EntityFlags.Attracting | EntityFlags.Attracted | EntityFlags.Collider;
			}
			return EntityFlags.Attracted | EntityFlags.Collider;
		}
	}

	public bool IsAttracting => (EntityFlags & EntityFlags.Attracting) != 0;

	[PropertyInfo("f62e695a940b46a1ad356ae9828100cd", "")]
	[Unit(UnitType.Length)]
	public double DistanceToClosestStar { get; set; }

	public GameObject gameObject
	{
		get
		{
			return GameObject?.gameObject;
		}
		set
		{
			if (GameObject != null)
			{
				GameObject.gameObject = value;
			}
		}
	}

	public Vector3 TransformedPosition
	{
		get
		{
			
			return Transform.TransformedPosition;
		}
		set
		{
			
			
			Transform.TransformedPosition = value;
		}
	}

	public float TransformedRadius
	{
		get
		{
			return Transform.TransformedRadius;
		}
		set
		{
			Transform.TransformedRadius = value;
		}
	}

	public double VisualRadius
	{
		get
		{
			return Transform.VisualRadius;
		}
		set
		{
			Transform.VisualRadius = value;
		}
	}

	public Vector3D VisualPos
	{
		get
		{
			return Transform.VisualPos;
		}
		set
		{
			Transform.VisualPos = value;
		}
	}

	public bool Selected
	{
		get
		{
			if (Selectable != null)
			{
				return Selectable.Selected;
			}
			return false;
		}
		set
		{
			if (Selectable != null)
			{
				Selectable.Selected = value;
			}
		}
	}

	public float PixelSize
	{
		get
		{
			if (Selectable != null)
			{
				return Selectable.PixelSize;
			}
			return 0f;
		}
		set
		{
			if (Selectable != null)
			{
				Selectable.PixelSize = value;
			}
		}
	}

	public float LabelAlpha
	{
		get
		{
			if (Selectable != null)
			{
				return Selectable.LabelAlpha;
			}
			return 0f;
		}
		set
		{
			if (Selectable != null)
			{
				Selectable.LabelAlpha = value;
			}
		}
	}

	public float FadeHover
	{
		get
		{
			if (Selectable != null)
			{
				return Selectable.FadeHover;
			}
			return 0f;
		}
		set
		{
			if (Selectable != null)
			{
				Selectable.FadeHover = value;
			}
		}
	}

	public float FadeSelect
	{
		get
		{
			if (Selectable != null)
			{
				return Selectable.FadeSelect;
			}
			return 0f;
		}
		set
		{
			if (Selectable != null)
			{
				Selectable.FadeSelect = value;
			}
		}
	}

	public Color GUIColor
	{
		get
		{
			
			
			if (Selectable != null)
			{
				return Selectable.GUIColor;
			}
			return Color.white;
		}
		set
		{
			
			
			if (Selectable != null)
			{
				Selectable.GUIColor = value;
				dirtyPhysicsProperties |= PropertyFlags.Color;
			}
		}
	}

	public Vector3 ScreenPosition
	{
		get
		{
			
			
			if (Selectable != null)
			{
				return Selectable.ScreenPosition;
			}
			return Vector3.zero;
		}
		set
		{
			
			
			if (Selectable != null)
			{
				Selectable.ScreenPosition = value;
			}
		}
	}

	public event Action<KillReason> OnKill;

	public event Action<Vector3D> OnPositionSet;

	public event Action<Vector3D> OnVelocitySet;

	public static void SetCounter(int id)
	{
		_idCounter = id;
	}

	public static void ResetId()
	{
		if (BuildInfo.DEBUG_SIMULATIONSTART)
		{
			Debug.Log((object)"body ID counter reset to 0");
		}
		_idCounter = 0;
	}

	public static int NewId()
	{
		checked
		{
			try
			{
				_idCounter++;
			}
			catch (OverflowException)
			{
				_idCounter = 0;
			}
			return _idCounter;
		}
	}

	public static int LastId()
	{
		return _idCounter;
	}

	public void Kill(KillReason reason = KillReason.None)
	{
		
		if (IsDead)
		{
			return;
		}
		KillReason = reason;
		savedFlags = EntityFlags;
		GameplayHooks.RecordBodyKilled(reason, Origin);
		if (Celestial != null && reason == KillReason.Deletion)
		{
			HologramComponent hologram = Hologram;
			if (hologram == null || hologram.Mode != HologramMode.FadeOutCompletely)
			{
				UniTaskExtensions.Forget(FadeOutAndKill());
			}
		}
		else
		{
			FinalizeKill();
		}
	}

	private void FinalizeKill()
	{
		IsDead = true;
		OnKill?.Invoke(KillReason);
		HologramSystem.SetHoloMode(this, HologramMode.Solid);
		if (KillReason == KillReason.Deletion || KillReason == KillReason.Replacement)
		{
			if (IsGalaxyBlackHole)
			{
				(Group as Galaxy).RemoveBlackHole();
			}
			Terminal.Event(TerminalEvent.BodyDeleted, Name);
		}
	}

	[AsyncStateMachine(typeof(<FadeOutAndKill>d__36))]
	private UniTask FadeOutAndKill()
	{
		
		
		
		<FadeOutAndKill>d__36 obj = default;
		obj.<>t__builder = AsyncUniTaskMethodBuilder.Create();
		obj.<>4__this = this;
		obj.<>1__state = -1;
		obj.<>t__builder.Start<<FadeOutAndKill>d__36>(ref obj);
		return obj.<>t__builder.Task;
	}

	public void ScheduleKill(KillReason reason)
	{
		
		
		
		
		EntityManager entityManager = World.DefaultGameObjectInjectionWorld.EntityManager;
		if (!entityManager.HasComponent<DestroyEntity>(dotsEntity))
		{
			entityManager.AddComponentData<DestroyEntity>(dotsEntity, new DestroyEntity
			{
				KillReason = reason
			});
		}
	}

	public void RemapId(Dictionary<int, int> remap)
	{
		if (ParentId > -1 && remap.TryGetValue(ParentId, out var value))
		{
			ParentId = value;
		}
		if (SourceId > -1 && remap.TryGetValue(SourceId, out var value2))
		{
			SourceId = value2;
		}
		foreach (BaseComponent component in Components)
		{
			if (component is IIndexComponent indexComponent)
			{
				indexComponent.RemapId(remap);
			}
		}
	}

	private Vector3D ToZUpFrame(Vector3D v)
	{
		return new Vector3D(v.z, v.x, 0.0 - v.y);
	}

	private Vector3D ToYUpFrame(Vector3D v)
	{
		return new Vector3D(v.y, 0.0 - v.z, v.x);
	}

	public void TidallyLock()
	{
		if (Elements != null && !(Elements.OrbitalPeriod <= 0.0))
		{
			smoothObliquity.Value = RelativeObliquity;
			smoothObliquity.Goal = 0.0;
			smoothObliquity.SmoothType = FadeType.Smooth;
			smoothObliquity.LengthInSec = 2.0;
			smoothSpinRate.Value = angularVelocity.Length;
			smoothSpinRate.Goal = Math.PI * 2.0 / Elements.OrbitalPeriod;
			smoothSpinRate.SmoothType = FadeType.Smooth;
			smoothSpinRate.LengthInSec = 3.0;
		}
	}

	public void setMomentumAwareMass(double mass)
	{
		Vector3D vector3D = Mass * Velocity;
		Vector3D vector3D2 = Mathematics.Equations.Sphere.AngularSpinMomentumObject(Mass, Radius, AngularVelocity);
		Mass = mass;
		Velocity = vector3D / Mass;
		AngularVelocity = vector3D2 / Mathematics.Equations.Sphere.MomentOfInertia(Radius, Mass);
	}

	public void StabilizeDeformation()
	{
		
		
		
		
		
		
		if (!(dotsEntity == Entity.Null))
		{
			EntityManager entityManager = World.DefaultGameObjectInjectionWorld.EntityManager;
			entityManager.AddComponent<StabilizeDeformation>(dotsEntity);
			entityManager.SetComponentEnabled<StabilizeDeformation>(dotsEntity, true);
		}
	}

	private static void SetTag<T>(Entity entity, bool value)
	{
		
		
		
		
		
		
		
		EntityManager entityManager = World.DefaultGameObjectInjectionWorld.EntityManager;
		if (!entityManager.Exists(entity))
		{
			return;
		}
		if (value)
		{
			if (!entityManager.HasComponent<T>(entity))
			{
				entityManager.AddComponent<T>(entity);
			}
		}
		else if (entityManager.HasComponent<T>(entity))
		{
			entityManager.RemoveComponent<T>(entity);
		}
	}

	private void SyncFlagTags(EntityFlags changedFlags, EntityFlags flags)
	{
		
		
		
		
		
		
		
		
		
		if ((changedFlags & EntityFlags.Attracting) != 0)
		{
			SetTag<Attractor>(dotsEntity, (flags & EntityFlags.Attracting) != 0);
		}
		if ((changedFlags & EntityFlags.Attracted) != 0)
		{
			SetTag<Attracted>(dotsEntity, (flags & EntityFlags.Attracted) != 0);
		}
		if ((changedFlags & EntityFlags.Collider) != 0)
		{
			SetTag<CanCollide>(dotsEntity, (flags & EntityFlags.Collider) != 0);
		}
		if ((changedFlags & EntityFlags.ColliderGroupMajor) != 0)
		{
			SetTag<MajorCollider>(dotsEntity, (flags & EntityFlags.ColliderGroupMajor) != 0);
		}
		if ((changedFlags & EntityFlags.Fragmenting) != 0)
		{
			SetTag<CanFragment>(dotsEntity, (flags & EntityFlags.Fragmenting) != 0);
		}
		if ((changedFlags & EntityFlags.MomentumLess) != 0)
		{
			SetTag<Momentumless>(dotsEntity, (flags & EntityFlags.MomentumLess) != 0);
		}
		if ((changedFlags & EntityFlags.InvertedGravity) != 0)
		{
			SetTag<InvertedGravity>(dotsEntity, (flags & EntityFlags.InvertedGravity) != 0);
		}
		if ((changedFlags & EntityFlags.LockPosition) != 0)
		{
			SetTag<LockPosition>(dotsEntity, (flags & EntityFlags.LockPosition) != 0);
		}
		if ((changedFlags & EntityFlags.LockRotation) != 0)
		{
			SetTag<LockRotation>(dotsEntity, (flags & EntityFlags.LockRotation) != 0);
		}
	}

	public Body(ObjectPool<Body> pool)
	{
		
		
		
		
		
		
		
		
		
		
		EntityFlags = EntityFlags.None;
		ParentId = (SourceId = -1);
		Id = NewId();
		AddComponentRaw<TransformComponent>();
		Color = ColorHelper.RandomBodyColor();
		GeometryColor = Color;
	}

	public Body()
	{
		
		
		
		
		
		
		
		
		
		
		
		
		EntityFlags = EntityFlags.None;
		ParentId = (SourceId = -1);
		AddComponentRaw<TransformComponent>();
		Id = NewId();
		Color = ColorHelper.RandomBodyColor();
		GeometryColor = Color;
		DefaultGUIColor = Color;
	}

	public void DOTSReset()
	{
		
		
		
		NewEntityId();
		Id = NewId();
		dirtyPhysicsProperties = PropertyFlags.None;
		defaultFlagsSet = false;
		IsDead = false;
		IsInitialized = false;
		int parentId = (SourceId = -1);
		ParentId = parentId;
		EngineOrientation = Quaternion.identity;
		Color = ColorHelper.RandomBodyColor();
		GeometryColor = Color;
	}

	public void ResetWith(PhysicsBody pBody)
	{
		
		
		
		NewEntityId();
		Id = NewId();
		dirtyPhysicsProperties = PropertyFlags.None;
		defaultFlagsSet = false;
		IsDead = false;
		IsInitialized = false;
		int parentId = (SourceId = -1);
		ParentId = parentId;
		CustomOrbitParent = null;
		EngineOrientation = Quaternion.identity;
		Color = ColorHelper.RandomBodyColor();
		GeometryColor = Color;
		Age = 0.0;
		EntityFlags = EntityFlags.None;
	}

	public void Reset()
	{
		ResetWith(new PhysicsBody(Vector3D.Zero, Vector3D.Zero, 1.0, 1.0, 1.0));
	}

	public EntityFlags CalculateDefaultFlags()
	{
		EntityFlags bodyFlags = this.bodyFlags | EntityFlags.Attracted | EntityFlags.Collider;
		if (LockPosition)
		{
			bodyFlags |= EntityFlags.LockPosition;
		}
		if (Celestial != null)
		{
			bodyFlags |= EntityFlags.Attracting;
			bodyFlags |= EntityFlags.Fragmenting;
			bodyFlags |= EntityFlags.Collider;
			bodyFlags |= EntityFlags.ColliderGroupMajor;
			bodyFlags |= EntityFlags.TidalEffects;
		}
		else if (Particle != null)
		{
			switch (Particle.Type)
			{
			case ParticleType.None:
			case ParticleType.Ice:
			case ParticleType.Material:
			case ParticleType.Collision:
			case ParticleType.Dust:
			case ParticleType.Fireworks:
				bodyFlags |= EntityFlags.MomentumLess;
				break;
			case ParticleType.Cloud:
				bodyFlags = bodyFlags.Remove(EntityFlags.Collider);
				break;
			case ParticleType.Star:
				bodyFlags |= EntityFlags.MomentumLess | EntityFlags.TidalEffects;
				break;
			default:
				bodyFlags |= EntityFlags.MomentumLess;
				break;
			}
		}
		return bodyFlags;
	}

	public void SetDefaultFlags(EntityFlags forceFlags = EntityFlags.None)
	{
		if (!defaultFlagsSet)
		{
			defaultFlagsSet = true;
			DisplayFlags |= DisplayFlags.Trail | DisplayFlags.Orbit;
			if ((int)forceFlags > 0)
			{
				EntityFlags = forceFlags;
			}
			else
			{
				EntityFlags = CalculateDefaultFlags();
			}
		}
	}

	public override int GetHashCode()
	{
		return base.GetHashCode();
	}

	public bool IsChildOf(Body body)
	{
		if (body.Children.Contains(this))
		{
			return true;
		}
		for (int i = 0; i < body.Children.Count; i++)
		{
			if (IsChildOf(body.Children[i]))
			{
				return true;
			}
		}
		return false;
	}

	public IEnumerator<Body> GetEnumerator()
	{
		return Children.GetEnumerator();
	}

	public void Suspend(bool suspendChildren = true)
	{
		
		if (Suspended)
		{
			return;
		}
		Suspended = true;
		if (suspendChildren)
		{
			for (int i = 0; i < Children.Count; i++)
			{
				Children[i].Suspend();
			}
		}
		UniTaskExtensions.Forget(HandleSuspendedChanged());
	}

	public void Resume()
	{
		
		if (Suspended)
		{
			Suspended = false;
			for (int i = 0; i < Children.Count; i++)
			{
				Children[i].Resume();
			}
			UniTaskExtensions.Forget(HandleSuspendedChanged());
		}
	}

	[AsyncStateMachine(typeof(<HandleSuspendedChanged>d__482))]
	private UniTask HandleSuspendedChanged()
	{
		
		
		
		<HandleSuspendedChanged>d__482 obj = default;
		obj.<>t__builder = AsyncUniTaskMethodBuilder.Create();
		obj.<>4__this = this;
		obj.<>1__state = -1;
		obj.<>t__builder.Start<<HandleSuspendedChanged>d__482>(ref obj);
		return obj.<>t__builder.Task;
	}

	public double GetRadius(bool uniform, bool forChart = true)
	{
		if (uniform)
		{
			if (SystemEngine != null)
			{
				return 6371010.0;
			}
			return Radius;
		}
		if (forChart && ChartRadius > 0.0)
		{
			return ChartRadius;
		}
		return Radius;
	}

	public List<Body> GetChildren()
	{
		return new List<Body>(Children);
	}

	public List<Body> GetChildren(bool recursive)
	{
		if (!recursive)
		{
			return GetChildren();
		}
		List<Body> list = new List<Body>();
		for (int i = 0; i < Children.Count; i++)
		{
			Body body = Children[i];
			list.Add(body);
			body.GetChildren(list, recursive: true);
		}
		return list;
	}

	public void GetChildren(List<Body> result)
	{
		result.AddRange(Children);
	}

	public void GetChildren(List<Body> result, bool recursive)
	{
		if (!recursive)
		{
			GetChildren(result);
			return;
		}
		for (int i = 0; i < Children.Count; i++)
		{
			Body body = Children[i];
			result.Add(body);
			body.GetChildren(result, recursive: true);
		}
	}

	public void PositionTo(Body other, double distance, double inclination, double trueAnomaly)
	{
		Vector3D vector3D = Vector3D.RotateAround(other?.Position ?? Vector3D.Zero, distance, (float)trueAnomaly, (float)inclination);
		Vector3D vector3D2 = vector3D - Position;
		Position = vector3D;
		if (!(Group is Galaxy galaxy))
		{
			return;
		}
		foreach (Body member in galaxy.Members)
		{
			member.Position += vector3D2;
		}
	}

	public void PositionTo(Body other, double distance)
	{
		double trueAnomaly = Elements.WhereInOrbit * MathS.Deg2Rad;
		double inclination = Elements.Inclination * (double)MathS.Deg2Rad;
		PositionTo(other, distance, inclination, trueAnomaly);
	}

	public void Restore(bool removeFromPool = true)
	{
		IsDead = false;
		IsInitialized = false;
		for (int i = 0; i < Components.Count; i++)
		{
			Components[i].IsFirstUpdate = true;
		}
		HologramSystem.SetHoloMode(this, HologramMode.FadeToSolid);
		if (isPooled & removeFromPool)
		{
			(SourcePool ?? Pool).Remove(this);
		}
	}

	public void Initialize()
	{
		if (!IsInitialized)
		{
			for (int i = 0; i < Components.Count; i++)
			{
				Components[i].Initialize();
			}
			SetDefaultFlags();
			if (savedFlags != EntityFlags.None)
			{
				EntityFlags = savedFlags;
			}
			InitializeDOTS();
			IsInitialized = true;
			Budget.Add(BudgetType);
			if (tempCustomReferenceBody != -1)
			{
				CustomReferenceBody = SystemEngine.GetBodyById(tempCustomReferenceBody);
				tempCustomReferenceBody = -1;
			}
		}
	}

	public void CreateEntity()
	{
		
		
		
		
		
		
		if (!(dotsEntity != Entity.Null))
		{
			EntityManager entityManager = World.DefaultGameObjectInjectionWorld.EntityManager;
			dotsEntity = entityManager.CreateEntity();
		}
	}

	private void InitializeDOTS()
	{
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		
		EntityManager entityManager = World.DefaultGameObjectInjectionWorld.EntityManager;
		if (dotsEntity == Entity.Null)
		{
			CreateEntity();
		}
		if (entityManager.HasComponent<Mass>(dotsEntity))
		{
			entityManager.SetComponentData<ColorSet>(dotsEntity, new ColorSet
			{
				Color = Color.op_Implicit(Color.op_Implicit(Color)),
				GUI = Color.op_Implicit(Color.op_Implicit(Color)),
				Geometry = Color.op_Implicit(Color.op_Implicit(Color)),
				CustomColor = Color.op_Implicit(Color.op_Implicit(CustomColor)),
				CustomGUIColor = Color.op_Implicit(Color.op_Implicit(CustomGUIColor)),
				GUIMode = GUIColorMode,
				GeometryMode = ColorMode
			});
			return;
		}
		if (Selectable != null)
		{
			entityManager.AddComponentData<Selectable>(dotsEntity, default(Selectable));
		}
		entityManager.AddComponentData<Mass>(dotsEntity, new Mass
		{
			value = PhysicsMass
		});
		entityManager.AddComponentData<Radius>(dotsEntity, new Radius
		{
			value = Radius
		});
		entityManager.AddComponentData<Age>(dotsEntity, new Age
		{
			value = Age
		});
		if (GravityRadius != 0.0)
		{
			entityManager.AddComponentData<GravityRadius>(dotsEntity, new GravityRadius
			{
				value = GravityRadius
			});
		}
		if (Density == 0.0)
		{
			Density = Utilities.Density(Mass, Radius);
		}
		entityManager.AddComponentData<Density>(dotsEntity, new Density
		{
			value = Density
		});
		entityManager.AddComponentData<Position>(dotsEntity, new Position
		{
			value = Position
		});
		entityManager.AddComponentData<Rotation>(dotsEntity, new Rotation
		{
			value = Quaternion.op_Implicit(EngineOrientation)
		});
		entityManager.AddComponentData<Velocity>(dotsEntity, new Velocity
		{
			value = Velocity
		});
		entityManager.AddComponentData<Albedo>(dotsEntity, new Albedo
		{
			Value = (float)(Heat?.Albedo ?? 0.0)
		});
		entityManager.AddComponentData<AngularVelocity>(dotsEntity, new AngularVelocity
		{
			value = AngularVelocity
		});
		entityManager.AddComponentData<AngularAcceleration>(dotsEntity, new AngularAcceleration
		{
			Precession = double3.zero,
			Forces = double3.zero
		});
		entityManager.AddComponentData<Acceleration>(dotsEntity, new Acceleration
		{
			value = double3.zero,
			previousValue = double3.zero
		});
		entityManager.AddComponentData<Force>(dotsEntity, new Force
		{
			value = double3.zero
		});
		entityManager.AddComponentData<Torque>(dotsEntity, new Torque
		{
			value = double3.zero
		});
		entityManager.AddComponentData<Composition>(dotsEntity, new Composition
		{
			Softness = 1.0,
			Opacity = 0.0
		});
		if (Composition != null && Composition.RadiusFollowsComposition)
		{
			entityManager.AddComponentData<SimulateRadius>(dotsEntity, default(SimulateRadius));
		}
		entityManager.AddComponentData<OrbitalInfo>(dotsEntity, new OrbitalInfo
		{
			orbitalParent = Entity.Null,
			strongestAttractor = Entity.Null,
			rocheLobeRadius = 0.0
		});
		entityManager.AddComponentData<Jerk>(dotsEntity, new Jerk
		{
			value = double3.zero
		});
		entityManager.AddComponentData<ColorSet>(dotsEntity, new ColorSet
		{
			Color = Color.op_Implicit(Color.op_Implicit(Color)),
			GUI = Color.op_Implicit(Color.op_Implicit(Color)),
			Geometry = Color.op_Implicit(Color.op_Implicit(Color))
		});
		if (Heat != null)
		{
			bool flag = Celestial != null && !Heat.TemperatureInitialized && !Heat.OverrideStartingTemp && (!Suspended || !Heat.deserialized);
			if (Heat.OverrideStartingTemp)
			{
				entityManager.AddComponentData<Temperature>(dotsEntity, new Temperature
				{
					Surface = (float)Heat.StartingTemperature,
					Effective = (float)Heat.EffectiveTemperature,
					Equilibrium = (float)Heat.EquilibriumTemperature,
					Authored = true
				});
			}
			else
			{
				entityManager.AddComponentData<Temperature>(dotsEntity, new Temperature
				{
					Surface = (float)Heat.SurfaceTemperature,
					Effective = (float)Heat.EffectiveTemperature,
					Equilibrium = (float)Heat.EquilibriumTemperature,
					NeedsInitialization = flag
				});
			}
			Heat.SyncSurfaceHeatCapacity();
			if (flag && !Suspended)
			{
				Heat.TemperatureInitialized = true;
			}
		}
		if (BodyFlagAttracted)
		{
			entityManager.AddComponent<Attracted>(dotsEntity);
		}
		if (BodyFlagCollider)
		{
			entityManager.AddComponent<CanCollide>(dotsEntity);
		}
		if (BodyFlagInvertedGravity)
		{
			entityManager.AddComponent<InvertedGravity>(dotsEntity);
		}
		if (BodyFlagMomentumless)
		{
			entityManager.AddComponent<Momentumless>(dotsEntity);
		}
		if (LockPosition)
		{
			entityManager.AddComponent<LockPosition>(dotsEntity);
		}
		entityManager.AddComponentData<BodyIndex>(dotsEntity, new BodyIndex
		{
			id = entityId
		});
		if (!Suspended)
		{
			entityManager.AddComponentData<NBodyMember>(dotsEntity, new NBodyMember
			{
				timestepScalar = TimeStepScalar
			});
		}
		entityManager.AddComponentData<ViewFlags>(dotsEntity, new ViewFlags
		{
			value = DisplayFlags
		});
		if (Celestial != null)
		{
			if (BodyFlagAttracting)
			{
				entityManager.AddComponentData<Attractor>(dotsEntity, new Attractor
				{
					useJ2 = J2Enabled,
					j2value = J2
				});
			}
			entityManager.AddComponentData<Constitution>(dotsEntity, new Constitution
			{
				SolidFraction = 1.0,
				Viscosity = Celestial.Viscosity,
				Cohesion = Celestial.Cohesion
			});
			entityManager.AddComponentData<MagneticField>(dotsEntity, new MagneticField
			{
				FluxDensity = Celestial.MagneticField
			});
			if (Composition.NeedsSetTuningFactor)
			{
				entityManager.AddComponentData<DetermineTuningFactor>(dotsEntity, default(DetermineTuningFactor));
				Composition.NeedsSetTuningFactor = false;
			}
			bool flag2 = !string.IsNullOrEmpty(Appearance.PrefabSource);
			bool flag3 = IsGalaxyBlackHole || DOTS.EntityUtil.IsWithinSchwarzschildRadius(Mass, Radius, (SystemEngine != null) ? SystemEngine.Gravity : 6.6740831E-11);
			if (!flag2 && Celestial.PresentType != CelestialType.BlackHole && !flag3)
			{
				entityManager.AddComponentData<TidalEffects>(dotsEntity, new TidalEffects
				{
					Heating = 0.0,
					RocheLobeRadius = 0.0,
					Stress = double3x3.op_Implicit(float3x3.zero)
				});
				entityManager.AddComponentData<DeformationState>(dotsEntity, new DeformationState
				{
					Deformation = Celestial.Deformation,
					DeformationRate = double3x3.op_Implicit(float3x3.zero),
					DeformationAcceleration = double3x3.op_Implicit(float3x3.zero),
					Eigenvectors = float3x3.zero
				});
				if (BodyFlagFragmenting)
				{
					entityManager.AddComponent<CanFragment>(dotsEntity);
				}
				if (LockDeformation)
				{
					SetTag<LockDeformation>(dotsEntity, value: true);
				}
				else if (Celestial.NeedsStabilization)
				{
					entityManager.AddComponent<StabilizeDeformation>(dotsEntity);
					Celestial.NeedsStabilization = false;
				}
				entityManager.AddComponentData<Atmosphere>(dotsEntity, new Atmosphere
				{
					Mass = Celestial.AtmosphereMass,
					Density = Celestial.AtmosphereDensity,
					ScaleHeight = Celestial.ScaleHeight
				});
				entityManager.AddComponentData<Polytrope>(dotsEntity, default(Polytrope));
				entityManager.AddComponentData<Textures>(dotsEntity, default(Textures));
			}
			CelestialType celestialType = DOTS.EntityUtil.DetermineCelestialType(Mass, Radius, 0.0, (SystemEngine != null) ? SystemEngine.Gravity : 6.6740831E-11);
			bool flag4 = celestialType == CelestialType.TerrestrialPlanet || celestialType == CelestialType.Planetoid || celestialType == CelestialType.GasGiant;
			if ((!flag2 && SurfaceGrid != null) & flag4)
			{
				entityManager.AddComponentData<Surface>(dotsEntity, new Surface
				{
					AtlasIndex = -1,
					ElevationRange = SurfaceGrid.SafeElevationRange,
					ElevationToRadiusRatio = SurfaceGrid.ElevationToRadiusRatio,
					LastRadius = Radius
				});
				entityManager.AddComponentData<GeostrophicWind>(dotsEntity, new GeostrophicWind
				{
					Speed = (float)Composition.GeostrophicWindSpeed
				});
				entityManager.SetComponentEnabled<GeostrophicWind>(dotsEntity, Composition.GeostrophicWindSpeedMode == SimulatedMode.Custom);
				if (SurfaceGrid.InstantPhaseToggle != PhaseEnum.None)
				{
					entityManager.AddComponentData<InstantPhaseChange>(dotsEntity, new InstantPhaseChange
					{
						Phase = SurfaceGrid.InstantPhaseToggle,
						MapIndex = (int)SurfaceGrid.InstantPhaseMapIndex
					});
				}
				if (SurfaceGrid.SerializedIndex > -1)
				{
					entityManager.AddComponentData<RestoreAtlasIndex>(dotsEntity, new RestoreAtlasIndex
					{
						Value = SurfaceGrid.SerializedIndex
					});
					SurfaceGrid.SerializedIndex = -1;
				}
				if (SurfaceGrid.OverrideStartingSeaLevel && SurfaceGrid.StartingSeaLevelMaterialID != 0 && (SurfaceGrid.StartingSeaLevel >= 0f || SurfaceGrid.StartingOceanVolume > 0.0))
				{
					entityManager.AddComponentData<PendingSeaLevel>(dotsEntity, new PendingSeaLevel
					{
						Level = SurfaceGrid.StartingSeaLevel,
						MaterialID = SurfaceGrid.StartingSeaLevelMaterialID,
						TargetVolume = SurfaceGrid.StartingOceanVolume
					});
				}
			}
			entityManager.AddComponent<MajorCollider>(dotsEntity);
			if (flag3)
			{
				entityManager.AddComponent<DOTS.BlackHole>(dotsEntity);
			}
			if (IsGalaxyBlackHole)
			{
				entityManager.AddComponent<GalaxyBlackHole>(dotsEntity);
			}
			if (IsStar)
			{
				entityManager.AddComponentData<Luminosity>(dotsEntity, new Luminosity
				{
					Value = Celestial.Luminosity
				});
				entityManager.AddComponent<RadiationSource>(dotsEntity);
				entityManager.AddComponentData<StellarState>(dotsEntity, new StellarState
				{
					Realistic = Celestial.Realistic,
					Type = Celestial.StarType,
					Initialized = true
				});
				LuminosityTarget luminosityTarget = default;
				bool flag5 = Celestial.userChangedLuminosity && Celestial.luminosityLerp != null && Celestial.luminosityLerp.Active;
				if (flag5)
				{
					luminosityTarget.Start = Celestial.luminosityLerp.Value;
					luminosityTarget.Goal = Celestial.luminosityLerp.Goal;
				}
				entityManager.AddComponentData<LuminosityTarget>(dotsEntity, luminosityTarget);
				entityManager.SetComponentEnabled<LuminosityTarget>(dotsEntity, flag5);
			}
			EntityTypeFactory.AddTrail(dotsEntity, entityManager);
			if (TimeStepScalar <= 1.0)
			{
				entityManager.AddComponentData<Snap>(dotsEntity, new Snap
				{
					value = double3.zero
				});
				entityManager.AddComponentData<Crackle>(dotsEntity, new Crackle
				{
					value = double3.zero
				});
				entityManager.AddComponentData<Hermite8thOrder>(dotsEntity, default(Hermite8thOrder));
			}
			else if (TimeStepScalar > 1.0)
			{
				entityManager.AddComponentData<Snap>(dotsEntity, new Snap
				{
					value = double3.zero
				});
				entityManager.AddComponentData<Hermite6thOrder>(dotsEntity, default(Hermite6thOrder));
			}
			entityManager.AddBuffer<ProjectedState>(dotsEntity);
		}
		else
		{
			entityManager.AddComponentData<Expansivity>(dotsEntity, new Expansivity
			{
				Value = 1f,
				MaxSpeed = float.MaxValue,
				GasTemperature = (float)(Heat?.InitialGasTemperature ?? 0.0)
			});
			if (Trail != null)
			{
				EntityTypeFactory.AddTrail(dotsEntity, entityManager);
			}
			entityManager.AddComponentData<Hermite4thOrder>(dotsEntity, default(Hermite4thOrder));
			if (Particle != null && Particle.Type != ParticleType.None)
			{
				entityManager.AddComponentData<LockDensity>(dotsEntity, default(LockDensity));
			}
		}
		if (Barycenter != null)
		{
			DynamicBuffer<BarycenterEntity> val = entityManager.AddBuffer<BarycenterEntity>(dotsEntity);
			foreach (Body body in Barycenter.Bodies)
			{
				val.Add(new BarycenterEntity
				{
					entity = body.dotsEntity
				});
			}
			PropertyFlags = PropertyFlags.None;
		}
		if (Lagrange != null)
		{
			entityManager.AddComponentData<Lagrange>(dotsEntity, new Lagrange
			{
				primary = Lagrange.Primary.dotsEntity,
				secondary = Lagrange.Secondary.dotsEntity,
				type = Lagrange.PointType
			});
			PropertyFlags = PropertyFlags.None;
		}
		entityManager.AddBuffer<SubstepState>(dotsEntity);
		if (Composition != null)
		{
			DynamicBuffer<MaterialBucket> val2 = entityManager.AddBuffer<MaterialBucket>(dotsEntity);
			foreach (KeyValuePair<MaterialData, MaterialDepot> depot in Composition.Depots)
			{
				MaterialDepot value = depot.Value;
				val2.Add(new MaterialBucket
				{
					MaterialID = depot.Key.Hashcode,
					Mass = value.Mass,
					IsLocked = value.LockSurfaceTracking,
					Phases = new float4(value.SurfaceSolidFraction, value.SurfaceLiquidFraction, value.SurfaceGasFraction, 0f),
					InteriorPhases = new float4(value.InteriorSolidFraction, value.InteriorLiquidFraction, value.InteriorGasFraction, 0f)
				});
			}
		}
		else if (Particle != null)
		{
			DynamicBuffer<MaterialBucket> val3 = entityManager.AddBuffer<MaterialBucket>(dotsEntity);
			foreach (KeyValuePair<MaterialData, double> material in Particle.Materials)
			{
				val3.Add(new MaterialBucket
				{
					MaterialID = material.Key.Hashcode,
					Mass = material.Value
				});
			}
		}
		if (Composition != null || Particle != null)
		{
			MassRadiusDensityTarget massRadiusDensityTarget = default;
			bool flag6 = false;
			if (Composition != null)
			{
				if (Composition.Mass.Active)
				{
					massRadiusDensityTarget.MassStart = Composition.Mass.Value;
					massRadiusDensityTarget.MassGoal = Composition.Mass.Goal;
					flag6 = (massRadiusDensityTarget.MassActive = true);
				}
				if (Composition.Radius.Active)
				{
					massRadiusDensityTarget.RadiusStart = Radius;
					massRadiusDensityTarget.RadiusGoal = Composition.Radius.Goal;
					flag6 = (massRadiusDensityTarget.RadiusActive = true);
				}
				if (Composition.Density.Active)
				{
					massRadiusDensityTarget.DensityGoal = Composition.Density.Goal;
				}
			}
			entityManager.AddComponentData<MassRadiusDensityTarget>(dotsEntity, massRadiusDensityTarget);
			entityManager.SetComponentEnabled<MassRadiusDensityTarget>(dotsEntity, flag6);
		}
		if (WasDeserialized)
		{
			entityManager.AddComponent<Deserialized>(dotsEntity);
		}
		FixedString64Bytes val4 = default;
		FixedStringMethods.CopyFromTruncated<FixedString64Bytes>(ref val4, Language.GetDev(Name));
		entityManager.SetName(dotsEntity, val4);
	}

	public void AutoOrbit()
	{
		if (SystemEngine != null)
		{
			FindStrongestAttractor();
			FindOrbitVelocity();
		}
	}

	public void Update()
	{
		Initialize();
	}

	public void OnLevelOfDetailChanged(LoDLevel prevLevelOfDetail, LoDLevel newLevelOfDetail)
	{
		for (int i = 0; i < Components.Count; i++)
		{
			if (Components[i] is ILoDChangedHandler loDChangedHandler)
			{
				loDChangedHandler.OnLevelOfDetailChanged(prevLevelOfDetail, newLevelOfDetail);
			}
		}
	}

	public void SetRenderMode(BodyRenderMode mode)
	{
		for (int i = 0; i < Components.Count; i++)
		{
			if (Components[i] is ISetRenderMode setRenderMode)
			{
				setRenderMode.SetRenderMode(mode);
			}
		}
		if ((Object)(object)gameObject != (Object)null)
		{
			gameObject.BroadcastMessage("SetRenderMode", (object)mode, (SendMessageOptions)1);
		}
	}

	protected override void OnDestroy()
	{
		
		
		
		
		
		
		
		if (IsInitialized)
		{
			Budget.Subtract(BudgetType);
			IsDead = true;
			IsInitialized = false;
			SystemEngine = null;
			if (isPooled)
			{
				(SourcePool ?? Pool).Release(this);
			}
			if (World.DefaultGameObjectInjectionWorld != null && dotsEntity != Entity.Null)
			{
				EntityManager entityManager = World.DefaultGameObjectInjectionWorld.EntityManager;
				entityManager.AddComponentData<PendingDestruction>(dotsEntity, new PendingDestruction
				{
					KillReason = KillReason
				});
				dotsEntity = Entity.Null;
			}
		}
	}

	public void DestroyAll()
	{
		for (int i = 0; i < Children.Count; i++)
		{
			Children[i].DestroyAll();
		}
		Destroy();
	}

	public void FindStrongestAttractor()
	{
		if (SystemEngine != null)
		{
			FindStrongestAttractor(SystemEngine.Bodies);
		}
	}

	public void SetAttractor(IList<Body> list)
	{
		if (SystemEngine == null)
		{
			return;
		}
		double num = double.MinValue;
		Attractor = null;
		int count = list.Count;
		for (int i = 0; i < count; i++)
		{
			Body body = list[i];
			if (body == this || body.Particle != null || body.DummyCamera != null || (body.Suspended && !body.IsMathematicalPoint) || (body.Mass <= 0.0 && !body.IsMathematicalPoint) || (body.Barycenter != null && body.Barycenter.IsPartOf(this)) || body.PhysicsMass < PhysicsMass)
			{
				continue;
			}
			double num2 = Vector3D.Distance(Position, body.Position);
			if (num2 != 0.0)
			{
				if (num2 == double.NaN || double.IsInfinity(num2))
				{
					Debug.Log((object)("invalid dist " + num2 + " - Body.FindStrongestAttractor"));
				}
				double num3 = SystemEngine.Gravity * body.PhysicsMass / (num2 * num2 * num2);
				if (num3 > num)
				{
					Attractor = body;
					num = num3;
				}
			}
		}
		if (Attractor == this)
		{
			Attractor = null;
		}
	}

	public void SetOrbitParent()
	{
		List<Body> list = new List<Body>(SystemEngine.Bodies);
		list.Sort((Body a, Body b) => b.PhysicsMass.CompareTo(a.PhysicsMass));
		SetOrbitParent(list);
	}

	public void SetOrbitParent(IList<Body> list)
	{
		if (SystemEngine == null)
		{
			return;
		}
		double num = list[0].PhysicsMass;
		double num2 = double.MaxValue;
		_orbitParent = null;
		int count = list.Count;
		for (int i = 0; i < count; i++)
		{
			Body body = list[i];
			if (body == this || body.Particle != null || body.DummyCamera != null || (body.Suspended && !body.IsMathematicalPoint) || (body.Mass <= 0.0 && !body.IsMathematicalPoint) || (body.Barycenter != null && body.Barycenter.IsPartOf(this)) || body.PhysicsMass < PhysicsMass)
			{
				continue;
			}
			Vector3D vector3D = Mathematics.Equations.Physics.Gravity.BaryCenter(Position, body.Position, PhysicsMass, body.PhysicsMass);
			if ((Position - vector3D).Length > Radius * 5.0 || GalaxyMember != null)
			{
				double num3 = Mathematics.Equations.Physics.Gravity.Orbit(WorldMgr.ActiveSimulation.Gravity, body.Velocity, Velocity, body.Position, Position, body.PhysicsMass, PhysicsMass);
				if (num3 < num2 && num3 > 0.0)
				{
					num2 = num3;
					_orbitParent = body;
				}
			}
		}
		if (GalaxyMember != null && Parent != null)
		{
			_orbitParent = Parent;
		}
		if (PhysicsMass >= num)
		{
			_orbitParent = null;
			Attractor = null;
		}
		if (_orbitParent == this)
		{
			_orbitParent = null;
		}
	}

	public void FindStrongestAttractor(IList<Body> list)
	{
		if (SystemEngine == null)
		{
			return;
		}
		double num = double.MinValue;
		Attractor = null;
		int count = list.Count;
		for (int i = 0; i < count; i++)
		{
			Body body = list[i];
			if (body == this || body.Particle != null || body.DummyCamera != null || (body.Suspended && !body.IsMathematicalPoint) || (body.Mass <= 0.0 && !body.IsMathematicalPoint) || (body.Barycenter != null && body.Barycenter.IsPartOf(this)))
			{
				continue;
			}
			double num2 = Vector3D.Distance(Position, body.Position);
			if (num2 != 0.0)
			{
				if (num2 == double.NaN || double.IsInfinity(num2))
				{
					Debug.Log((object)("invalid dist " + num2 + " - Body.FindStrongestAttractor"));
				}
				double num3 = SystemEngine.Gravity * body.PhysicsMass / (num2 * num2 * num2);
				if (num3 > num)
				{
					Attractor = body;
					num = num3;
				}
			}
		}
		if (Attractor == this)
		{
			Attractor = null;
		}
	}

	public Body FindClosestBody(double minDistance = double.MinValue, bool ignoreParticles = false, bool ignoreGhosts = true)
	{
		if (WorldMgr.ActiveSimulation == null)
		{
			return null;
		}
		double num = double.MaxValue;
		Body result = null;
		for (int i = 0; i < WorldMgr.ActiveSimulation.Bodies.Count; i++)
		{
			Body body = WorldMgr.ActiveSimulation.Bodies[i];
			if (body != this && body.DummyCamera == null && !body.Suspended && !(body.Mass <= 0.0) && !body.IsDead && (!ignoreParticles || body.Particle == null) && (!ignoreGhosts || !body.IsGhost) && body.DummyCamera == null)
			{
				double num2 = Utilities.DistanceBetweenBodySurfaces(this, body);
				if (num2 < num && num2 > minDistance)
				{
					num = num2;
					result = body;
				}
			}
		}
		return result;
	}

	public void FindBinaryOrbitVelocity(bool balanceMotion, double eccentricity = 0.0, bool applyAttractorMotion = true)
	{
		if (Attractor == null)
		{
			return;
		}
		Vector3D orbitNormal = new Vector3D(0.0, 1.0, 0.0);
		double parentMass = Mass + Attractor.Mass;
		Vector3D vector3D = Attractor.Velocity;
		Velocity = Utilities.CalcOrbit3DVelocity(Position, Attractor.Position, orbitNormal, vector3D, parentMass, SystemEngine.Gravity, eccentricity);
		Vector3D vector3D2 = Utilities.CalcOrbit3DVelocity(Attractor.Position, Position, orbitNormal, Velocity, parentMass, SystemEngine.Gravity, eccentricity);
		Velocity -= vector3D2 - vector3D;
		if (balanceMotion)
		{
			Vector3D vector3D3 = Utilities.GroupVelocity(new List<Body> { this, Attractor });
			Velocity -= vector3D3;
			if (applyAttractorMotion)
			{
				Attractor.Velocity -= vector3D3;
			}
		}
	}

	public void FindOrbitVelocity()
	{
		if (SystemEngine != null)
		{
			if (Attractor != null && Parent == null)
			{
				Velocity = Utilities.CalcOrbit3DVelocity(Position, Attractor.Position, Attractor.Velocity, Attractor.PhysicsMass, SystemEngine.Gravity);
			}
			else if (Parent != null)
			{
				FindOrbitVelocity(Parent);
			}
		}
	}

	public void FindOrbitVelocity(bool retrograde, double eccentricity)
	{
		double retrograde2 = (retrograde ? 1.0 : (-1.0));
		if (Attractor != null && Parent == null)
		{
			Velocity = Utilities.CalcOrbit3DVelocity(Position, Attractor.Position, Vector3D.Up, Attractor.Velocity, Attractor.PhysicsMass, SystemEngine.Gravity, eccentricity, retrograde2);
		}
		else if (Parent != null)
		{
			Velocity = Utilities.CalcOrbit3DVelocity(Position, Parent.Position, Vector3D.Up, Parent.Velocity, Parent.PhysicsMass, SystemEngine.Gravity, eccentricity, retrograde2);
		}
	}

	public void FindOrbitVelocity(Body other)
	{
		Velocity = Utilities.CalcOrbit3DVelocity(Position, other.Position, Vector3D.Up, other.Velocity, other.PhysicsMass, SystemEngine.Gravity, 0.0);
	}

	public void StartRotateAround(Vector3D center)
	{
		
		
		
		
		if (SelectWholeGroup())
		{
			Group.StartRotateAround(center);
			return;
		}
		Vector3D vector3D = Position - center;
		length = vector3D.Length;
		offset = vector3D.UnitDirection.ToVector3();
		velocityForGizmo = Velocity;
		rotStart = EngineOrientation;
		for (int i = 0; i < Children.Count; i++)
		{
			Children[i].StartRotateAround(center);
		}
	}

	public void SetRotateAround(Quaternion rotation)
	{
		
		
		
		
		if (SelectWholeGroup())
		{
			Group.SetRotateAround(rotation);
			return;
		}
		rotEnd = rotation;
		for (int i = 0; i < Children.Count; i++)
		{
			Children[i].SetRotateAround(rotation);
		}
	}

	public void UpdateRotateAround(Vector3D center)
	{
		
		
		
		
		
		
		
		
		
		
		
		if (SelectWholeGroup())
		{
			Group.UpdateRotateAround(center);
			return;
		}
		rotCurrent = Quaternion.Lerp(rotCurrent, rotEnd, Mathf.Min(Time.smoothDeltaTime * 4f, 1f));
		Position = center + (rotCurrent * offset).ToVector3D() * length;
		Velocity = velocityForGizmo.Multiply(rotCurrent);
		EngineOrientation = rotCurrent * rotStart;
		for (int i = 0; i < Children.Count; i++)
		{
			Children[i].UpdateRotateAround(center);
		}
	}

	public void EndRotateAround()
	{
		
		
		
		
		if (SelectWholeGroup())
		{
			Group.EndRotateAround();
			return;
		}
		rotCurrent = Quaternion.identity;
		rotEnd = Quaternion.identity;
		for (int i = 0; i < Children.Count; i++)
		{
			Children[i].EndRotateAround();
		}
	}

	public void StartMove(Vector3D position)
	{
		if (isMoving)
		{
			return;
		}
		if (SelectWholeGroup())
		{
			Group.StartMove(position);
			return;
		}
		wasPositionLocked = LockPosition;
		LockPosition = true;
		isMoving = true;
		moveOffset = Position - position;
		for (int i = 0; i < Children.Count; i++)
		{
			Children[i].StartMove(position);
		}
	}

	public void UpdateMove(Vector3D position)
	{
		if (SelectWholeGroup())
		{
			Group.UpdateMove(position);
			return;
		}
		Vector3D position2 = (VisualPos = position + moveOffset);
		Position = position2;
		for (int i = 0; i < Children.Count; i++)
		{
			Children[i].UpdateMove(position);
		}
	}

	public void EndMove()
	{
		if (SelectWholeGroup())
		{
			Group.EndMove();
			return;
		}
		LockPosition = wasPositionLocked;
		isMoving = false;
		for (int i = 0; i < Children.Count; i++)
		{
			Children[i].EndMove();
		}
	}

	public override string ToString()
	{
		return Name;
	}

	protected override void OnComponentAdded(BaseComponent component)
	{
		if (component is BodyComponent)
		{
			(component as BodyComponent).Body = this;
		}
	}

	public override void FromJson(JSON json, int version)
	{
		
		
		
		
		
		
		
		
		
		
		
		
		
		BuildCommit = version;
		Name = json.ToString("Name");
		Id = json.ToInt("Id");
		if (json.Contains("HorizonID"))
		{
			HorizonID = json.ToString("HorizonID");
		}
		GroupId = json.ToInt("Group");
		IsGalaxyBlackHole = Group != null && (Group as Galaxy)?.BlackHoleId == Id;
		Age = json.ToDouble("Age");
		Color val = (GeometryColor = Conv.StringToColor(json.ToString("Color")));
		Color = val;
		PhysicsMass = json.ToDouble("PhysicsMass");
		Mass = json.ToDouble("Mass");
		Radius = json.ToDouble("Radius");
		GravityRadius = (json.Contains("GravityRadius") ? json.ToDouble("GravityRadius") : 0.0);
		Density = json.ToDouble("Density");
		Origin = (BodyOrigin)json.ToInt("Origin");
		Generation = json.ToInt("Generation");
		EntityFlags = (EntityFlags)json.ToInt("Flags");
		DisplayFlags = (DisplayFlags)json.ToInt("DisplayFlags");
		EngineOrientation = Conv.StringToQuaternion(json.ToString("Orientation"));
		AngularVelocity = Conv.StringToVector3D(json.ToString("AngularVelocity"));
		Position = Conv.StringToVector3D(json.ToString("Position"));
		Velocity = Conv.StringToVector3D(json.ToString("Velocity"));
		BudgetType = (BudgetContext)json.ToInt("BudgetType");
		if (json.Contains("RotationAxis"))
		{
			_rotationAxis = Conv.StringToVector3D(json.ToString("RotationAxis"));
		}
		Suspended = json.ToBoolean("Suspended");
		LockRotation = json.ToBoolean("LockRotation");
		LockPosition = json.ToBoolean("LockPosition");
		LockDeformation = json.ToBoolean("LockDeformation");
		if (json.Contains("Category"))
		{
			Category = json.ToString("Category");
		}
		if (json.Contains("Parent"))
		{
			ParentId = json.ToInt("Parent");
		}
		if (json.Contains("Source"))
		{
			SourceId = json.ToInt("Source");
		}
		if (json.Contains("CustomOrbitParentId"))
		{
			CustomOrbitParentId = json.ToInt("CustomOrbitParentId");
		}
		if (json.Contains("LockedProperties"))
		{
			JSON[] array = json.ToArray<JSON>("LockedProperties");
			lockedProperties.Clear();
			JSON[] array2 = array;
			foreach (JSON jSON in array2)
			{
				lockedProperties.Add(jSON.ToString("$v"));
			}
		}
		if (json.Contains("UserChangedColor"))
		{
			UserChangedColor = json.ToBoolean("UserChangedColor");
			ColorMode = (ColorMode)json.ToInt("ColorMode");
			CustomColor = Conv.StringToColor(json.ToString("CustomColor"));
		}
		if (json.Contains("ColorPalette"))
		{
			ColorPalette = (ColorPalette)json.ToInt("ColorPalette");
		}
		DefaultGUIColor = Color;
		if (json.Contains("DefaultGUIColor"))
		{
			DefaultGUIColor = Conv.StringToColor(json.ToString("DefaultGUIColor"));
		}
		if (json.Contains("UserChangedGUIColor"))
		{
			UserChangedGUIColor = json.ToBoolean("UserChangedGUIColor");
			GUIColorMode = (ColorMode)json.ToInt("GUIColorMode");
			CustomGUIColor = Conv.StringToColor(json.ToString("CustomGUIColor"));
		}
		if (json.Contains("NonSphericalGravityEnabled"))
		{
			J2Enabled = json.ToBoolean("NonSphericalGravityEnabled");
			J2 = json.ToDouble("J2");
		}
		InMajorCollision = json.ToBoolean("InMajorCollision");
		if (json.Contains("DatabaseID"))
		{
			DatabaseID = new Guid(json.ToString("DatabaseID"));
		}
		if (json.Contains("Description"))
		{
			Description = json.ToString("Description");
		}
		base.FromJson(json, version);
		if (json.Contains("RelativeTo"))
		{
			RelativeTo = (ReferenceBodyType)json.ToInt("RelativeTo");
			if (json.Contains("CustomReferenceBody") && RelativeTo == ReferenceBodyType.Custom)
			{
				tempCustomReferenceBody = json.ToInt("CustomReferenceBody");
			}
		}
		MapComponents();
		Group?.AddMember(this);
		if (version < 21240)
		{
			EntityFlags |= EntityFlags.Attracted;
		}
		defaultFlagsSet = true;
	}

	public override JSON ToJson()
	{
		
		
		
		
		
		JSON jSON = base.ToJson();
		jSON["Id"] = Id;
		jSON["HorizonID"] = HorizonID;
		jSON["Age"] = Age;
		jSON["Color"] = Color;
		jSON["DefaultGUIColor"] = DefaultGUIColor;
		jSON["CustomColor"] = CustomColor;
		jSON["CustomGUIColor"] = CustomGUIColor;
		jSON["UserChangedColor"] = UserChangedColor;
		jSON["UserChangedGUIColor"] = UserChangedGUIColor;
		jSON["ColorPalette"] = (int)ColorPalette;
		jSON["PhysicsMass"] = PhysicsMass;
		jSON["Mass"] = Mass;
		jSON["Radius"] = Radius;
		jSON["GravityRadius"] = GravityRadius;
		jSON["Density"] = Density;
		jSON["Generation"] = Generation;
		jSON["Flags"] = (int)bodyFlags;
		jSON["DisplayFlags"] = (int)DisplayFlags;
		jSON["Orientation"] = EngineOrientation.ToJson();
		jSON["AngularVelocity"] = AngularVelocity.ToJson();
		jSON["RotationAxis"] = RotationAxis.ToJson();
		jSON["Position"] = Position.ToJson();
		jSON["Velocity"] = Velocity.ToJson();
		jSON["Suspended"] = Suspended;
		jSON["LockPosition"] = LockPosition;
		jSON["LockRotation"] = LockRotation;
		jSON["LockDeformation"] = LockDeformation;
		jSON["ColorMode"] = (int)ColorMode;
		jSON["GUIColorMode"] = (int)GUIColorMode;
		jSON["Parent"] = ParentId;
		jSON["Source"] = SourceId;
		jSON["Group"] = GroupId;
		jSON["CustomOrbitParentId"] = CustomOrbitParentId;
		jSON["LockedProperties"] = lockedProperties.ToJson();
		jSON["Origin"] = (int)Origin;
		jSON["Category"] = Category;
		jSON["BudgetType"] = (int)BudgetType;
		jSON["InMajorCollision"] = InMajorCollision;
		jSON["NonSphericalGravityEnabled"] = J2Enabled;
		jSON["J2"] = J2;
		jSON["DatabaseID"] = DatabaseID.ToString();
		jSON["Description"] = Description;
		jSON["RelativeTo"] = (int)RelativeTo;
		if (RelativeTo == ReferenceBodyType.Custom && CustomReferenceBody != null)
		{
			jSON["CustomReferenceBody"] = CustomReferenceBody.Id;
		}
		return jSON;
	}

	public bool IsLocked(string property)
	{
		return LockedProperties.Contains(property);
	}

	public void Lock(string property)
	{
		if (!LockedProperties.Contains(property))
		{
			LockedProperties.Add(property);
		}
	}

	public void Unlock(string property)
	{
		if (LockedProperties.Contains(property))
		{
			LockedProperties.Remove(property);
		}
	}

	public bool SelectWholeGroup()
	{
		if (Group != null)
		{
			return Group.SelectWholeGroup;
		}
		return false;
	}

	public bool GroupIsSelected()
	{
		if (Group != null)
		{
			return Group.Selected;
		}
		return false;
	}

	public void RemoveFromGroup()
	{
		if (Group != null)
		{
			_group = null;
			_groupId = -1;
		}
	}

	[AsyncStateMachine(typeof(<OnDropped>d__553))]
	public static UniTask<string> OnDropped(UboxManifest.Entry entryPoint, string targetFolder, string filename)
	{
		
		
		
		<OnDropped>d__553 obj = default;
		obj.<>t__builder = AsyncUniTaskMethodBuilder<string>.Create();
		obj.entryPoint = entryPoint;
		obj.targetFolder = targetFolder;
		obj.filename = filename;
		obj.<>1__state = -1;
		obj.<>t__builder.Start<<OnDropped>d__553>(ref obj);
		return obj.<>t__builder.Task;
	}
}
