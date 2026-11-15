using System.ComponentModel;
using CosmosLogic.Core;

public class PrefabAppearance
{
	public AppearanceComponent Appearance;

	public Celestial Celestial;

	public Body Body;

	public CustomizedColor Color = new CustomizedColor(Color.white);

	[ValueRange(0.0, 1.0)]
	[Unit(UnitType.Percent)]
	[PropertyInfo("2d5c092d85d04bcc9235e8cdd04ee4b8", "")]
	[DefaultValue(0)]
	public float Metallic;

	[ValueRange(0.0, 1.0)]
	[Unit(UnitType.Percent)]
	[PropertyInfo("e4b138ff76ba460582c9b2b5b552db12", "")]
	[DefaultValue(0.5f)]
	public float Smoothness = 0.5f;

	public bool EnableAdditionalOptions;

	private bool _initialized;

	private bool wasDeserialized;

	private bool hasMetallic;

	private bool hasSmoothness;

	public void Initialize()
	{
		if (_initialized)
		{
			return;
		}
		_initialized = true;
		Body = Appearance.Body;
		Celestial = Appearance.Celestial;
		if (!((Object)(object)Appearance.Material != (Object)null))
		{
			return;
		}
		hasMetallic = Appearance.Material.HasFloat("_Metallic");
		hasSmoothness = Appearance.Material.HasFloat("_Smoothness");
		if (!wasDeserialized)
		{
			if (hasMetallic)
			{
				Metallic = Appearance.Material.GetFloat("_Metallic");
			}
			if (hasSmoothness)
			{
				Smoothness = Appearance.Material.GetFloat("_Smoothness");
			}
		}
	}

	public void Update()
	{
		
		
		
		
		
		if (!_initialized)
		{
			Initialize();
		}
		Color color = Color.Color;
		color.a = ((Color.ColorMode == ColorMode.Custom) ? 1f : 0f);
		Appearance.AddColor(MaterialProperties.CustomColor, color);
		Appearance.NominalAlbedo = Color.Color;
		if (hasMetallic)
		{
			Appearance.AddFloat(MaterialProperties.Metallic, Metallic);
		}
		if (hasSmoothness)
		{
			Appearance.AddFloat(MaterialProperties.Smoothness, Smoothness);
		}
		Appearance.AddBool(MaterialProperties.EnableAdditionalOptions, EnableAdditionalOptions);
	}

	public void FromJson(JSON json, int version)
	{
		
		
		
		
		
		
		Color.Color = Conv.StringToColor(json.ToString("Color"));
		Color.SetColors(Color.Color, Conv.StringToColor(json.ToString("customColor")));
		if (json.Contains("originalColor"))
		{
			Color.ColorMode = (ColorMode)json.ToInt("ColorMode");
			Color.SetColors(Conv.StringToColor(json.ToString("originalColor")), Conv.StringToColor(json.ToString("customColor")));
		}
		if (json.Contains("Metallic"))
		{
			Metallic = json.ToFloat("Metallic");
		}
		if (json.Contains("Smoothness"))
		{
			Smoothness = json.ToFloat("Smoothness");
		}
		wasDeserialized = true;
	}

	public JSON ToJson()
	{
		
		
		
		return new JSON
		{
			["Color"] = Color.Color,
			["ColorMode"] = (int)Color.ColorMode,
			["originalColor"] = Color.OriginalColor,
			["customColor"] = Color.CustomColor,
			["Metallic"] = Metallic,
			["Smoothness"] = Smoothness
		};
	}

	public PrefabAppearance()
	{
		
	}
}
