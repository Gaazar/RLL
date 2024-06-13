#include "win32paint_d12.h"

bool D3D12Morph::BuildMesh()
{
	if (!Check()) return false;
	std::vector<Math3D::Vector2> convex_comp;
	std::vector<Math3D::Vector2> convex;
	convex_comp.assign(from_geom->mesh->verts.begin(), from_geom->mesh->verts.end());
	//convex_comp.insert();

}