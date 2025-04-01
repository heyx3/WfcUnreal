#include "WfcFacePrototype.h"


WFC::Tiled3D::FaceIdentifiers FWfcFacePrototype::Unwrap(int pointIdOffset) const
{
	WFC::Tiled3D::FaceIdentifiers id;
		
	#define WFCPP_CASE(Domain, point) \
		id.Domain##s[WFC::Tiled3D::FacePoints::point] = \
		pointIdOffset + static_cast<int>(Domain##point);
	WFCPP_CASE(Corner, AA);
	WFCPP_CASE(Corner, AB);
	WFCPP_CASE(Corner, BA);
	WFCPP_CASE(Corner, BB);
	WFCPP_CASE(Edge, AA);
	WFCPP_CASE(Edge, AB);
	WFCPP_CASE(Edge, BA);
	WFCPP_CASE(Edge, BB);
		
	return id;
}

const FString* FWfcFacePrototype::GetName(PointSymmetry point) const
{
	switch (point)
	{
		case PointSymmetry::a: return nullptr;
	    case PointSymmetry::b: return &NameOfB;
	    case PointSymmetry::c: return &NameOfC;
	    case PointSymmetry::d: return &NameOfD;
		default: check(false); return nullptr;
	}
}
