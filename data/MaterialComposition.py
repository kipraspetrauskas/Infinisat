using CosmosLogic.Engine;

namespace DOTS;

public struct Composition : IComponentData, IQueryTypeParameter
{
	public double Softness;

	public double Opacity;

	public double GasThickness;

	public double MomentOfInertia;

	public double TuningFactor;

	public double TotalMassLost;

	public double AblationMassLoss;

	public double ProfileSignature;

	public double DensitySignature;

	public bool DetailedProfile;
}
