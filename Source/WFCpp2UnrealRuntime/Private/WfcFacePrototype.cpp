#include "WfcFacePrototype.h"


TArray<FString, TInlineAllocator<3>> FWfcFacePointDefs::GatherPoints() const
{
	TArray<FString, TInlineAllocator<3>> output;
	if (AddPoint1)
	{
		output.Add(NameOfPoint1);
		if (AddPoint2)
		{
			output.Add(NameOfPoint2);
			if (AddPoint3)
			{
				output.Add(NameOfPoint3);
			}
		}
	}
	return output;
}
EWfcPointID& FWfcFacePointDefs::PointAt(WFC::Tiled3D::FacePoints point)
{
	switch (point)
	{
		case WFC::Tiled3D::FacePoints::AA: return PointAA;
		case WFC::Tiled3D::FacePoints::AB: return PointAB;
		case WFC::Tiled3D::FacePoints::BA: return PointBA;
		case WFC::Tiled3D::FacePoints::BB: return PointBB;
		default: check(false); return PointAA;
	}
}
void FWfcFacePointDefs::PostScriptConstruct()
{
	//Sanitize the flags.
	if (!AddPoint1)
	{
		NameOfPoint1 = TEXT("UNASSIGNED: 1");
		
		AddPoint2 = false;
		NameOfPoint2 = TEXT("UNASSIGNED: 2");
	}
	if (!AddPoint1 || !AddPoint2)
	{
		AddPoint3 = false;
		NameOfPoint3 = TEXT("UNASSIGNED: 3");
	}
}
const FString* FWfcFacePointDefs::GetName(EWfcPointID pointID) const
{
	switch (pointID)
	{
		case EWfcPointID::null: return nullptr;
		case EWfcPointID::p1: return &NameOfPoint1;
		case EWfcPointID::p2: return &NameOfPoint2;
		case EWfcPointID::p3: return &NameOfPoint3;
		default: check(false); return nullptr;
	}
}

WFC::Tiled3D::FaceIdentifiers FWfcFacePrototype::Unwrap(int pointIdOffset) const
{
	WFC::Tiled3D::FaceIdentifiers id;

	auto doElement = [&](WFC::Tiled3D::PerFacePoint<WFC::Tiled3D::PointID>& outputs,
						WFC::Tiled3D::FacePoints facePoint,
						const FWfcFacePointDefs& definitions)
	{
		outputs[facePoint] = pointIdOffset + static_cast<int>(definitions.PointAt(facePoint));
	};
	for (int i = 0; i < WFC::Tiled3D::N_FACE_POINTS; ++i)
	{
		auto facePoint = static_cast<WFC::Tiled3D::FacePoints>(i);
		doElement(id.Corners, facePoint, Corners);
		doElement(id.Edges, facePoint, Edges);
	}
		
	return id;
}
