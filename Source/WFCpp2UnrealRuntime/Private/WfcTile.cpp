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

void FWfcTile::GetSupportedTransforms(TSet<FWFC_Transform3D>& output) const
{
	output.Empty(); //This is important for BP calls; Unreal reuses collections without clearing them
	output.Append(PrecisePermutations);

	//In the future we can hopefully support the more complex settings mentioned in the header.
}