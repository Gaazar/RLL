#pragma once
#include "win32internal.h"


//struct FilledGeometrySet : public  RLL::ISVGBuilder, public  RLL::ISVG // svg, submesh
//{
//	PathBuffer* buffer;
//	int geomCount = 0;
//	std::vector<short> indices;
//	std::vector<Math3D::Vector2> verts;
//	std::vector<Math3D::Vector2> uv;
//	std::vector<Math3D::Vector3> path_norm_fill;
//
//	FilledGeometrySet(PathBuffer* path) :buffer(path) {};
//	void PushGeometry(RLL::IGeometry* geom, RLL::IBrush* filler)
//	{
//		auto ptrg = dynamic_cast<Geometry*>(geom);
//		auto& g = *ptrg;
//		float fil = 0;
//		if (filler)
//		{
//			fil = ((Filling*)filler)->index;
//		}
//		int ibegin = verts.size() - 1;
//		verts.insert(verts.end(), g.verts.begin(), g.verts.end());
//		uv.insert(uv.end(), g.verts.begin(), g.verts.end());
//		for (auto i : g.path_norm)
//		{
//			path_norm_fill.push_back({ i.x,i.y,fil });
//		}
//		for (int i = 0; i < g.indices.size(); i++)
//		{
//			indices.push_back(ibegin + g.indices[i]);
//		}
//		geomCount++;
//	};
//	void PushGeometry(RLL::IGeometry* geom, Math3D::Matrix4x4& transform, RLL::IFilling* filler)// 2D transform, dont modify z-axis
//	{
//		auto ptrg = dynamic_cast<Geometry*>(geom);
//		auto& g = *ptrg;
//		float fil = 0;
//		if (filler)
//		{
//			fil = ((Filling*)filler)->index;
//		}
//		int ibegin = verts.size() - 1;
//		uv.insert(uv.end(), g.verts.begin(), g.verts.end());
//		for (auto i : g.verts)
//		{
//			Math3D::Vector4 pt{ i.x,i.y,0,1 };
//			pt = pt * transform;
//			verts.push_back({ pt.x,pt.y });
//		}
//		for (auto i : g.path_norm)
//		{
//			path_norm_fill.push_back({ i.x,i.y,fil });
//		}
//		for (int i = 0; i < g.indices.size(); i++)
//		{
//			indices.push_back(ibegin + g.indices[i]);
//		}
//		geomCount++;
//	};
//};
//final image is made with sets of vs/this
//for sets of fgs/svg to layout and paint
struct ViewCanvas
{


};
struct CBFrame
{
	Math3D::Matrix4x4 gWorldViewProj;
	Math3D::float4 dColor;
	float time;
	struct {
		unsigned int width;
		unsigned int height;
	}vwh;
	float reservedCBF;

};
struct CBObject
{
	Math3D::Matrix4x4 objToWorld;
	int sampleType;
	float timeBegin;
	float timePeriod;
	int timeLoop;
};
class D3D12PaintDevice;
struct CoreMesh
{
	struct SubMesh;
	enum VERTEX_FORMAT
	{
		VERTEX_FORMAT_POSITION,
		VERTEX_FORMAT_NORMAL,
		VERTEX_FORMAT_TANGENT,
		VERTEX_FORMAT_BONE_WEIGHT,
		VERTEX_FORMAT_COLOR,
		VERTEX_FORMAT_BONE_INDEX,
		VERTEX_FORMAT_UV,
		VERTEX_FORMAT_UV1,
		VERTEX_FORMAT_UV2,
		VERTEX_FORMAT_UV3,
	};
	Math3D::Vector3* vertices = nullptr; // x,y,normal
	Math3D::Vector3* pbpb = nullptr;
	unsigned short* indices = nullptr;

	unsigned int vertCount;
	unsigned int idxCount;
	UINT32 subMeshCount;
	SubMesh* subMeshs;

	ResourceBlob vertBuffer;
	ResourceBlob pbpbBuffer;
	ResourceBlob indexBuffer;

	D3D12_INDEX_BUFFER_VIEW ibv;
	D3D12_VERTEX_BUFFER_VIEW vbv[2];

public:
	CoreMesh()
	{
		idxCount = vertCount = subMeshCount = 0;
		vertices = nullptr;
		indices = nullptr;

		subMeshs = nullptr;
		vertBuffer.data = reinterpret_cast<void**>(&vertices);
		pbpbBuffer.data = reinterpret_cast<void**>(&pbpb);
		indexBuffer.data = reinterpret_cast<void**>(&indices);

	}
	struct SubMesh
	{
		CoreMesh* mesh;
		uint16_t startIndex;
		uint16_t indexCount;
		uint16_t startVertex;

		SubMesh() = default;
		SubMesh(CoreMesh* m, uint16_t si, uint16_t ic, uint16_t sv)
		{
			mesh = m;
			startIndex = si;
			indexCount = ic;
			startVertex = sv;
		}
	};

	void Upload(D3D12PaintDevice* device);
	void Uploaded(ID3D12Resource** gRes, int ioffset, int voffset);
	~CoreMesh();
};


