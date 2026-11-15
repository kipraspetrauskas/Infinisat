using System;
using SQLite;

public class DatabaseObject
{
	[Column("ID")]
	[PrimaryKey]
	public Guid ID;

	[Column("Name")]
	[Index]
	public string Name = "";

	[Column("Name1")]
	[Index]
	public string Name1 = "";

	[Column("Name2")]
	[Index]
	public string Name2 = "";

	[Column("Name3")]
	[Index]
	public string Name3 = "";

	[Column("Name4")]
	[Index]
	public string Name4 = "";

	public string OriginalName;
}
