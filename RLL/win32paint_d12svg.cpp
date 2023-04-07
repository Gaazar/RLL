#include "win32paint_d12.h"
using namespace RLL;
using namespace std;
using namespace Math3D;

void D3D12SVGBuilder::Push(RLL::IGeometry* geom, RLL::IBrush* brush, Matrix4x4* transform)
{
	if (geom == nullptr)
	{
		SetError("parameter geom is a null pointer.");
		return;
	}
	Layer l;
	l.type = Layer::TYPE::TYPE_GEOMETRY;
	l.geometry = (D3D12Geometry*)geom;
	l.brush = (D3D12Brush*)brush;
	if (transform)
		l.transform = *transform;
	else
		l.transform = Matrix4x4::Identity();
	layers.push_back(l);
	geom->AddRef();
	if (brush) brush->AddRef();
}
void D3D12SVGBuilder::Push(RLL::ISVG* svg, Matrix4x4* transform)
{
	if (svg == nullptr)
	{
		SetError("parameter svg is a null pointer.");
		return;
	}
	Layer l;
	l.type = Layer::TYPE::TYPE_SVG;
	l.geometry = (D3D12Geometry*)svg;
	if (transform)
		l.transform = *transform;
	else
		l.transform = Matrix4x4::Identity();
	layers.push_back(l);
	svg->AddRef();
}
void D3D12SVGBuilder::Reset()
{
	layers.clear();
}
RLL::ISVG* D3D12SVGBuilder::Commit()
{
	if (!layers.size())
	{
		SetError("Empty SVGBuilder. No layers pushed.");
		return nullptr;
	}

	vector<Vector2> uv;
	vector<Vector2> pos;
	vector<Vector3> pnb;
	vector<Vector4> tfs;
	vector<short> ind;
	int vb = 0, ib = 0;
	uv.reserve(512);
	pos.reserve(512);
	pnb.reserve(512);
	ind.reserve(512);
	tfs.reserve(512);

	for (auto& i : layers)
	{
		switch (i.type)
		{
		case Layer::TYPE::TYPE_GEOMETRY:
		{
			//uvMat._41 = i.transform._41;
			//uvMat._42 = i.transform._42;
			D3D12GeometryMesh& m = *i.geometry->mesh;
			auto uvMat = (i.transform).Inversed() * m.uvTransform;
			for (auto& n : m.verts)
			{
				pos.push_back(n * i.transform);
				tfs.push_back({ uvMat._11,uvMat._12,uvMat._21,uvMat._22 });
			}
			for (auto& n : m.uv)
			{
				uv.push_back(n);
			}
			for (auto& n : m.indices)
				ind.push_back(n + vb);
			auto normT = i.transform;
			normT._41 = 0;
			normT._42 = 0;
			normT._43 = 0;
			for (auto& n : m.path_norm)
			{
				auto norm = Vector2(cosf(n.y), sinf(n.y));
				norm = norm * normT;
				auto ny = atan2f(norm.y, norm.x);

				if (i.brush)
					pnb.push_back({ n.x,ny,(float)i.brush->id });
				else
					pnb.push_back({ n.x,ny,0.f });
			}

			vb += m.verts.size();
			ib += m.indices.size();
		}
		break;
		case Layer::TYPE::TYPE_SVG:
		{
			CoreMesh& v = *i.svg->mesh;
			for (int n = 0; n < v.vertCount; n++)
			{
				pos.push_back(v.vertices[n] * i.transform);
				uv.push_back(v.uvs[n]);
				pnb.push_back(v.pnbs[n]);
				tfs.push_back(v.tfs[n]);
			}
			for (int n = 0; n < v.idxCount; n++)
			{
				ind.push_back(vb + v.indices[n]);
			}

			vb += v.vertCount;
			ib += v.idxCount;
			//NOIMPL;
		}
		break;

		default:
			break;
		}
	}

	auto svg = new D3D12SVG(device);
	svg->id = device->AllocateMesh();
	auto corem = device->paths.meshes[svg->id];
	svg->mesh = corem;
	corem->vertCount = uv.size();
	corem->idxCount = ind.size();
	corem->indices = new unsigned short[corem->idxCount];
	corem->uvs = new Vector2[corem->vertCount];
	corem->pnbs = new Vector3[corem->vertCount];
	corem->vertices = new Vector2[corem->vertCount];
	corem->tfs = new Vector4[corem->vertCount];
#define vecize(x) (sizeof(x[0]) * x.size())
	memcpy(corem->indices, &ind[0], vecize(ind));
	memcpy(corem->uvs, &uv[0], vecize(uv));
	memcpy(corem->pnbs, &pnb[0], vecize(pnb));
	memcpy(corem->vertices, &pos[0], vecize(pos));
	memcpy(corem->tfs, &tfs[0], vecize(tfs));

	//corem.UploadToGPUMemory
	corem->Upload(device);
	device->flag |= FLAG_D12PD_PATH_DIRTY | FLAG_D12PD_CURVE_DIRTY;
	return svg;
}
void D3D12SVGBuilder::Dispose()
{
	delete this;
}
void D3D12SVG::Dispose()
{
	delete mesh;
	device->paths.meshes[id] = nullptr;
	id = -1;
	delete this;
}
