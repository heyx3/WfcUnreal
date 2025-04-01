#include "WfcBpUtils.h"

FRotator UWfcUtils::WfcToFRotator(WFC_Directions3D face)
{
	switch (face)
	{
		case WFC_Directions3D::MinX: return { 0, 180, 0 };
		case WFC_Directions3D::MaxX: return { 0, 0, 0 };
		case WFC_Directions3D::MinY: return { 0, 90, 0 };
		case WFC_Directions3D::MaxY: return { 0, 270, 0 };
		case WFC_Directions3D::MinZ: return { -90, 0, 0 };
		case WFC_Directions3D::MaxZ: return { 90, 0, 0 };
		default: check(false); return { 0, 0, 0 };
	}
}
