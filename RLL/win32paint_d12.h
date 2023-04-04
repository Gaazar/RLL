#pragma once
#include "Painter.h"
#include "vector"
#include "win32core_mesh.h"

#ifdef _DEBUG
#include <iostream>
#endif

struct D3D12GPUPath {
	unsigned int hi = 0;
	unsigned int  vi = 0;
	unsigned int  hl = 0;
	unsigned int  vl = 0;
};
struct D3D12GPUCurve {
	Math3D::Vector2 begin;
	Math3D::Vector2 control;
	Math3D::Vector2 end;
	bool IsLine()
	{
		return Math3D::Vector2::Cross(control - begin, end - control) == 0.f;
	}
	bool IsVertical()
	{
		if (!IsLine())
			return false;
		return (end - begin).x == 0;
	}
	bool IsHorizontal()
	{
		if (!IsLine())
			return false;
		return (end - begin).y == 0;

	}
	Math3D::Vector2 Max()
	{
		Math3D::Vector2 mx = begin;
		mx.x = max(mx.x, control.x);
		mx.x = max(mx.x, end.x);

		mx.y = max(mx.y, control.y);
		mx.y = max(mx.y, end.y);
		return mx;

	}
};
struct D3D12GPUBrush
{
public:
	Math3D::Vector2 center = { 0 }; //in directional is direction
	Math3D::Vector2 focus = { 0 };
	float radius = 0;
	unsigned int type = 0;// color mode: 0:foreground 1:directional 2:radial 3:sweep 4:tex
	unsigned int texid = 0;
	float pad = 0;
	float pos[8] = { 0 };
	RLL::Color stops[8] = {};
	static D3D12GPUBrush Foreground()
	{
		D3D12GPUBrush grads;
		grads.type = 0;
		return grads;
	};
	static D3D12GPUBrush Directional(
		Math3D::Vector2 direction,
		float pos0, RLL::Color col0,
		float pos1 = 2, RLL::Color col1 = { 0,0,0,0 },
		float pos2 = 2, RLL::Color col2 = { 0,0,0,0 },
		float pos3 = 2, RLL::Color col3 = { 0,0,0,0 },
		float pos4 = 2, RLL::Color col4 = { 0,0,0,0 },
		float pos5 = 2, RLL::Color col5 = { 0,0,0,0 },
		float pos6 = 2, RLL::Color col6 = { 0,0,0,0 },
		float pos7 = 2, RLL::Color col7 = { 0,0,0,0 }
	)
	{
		D3D12GPUBrush grads;
		grads.center = direction;
		grads.type = 1;
		grads.pos[0] = pos0;
		grads.pos[1] = pos1;
		grads.pos[2] = pos2;
		grads.pos[3] = pos3;
		grads.pos[4] = pos4;
		grads.pos[5] = pos5;
		grads.pos[6] = pos6;
		grads.pos[7] = pos7;

		grads.stops[0] = col0;
		grads.stops[1] = col1;
		grads.stops[2] = col2;
		grads.stops[3] = col3;
		grads.stops[4] = col4;
		grads.stops[5] = col5;
		grads.stops[6] = col6;
		grads.stops[7] = col7;
		return grads;
	}
	static D3D12GPUBrush Solid(
		RLL::Color col)
	{
		return Directional({ 1,0 }, 0, col, 1, col);
	}
	static D3D12GPUBrush Radial(
		Math3D::Vector2 center,
		float radius,
		float pos0, RLL::Color col0,
		float pos1 = 2, RLL::Color col1 = { 0,0,0,0 },
		float pos2 = 2, RLL::Color col2 = { 0,0,0,0 },
		float pos3 = 2, RLL::Color col3 = { 0,0,0,0 },
		float pos4 = 2, RLL::Color col4 = { 0,0,0,0 },
		float pos5 = 2, RLL::Color col5 = { 0,0,0,0 },
		float pos6 = 2, RLL::Color col6 = { 0,0,0,0 },
		float pos7 = 2, RLL::Color col7 = { 0,0,0,0 }
	)
	{
		D3D12GPUBrush grads;
		grads.center = center;
		grads.radius = radius;
		grads.focus = center;
		grads.type = 2;
		grads.pos[0] = pos0;
		grads.pos[1] = pos1;
		grads.pos[2] = pos2;
		grads.pos[3] = pos3;
		grads.pos[4] = pos4;
		grads.pos[5] = pos5;
		grads.pos[6] = pos6;
		grads.pos[7] = pos7;

		grads.stops[0] = col0;
		grads.stops[1] = col1;
		grads.stops[2] = col2;
		grads.stops[3] = col3;
		grads.stops[4] = col4;
		grads.stops[5] = col5;
		grads.stops[6] = col6;
		grads.stops[7] = col7;
		return grads;
	}
	static D3D12GPUBrush Sweep(
		Math3D::Vector2 center,
		float degree,
		float pos0, RLL::Color col0,
		float pos1 = 2, RLL::Color col1 = { 0,0,0,0 },
		float pos2 = 2, RLL::Color col2 = { 0,0,0,0 },
		float pos3 = 2, RLL::Color col3 = { 0,0,0,0 },
		float pos4 = 2, RLL::Color col4 = { 0,0,0,0 },
		float pos5 = 2, RLL::Color col5 = { 0,0,0,0 },
		float pos6 = 2, RLL::Color col6 = { 0,0,0,0 },
		float pos7 = 2, RLL::Color col7 = { 0,0,0,0 }
	)
	{
		D3D12GPUBrush grads;
		grads.center = center;
		grads.radius = degree;
		grads.type = 3;
		grads.pos[0] = pos0;
		grads.pos[1] = pos1;
		grads.pos[2] = pos2;
		grads.pos[3] = pos3;
		grads.pos[4] = pos4;
		grads.pos[5] = pos5;
		grads.pos[6] = pos6;
		grads.pos[7] = pos7;

		grads.stops[0] = col0;
		grads.stops[1] = col1;
		grads.stops[2] = col2;
		grads.stops[3] = col3;
		grads.stops[4] = col4;
		grads.stops[5] = col5;
		grads.stops[6] = col6;
		grads.stops[7] = col7;
		return grads;
	}
	static D3D12GPUBrush Texture(unsigned int texture)
	{
		D3D12GPUBrush grads;
		grads.texid = texture;
		return grads;
	}
};
struct D3D12GeometryMesh
{
	std::vector<short> indices;
	std::vector<Math3D::Vector2> verts;
	std::vector<Math3D::Vector2> uv;
	std::vector<Math3D::Vector2> path_norm;
	Math3D::Matrix4x4 uvTransform = Math3D::Matrix4x4::Identity();
	void MakeIndex(int off = 0)
	{
		std::vector<short> idcs;
		auto vs = verts.size() - off;

		if (vs == 3)
		{
			idcs = { 0,1,2 };
		}
		else if (vs == 4)
		{
			idcs = { 0,1,2,0,2,3 };
		}
		else if (vs == 5)
		{
			idcs = { 0,1,2,2,3,0,0,3,4 };
		}
		else if (vs == 6)
		{
			idcs = { 0,1,2,2,3,4,4,5,0,0,2,4 };
		}
		else
		{
			idcs.resize((vs - 2) * 3);
			for (int x = 0; x < idcs.size() / 3; x++)
			{
				idcs[x * 3 + 0] = 0;
				idcs[x * 3 + 1] = x + 1;
				idcs[x * 3 + 2] = x + 2;
			}
		}
		for (auto& i : idcs)
		{
			i += off;
		}
		indices.insert(indices.end(), idcs.begin(), idcs.end());
	}
	void SetPath(int index)
	{
		for (auto& i : path_norm)
			i.x = index;
	}
	void MakeNormal()
	{
		if (path_norm.size() != verts.size())
			path_norm.resize(verts.size());

		for (int v = 0; v < verts.size(); v++)
		{
			Math3D::Vector2 v_a, v_b;
			if (v == 0)
			{
				v_a = verts[0] - verts[verts.size() - 1];
			}
			else
			{
				v_a = verts[v] - verts[v - 1];
			}
			if (v == verts.size() - 1)
			{
				v_b = verts[v] - verts[0];
			}
			else
			{
				v_b = verts[v] - verts[v + 1];
			}
			auto norm = (v_a.Normalized() + v_b.Normalized());
			path_norm[v].y = atan2f(norm.y, norm.x);
		}
	}
	void MakeUV(Math3D::Matrix4x4& t)
	{
		uvTransform = t;
		uv.reserve(verts.size());
		for (auto& i : verts)
		{
			//auto u = i * t;
			uv.push_back(i * t);
		}
	}
	float GetArea() { return 0; };
	float GetCircumference() { return 0; };
};

struct PathBuffer
{
	std::vector<D3D12GPUPath> paths;
	std::vector<D3D12GPUCurve> curves;
	std::vector<D3D12GPUBrush> brushes;
	std::vector<CoreMesh*> meshes;
};
#define FLAG_D12PD_PATH_DIRTY 0x1
#define FLAG_D12PD_CURVE_DIRTY 0x2
#define FLAG_D12PD_BRUSH_DIRTY 0x4
#define FLAG_D12PD_MESH_DIRTY 0x8

struct MeshGroup
{
	std::vector<CoreMesh*> meshs;
	ID3D12Resource* resource[5];
	ResourceBlob uploads[5];
};

class D3D12PaintDevice : public RLL::IPaintDevice
{
	friend class D3D12GeometryBuilder;
	friend class D3D12SVGBuilder;
	friend class CoreMesh;

	void UploadMesh(CoreMesh* m);
	ID3D12Resource* MakeDefaultBuffer(ID3D12Resource* upload, int size);
public:
	int flag = 0b111;
	ID3D12Device5* d3dDevice = nullptr;
	ID3D12GraphicsCommandList* cmdList = nullptr;
	ID3D12RootSignature* coreSignature = nullptr;
	ID3D12PipelineState* corePSO = nullptr;
	ID3D12RootSignature* coreMorphSignature = nullptr;
	ID3D12PipelineState* coreMorphPSO = nullptr;

	PathBuffer paths;
	std::vector<MeshGroup> meshGroups;
	MeshGroup cMeshGroup;
	ID3D12Resource* gpuPath = nullptr;
	ID3D12Resource* gpuCurve = nullptr;
	ID3D12Resource* gpuBrush = nullptr;

	D3D12PaintDevice()
	{
		paths.brushes.push_back(D3D12GPUBrush::Foreground());
	};

	RLL::IBrush* CreateSolidColorBrush(RLL::Color c);
	RLL::IBrush* CreateDirectionalBrush(Math3D::Vector2 direction, RLL::ColorGradient* grad);
	RLL::IBrush* CreateRadialBrush(Math3D::Vector2 center, float radius, RLL::ColorGradient* grad);
	RLL::IBrush* CreateSweepBrush(Math3D::Vector2 center, float degree, RLL::ColorGradient* grad);
	RLL::IBrush* CreateTexturedBrush(RLL::ITexture* tex, void* sampleMode) { NOIMPL; return nullptr; };
	RLL::ITexture* CreateTexture() { NOIMPL; return nullptr; };
	RLL::ITexture* CreateTextureFromFile() { NOIMPL; return nullptr; };
	RLL::ITexture* CreateTextureFromStream() { NOIMPL; return nullptr; };
	RLL::ITexture* CreateTextureFromMemory() { NOIMPL; return nullptr; };

	RLL::IGeometryBuilder* CreateGeometryBuilder();
	RLL::ISVGBuilder* CreateSVGBuilder();

	RLL::IPaintContext* CreateContext() { NOIMPL; return nullptr; };

	void CopyTexture(RLL::ITexture* src, RLL::ITexture* dst) { NOIMPL; return; };
	void CopyTextureRegion(RLL::ITexture* src, RLL::ITexture* dst, RLL::RectangleI) { NOIMPL; return; };


	ID3D12Resource* CreateDefaultBuffer(const void* initData, UINT64 size, D3D12_RESOURCE_FLAGS flag = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
	ResourceBlob CreateUploadBuffer(UINT64 size);
	void CommitChange();
	void PostFlush();

	void DrawCanvas();
	void DrawSVG(RLL::ISVG* svg);
	void DrawGeometry(RLL::IGeometry* geom, RLL::IBrush* br = nullptr, Math3D::Matrix4x4* transform = nullptr);

};

class D3D12Brush : public RLL::IBrush
{
	friend class D3D12PaintDevice;
	friend class D3D12SVGBuilder;
	D3D12PaintDevice* device;
	int id;
	D3D12GPUBrush* brush;
	D3D12Brush(D3D12PaintDevice* dev) :device(dev) { id = 0; brush = nullptr; };
public:
	void Dispose();
};
class D3D12Geometry : public RLL::IGeometry
{
	friend class D3D12GeometryBuilder;
	friend class D3D12SVGBuilder;
public:
	D3D12PaintDevice* device;
	int pathID = -1;
	D3D12GPUPath* path = nullptr;
	D3D12GeometryMesh* mesh = nullptr;

	D3D12Geometry(D3D12PaintDevice* dev) :device(dev) {  };
public:
	void Dispose();

};
class D3D12GeometryBuilder : public RLL::IGeometryBuilder
{
	D3D12PaintDevice* device;
	struct SubPath
	{
		std::vector<D3D12GPUCurve> curves;
		bool closed = false;
	};
	Math3D::float2 o;
	std::vector<SubPath> paths;
	std::vector<Math3D::Vector2> points;
	SubPath cPath;
	bool opening = false;

	friend class D3D12PaintDevice;
	D3D12GeometryBuilder(D3D12PaintDevice* dev) :device(dev) { cPath.curves.reserve(256); points.reserve(256); };
public:
	void Begin(Math3D::Vector2 p);
	void End(bool close);
	void LineTo(Math3D::Vector2 to);
	void QuadraticTo(Math3D::Vector2 control, Math3D::Vector2 to);
	void CubicTo(Math3D::Vector2 control0, Math3D::Vector2 control1, Math3D::Vector2 to);
	void ArcTo(Math3D::Vector2 center, Math3D::Vector2 radius, float begin_rad, float end_rad, RLL::ARC_TYPE t);
	void Ellipse(Math3D::Vector2 center, Math3D::Vector2 radius, bool inv);
	void Rectangle(Math3D::Vector2 lt, Math3D::Vector2 rb, bool inv);
	void Triangle(Math3D::Vector2 p0, Math3D::Vector2 p1, Math3D::Vector2 p2, bool inv);
	void RoundRectangle(Math3D::Vector2 lt, Math3D::Vector2 rb, Math3D::Vector2 radius, bool inv);

	RLL::IGeometry* Fill(Math3D::Matrix4x4* bgTransform);
	RLL::IGeometry* Stroke(float stroke, RLL::StrokeStyle* type, Math3D::Matrix4x4* bgTransform);
	void Reset();
	void Dispose();

};

class D3D12SVG :public RLL::ISVG
{
	friend class D3D12SVGBuilder;
public:
	D3D12PaintDevice* device;
	int id = -1;
	CoreMesh* mesh = nullptr;
	D3D12SVG(D3D12PaintDevice* dev) :device(dev) {  };
public:
	void Dispose();

};

class D3D12SVGBuilder : public::RLL::ISVGBuilder
{
	friend class D3D12PaintDevice;
	D3D12PaintDevice* device = nullptr;
	struct Layer
	{
		enum class TYPE
		{
			TYPE_GEOMETRY,
			TYPE_SVG
		};
		union
		{
			struct {
				D3D12Geometry* geometry;
				D3D12Brush* brush;
			};
			D3D12SVG* svg;
		};
		TYPE type = TYPE::TYPE_GEOMETRY;
		Math3D::Matrix4x4 transform;
		Layer()
		{
			geometry = nullptr;
			brush = nullptr;
			svg = nullptr;
			transform = {};
		}
		Layer(const Layer& l) : Layer()
		{
			memcpy(this, &l, sizeof(l));
			if (type == TYPE::TYPE_GEOMETRY)
			{
				geometry->AddRef();
				if (brush)
					brush->AddRef();
			}
			else if (type == TYPE::TYPE_SVG)
			{
				svg->AddRef();
			}
		}
		~Layer()
		{
			//std::cout << "Layer desctruct." << std::endl;
			if (type == TYPE::TYPE_GEOMETRY)
			{
				geometry->Release();
				if (brush)
					brush->Release();
			}
			else if (type == TYPE::TYPE_SVG)
			{
				svg->Release();
			}
		}
	};
	std::vector<Layer> layers;

	D3D12SVGBuilder(D3D12PaintDevice* dev) :device(dev) { };
public:
	void Push(RLL::IGeometry* geom, RLL::IBrush* brush, Math3D::Matrix4x4* transform = nullptr);
	void Push(RLL::ISVG* svg, Math3D::Matrix4x4* transform = nullptr);
	void Reset();
	RLL::ISVG* Commit();
	void Dispose();
};