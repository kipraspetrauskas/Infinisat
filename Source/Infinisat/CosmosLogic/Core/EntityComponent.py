using System.ComponentModel;
using CosmosLogic.ECS;
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
			
			return World.DefaultGameObjectInjectionWorld.EntityManager;
		}
	}

	public bool HasDOTS<T>() where T : unmanaged, IComponentData
	{
		
		
		
		
		
		if (Body.dotsEntity != Entity.Null)
		{
			EntityManager eM = EM;
			return eM.HasComponent<T>(Body.dotsEntity);
		}
		return false;
	}

	public T GetDOTS<T>() where T : unmanaged, IComponentData
	{
		
		
		
		EntityManager eM = EM;
		return eM.GetComponentData<T>(Body.dotsEntity);
	}

	protected void SetDOTS<T>(T value) where T : unmanaged, IComponentData
	{
		
		
		
		EntityManager eM = EM;
		eM.SetComponentData<T>(Body.dotsEntity, value);
	}

	public bool GetDOTSEnabled<T>() where T : unmanaged, IComponentData, IEnableableComponent
	{
		
		
		
		EntityManager eM = EM;
		return eM.IsComponentEnabled<T>(Body.dotsEntity);
	}

	protected void SetDOTSEnabled<T>(bool enabled) where T : unmanaged, IComponentData, IEnableableComponent
	{
		
		
		
		EntityManager eM = EM;
		eM.SetComponentEnabled<T>(Body.dotsEntity, enabled);
	}

	protected bool HasDOTSBuffer<T>() where T : unmanaged, IBufferElementData
	{
		
		
		
		
		
		if (Body.dotsEntity != Entity.Null)
		{
			EntityManager eM = EM;
			return eM.HasBuffer<T>(Body.dotsEntity);
		}
		return false;
	}

	protected DynamicBuffer<T> GetDOTSBuffer<T>(bool isReadOnly = false) where T : unmanaged, IBufferElementData
	{
		
		
		
		
		EntityManager eM = EM;
		return eM.GetBuffer<T>(Body.dotsEntity, isReadOnly);
	}

	protected void SetDOTSTag<T>(bool value) where T : unmanaged, IComponentData
	{
		
		
		
		
		
		
		
		
		
		
		
		
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
