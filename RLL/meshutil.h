#pragma once
#include "math3d/Math3Df.h"

struct LayoutMesh
{
	Math3D::float3 *position;
	Math3D::float2 *uv;
	Math3D::float2 *pathnorm;

	int vertexCount;
	int triangleCount;

	bool Reserve();
	bool Append(LayoutMesh&);

private:
	void* Allocate(int vc,int tc);
	void Free();
};