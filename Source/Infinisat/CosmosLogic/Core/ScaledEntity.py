using SQLite;
using CosmosLogic.Core;

[Table("HumanScaleObject")]
public class HumanScaleObject : DatabaseObject, IBodyData
{
	[UIStringInfo("ba42fbd475694b6c87005bd56b48f21d", "")]
	public static UIString nameToken;

	[Column("AddBody")]
	public bool AddBody;

	[Column("DevOnly")]
	public bool DevOnly;

	[Column("Radius")]
	public double Radius;

	[Column("Mass")]
	public double Mass;

	[Column("PrefabString")]
	public string PrefabString = "";

	[Column("DiffuseTexture")]
	public string DiffuseTexture = "";

	[Column("Color")]
	public Color Color;

	[Column("HorizonID")]
	[Index]
	public string HorizonID = "";

	[Column("InGameDescription")]
	public string InGameDescription = "";

	[Column("WikipediaLink")]
	public string WikipediaLink = "";

	[Column("J2")]
	public double J2;

	[Column("CompositionString")]
	public string CompositionString;
}
