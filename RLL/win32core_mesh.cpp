#include "win32core_mesh.h"
#include "win32impl.h"
#include "win32paint_d12.h"
#include <algorithm>
#include <cstdlib>
using namespace RLL;
using namespace Math3D;


void CoreMesh::Upload(D3D12PaintDevice* device)
{
	indexBuffer.data = &indices;
	indexBuffer.size = idxCount * sizeof(short);
	//indexBuffer.gpuBuffer = device->CreateDefaultBuffer(indices, idxCount * sizeof(short));

	uvBuffer.data = &uvs;
	uvBuffer.size = vertCount * sizeof(Vector2);
	//uvBuffer.gpuBuffer = device->CreateDefaultBuffer(uvs, vertCount * sizeof(Vector2));
	pnbBuffer.data = &pnbs;
	pnbBuffer.size = vertCount * sizeof(Vector3);
	//pnbBuffer.gpuBuffer = device->CreateDefaultBuffer(pnbs, vertCount * sizeof(Vector3));
	vertBuffer.data = &vertices;
	vertBuffer.size = vertCount * sizeof(Vector2);
	//vertBuffer.gpuBuffer = device->CreateDefaultBuffer(vertices, vertCount * sizeof(Vector3));
	tfBuffer.data = &tfs;
	tfBuffer.size = vertCount * sizeof(Vector4);

	ibv.Format = DXGI_FORMAT_R16_UINT;
	ibv.SizeInBytes = indexBuffer.size;
	//ibv.BufferLocation = indexBuffer.gpuBuffer->GetGPUVirtualAddress();
	//0: vert 1:uv 2:pnb
	vbv[0].SizeInBytes = vertBuffer.size;
	//vbv[0].BufferLocation = vertBuffer.gpuBuffer->GetGPUVirtualAddress();
	//vbv[0].StrideInBytes = 12;

	vbv[1].SizeInBytes = uvBuffer.size;
	//vbv[1].BufferLocation = uvBuffer.gpuBuffer->GetGPUVirtualAddress();
	//vbv[1].StrideInBytes = 8;

	vbv[2].SizeInBytes = pnbBuffer.size;
	//vbv[2].BufferLocation = pnbBuffer.gpuBuffer->GetGPUVirtualAddress();
	//vbv[2].StrideInBytes = 12;
	vbv[3].SizeInBytes = tfBuffer.size;
	device->UploadMesh(this);
}
void CoreMesh::Uploaded(ID3D12Resource** gRes, int ioffset, int voffset)
{
	for (int i = 0; i < 5; i++)
	{
		gRes[i]->AddRef();
	}
	indexBuffer.gpuBuffer = gRes[0];
	vertBuffer.gpuBuffer = gRes[1];
	uvBuffer.gpuBuffer = gRes[2];
	pnbBuffer.gpuBuffer = gRes[3];
	tfBuffer.gpuBuffer = gRes[4];

	ibv.Format = DXGI_FORMAT_R16_UINT;
	ibv.SizeInBytes = indexBuffer.size;
	ibv.BufferLocation = gRes[0]->GetGPUVirtualAddress() + ioffset * sizeof(*indices);
	//0: vert 1:uv 2:pnb
	vbv[0].SizeInBytes = vertBuffer.size;
	vbv[0].BufferLocation = gRes[1]->GetGPUVirtualAddress() + voffset * sizeof(*vertices);;
	vbv[0].StrideInBytes = 8;

	vbv[1].SizeInBytes = uvBuffer.size;
	vbv[1].BufferLocation = gRes[2]->GetGPUVirtualAddress() + voffset * sizeof(*uvs);;
	vbv[1].StrideInBytes = 8;

	vbv[2].SizeInBytes = pnbBuffer.size;
	vbv[2].BufferLocation = gRes[3]->GetGPUVirtualAddress() + voffset * sizeof(*pnbs);;
	vbv[2].StrideInBytes = 12;

	vbv[3].SizeInBytes = tfBuffer.size;
	vbv[3].BufferLocation = gRes[4]->GetGPUVirtualAddress() + voffset * sizeof(*tfs);;
	vbv[3].StrideInBytes = 16;

}

CoreMesh::~CoreMesh()
{
	ibv = {};
	vbv[0] = {};
	vbv[1] = {};
	vbv[2] = {};
	vbv[3] = {};
	delete[] uvs;
	delete[] vertices;
	delete[] pnbs;
	delete[] indices;
	//delete[] tfs;
	uvs = nullptr;
	vertices = nullptr;
	pnbs = nullptr;
	indices = nullptr;
	tfs = nullptr;
	indexBuffer.gpuBuffer->Release();
	vertBuffer.gpuBuffer->Release();
	uvBuffer.gpuBuffer->Release();
	pnbBuffer.gpuBuffer->Release();
	tfBuffer.gpuBuffer->Release();
}
