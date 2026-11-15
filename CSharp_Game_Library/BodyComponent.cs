using System.ComponentModel;
using GiantArmy.ECS;
using CosmosLogic.Engine;

public abstract class BodyComponent : BaseComponent
{
	[Browsable(false)]
	public Body Body;

	[Browsable(false)]
	public Body Parent => Body.Parent;

	[Browsable(false)]
	public Body OrbitParent => Body.OrbitParent;

	private static EntityManager EM
	{
		get
		{
			//IL_0005: Unknown result type (might be due to invalid IL or missing references)
			return World.DefaultGameObjectInjectionWorld.EntityManager;
		}
	}

	public bool HasDOTS<T>() where T : unmanaged, IComponentData
	{
		//IL_0006: Unknown result type (might be due to invalid IL or missing references)
		//IL_000b: Unknown result type (might be due to invalid IL or missing references)
		//IL_0017: Unknown result type (might be due to invalid IL or missing references)
		//IL_001c: Unknown result type (might be due to invalid IL or missing references)
		//IL_0025: Unknown result type (might be due to invalid IL or missing references)
		if (Body.dotsEntity != Entity.Null)
		{
			EntityManager eM = EM;
			return eM.HasComponent<T>(Body.dotsEntity);
		}
		return false;
	}

	public T GetDOTS<T>() where T : unmanaged, IComponentData
	{
		//IL_0000: Unknown result type (might be due to invalid IL or missing references)
		//IL_0005: Unknown result type (might be due to invalid IL or missing references)
		//IL_000e: Unknown result type (might be due to invalid IL or missing references)
		EntityManager eM = EM;
		return eM.GetComponentData<T>(Body.dotsEntity);
	}

	protected void SetDOTS<T>(T value) where T : unmanaged, IComponentData
	{
		//IL_0000: Unknown result type (might be due to invalid IL or missing references)
		//IL_0005: Unknown result type (might be due to invalid IL or missing references)
		//IL_000e: Unknown result type (might be due to invalid IL or missing references)
		EntityManager eM = EM;
		eM.SetComponentData<T>(Body.dotsEntity, value);
	}

	public bool GetDOTSEnabled<T>() where T : unmanaged, IComponentData, IEnableableComponent
	{
		//IL_0000: Unknown result type (might be due to invalid IL or missing references)
		//IL_0005: Unknown result type (might be due to invalid IL or missing references)
		//IL_000e: Unknown result type (might be due to invalid IL or missing references)
		EntityManager eM = EM;
		return eM.IsComponentEnabled<T>(Body.dotsEntity);
	}

	protected void SetDOTSEnabled<T>(bool enabled) where T : unmanaged, IComponentData, IEnableableComponent
	{
		//IL_0000: Unknown result type (might be due to invalid IL or missing references)
		//IL_0005: Unknown result type (might be due to invalid IL or missing references)
		//IL_000e: Unknown result type (might be due to invalid IL or missing references)
		EntityManager eM = EM;
		eM.SetComponentEnabled<T>(Body.dotsEntity, enabled);
	}

	protected bool HasDOTSBuffer<T>() where T : unmanaged, IBufferElementData
	{
		//IL_0006: Unknown result type (might be due to invalid IL or missing references)
		//IL_000b: Unknown result type (might be due to invalid IL or missing references)
		//IL_0017: Unknown result type (might be due to invalid IL or missing references)
		//IL_001c: Unknown result type (might be due to invalid IL or missing references)
		//IL_0025: Unknown result type (might be due to invalid IL or missing references)
		if (Body.dotsEntity != Entity.Null)
		{
			EntityManager eM = EM;
			return eM.HasBuffer<T>(Body.dotsEntity);
		}
		return false;
	}

	protected DynamicBuffer<T> GetDOTSBuffer<T>(bool isReadOnly = false) where T : unmanaged, IBufferElementData
	{
		//IL_0000: Unknown result type (might be due to invalid IL or missing references)
		//IL_0005: Unknown result type (might be due to invalid IL or missing references)
		//IL_000e: Unknown result type (might be due to invalid IL or missing references)
		//IL_0014: Unknown result type (might be due to invalid IL or missing references)
		EntityManager eM = EM;
		return eM.GetBuffer<T>(Body.dotsEntity, isReadOnly);
	}

	protected void SetDOTSTag<T>(bool value) where T : unmanaged, IComponentData
	{
		//IL_0003: Unknown result type (might be due to invalid IL or missing references)
		//IL_0008: Unknown result type (might be due to invalid IL or missing references)
		//IL_0011: Unknown result type (might be due to invalid IL or missing references)
		//IL_003a: Unknown result type (might be due to invalid IL or missing references)
		//IL_003f: Unknown result type (might be due to invalid IL or missing references)
		//IL_0048: Unknown result type (might be due to invalid IL or missing references)
		//IL_001d: Unknown result type (might be due to invalid IL or missing references)
		//IL_0022: Unknown result type (might be due to invalid IL or missing references)
		//IL_002b: Unknown result type (might be due to invalid IL or missing references)
		//IL_0054: Unknown result type (might be due to invalid IL or missing references)
		//IL_0059: Unknown result type (might be due to invalid IL or missing references)
		//IL_0062: Unknown result type (might be due to invalid IL or missing references)
		EntityManager eM;
		if (value)
		{
			eM = EM;
			if (!eM.HasComponent<T>(Body.dotsEntity))
			{
				eM = EM;
				eM.AddComponent<T>(Body.dotsEntity);
				return;
			}
		}
		if (!value)
		{
			eM = EM;
			if (eM.HasComponent<T>(Body.dotsEntity))
			{
				eM = EM;
				eM.RemoveComponent<T>(Body.dotsEntity);
			}
		}
	}
}
