using ICSharpCode.SharpZipLib.Zip;

public class LoadSimulationData
{
	public UboxManifest Manifest;

	public UboxManifest.Entry CurrentEntry;

	public ZipFile ZipFile;

	public int BuildRevision;

	public string Folder;
}
