#include "WfcTile.h"


WFC::Tiled3D::FacePoints FWfcTileFace::GetPrototypeCorner(WFC::Tiled3D::FacePoints thisCorner) const
{
    auto dir = WFC::Tiled3D::MakeCornerFaceVector(thisCorner);
    dir = dir.Transform(static_cast<WFC::Transformations>(PrototypeOrientation));
    return WFC::Tiled3D::MakeCornerFacePoint(dir);
}
WFC::Tiled3D::FacePoints FWfcTileFace::GetPrototypeEdge(WFC::Tiled3D::FacePoints thisEdge) const
{
	auto dir = WFC::Tiled3D::MakeEdgeFaceVector(thisEdge);
	dir = dir.Transform(static_cast<WFC::Transformations>(PrototypeOrientation));
	return WFC::Tiled3D::MakeEdgeFacePoint(dir);
}

WFC::Tiled3D::TransformSet FWfcTile::GetSupportedTransforms() const
{
	auto output = ImplicitPermutations.Unwrap().GetExplicit();
	for (auto p : PrecisePermutations)
		output.Add(p.Unwrap());
	return output;
}
void FWfcTile::GetSupportedTransforms(TSet<FWFC_Transform3D>& output) const
{
	output.Empty(); //This is important for BP calls; Unreal reuses collections without clearing them
	
	for (auto tr : GetSupportedTransforms())
		output.Emplace(tr);
}