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

	vector<Vector3> posn;
	vector<Int4> pbpb;
	vector<short> ind;
	int vb = 0, ib = 0;
	posn.reserve(512);
	pbpb.reserve(512);
	ind.reserve(512);

	for (auto& i : layers)
	{
		switch (i.type)
		{
		case Layer::TYPE::TYPE_GEOMETRY:
		{
			//uvMat._41 = i.transform._41;
			//uvMat._42 = i.transform._42;
			D3D12GeometryMesh& m = *i.geometry->mesh;
			auto normT = i.transform;
			normT._41 = 0;
			normT._42 = 0;
			normT._43 = 0;
			for (auto& n : m.verts)
			{
				Vector3 novp = n;
				novp.z = 0;
				novp = novp * i.transform;
				auto norm = Vector2(cosf(n.y), sinf(n.y));
				norm = norm * normT;
				auto novn = atan2f(norm.y, norm.x);
				novp.z = novn;
				posn.push_back(novp);
			}
			for (auto& n : m.indices)
				ind.push_back(n + vb);

			for (auto& n : m.pbpb)
			{
				if (i.brush)
					pbpb.push_back({ n.x,i.brush->id,0,0 });
				else
					pbpb.push_back({ n.x,0,0,0 });
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


				auto normT = i.transform;
				normT._41 = 0;
				normT._42 = 0;
				normT._43 = 0;

				auto norm = Vector2(cosf(v.pnbs[n].y), sinf(v.pnbs[n].y));
				norm = norm * normT;
				auto ny = atan2f(norm.y, norm.x);
				auto pnbT = v.pnbs[n];
				pnbT.y = ny;
				pnb.push_back(pnbT);

				auto ruvT = Matrix4x4::Identity();
				ruvT._11 = v.tfs[n].x;
				ruvT._12 = v.tfs[n].y;
				ruvT._21 = v.tfs[n].z;
				ruvT._22 = v.tfs[n].w;
				auto uvMat = (i.transform).Inversed() * ruvT;

				tfs.push_back({ uvMat._11,uvMat._12,uvMat._21,uvMat._22 });
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
	if (pos.size() < 3)
	{
		SetError("Invalid svg, vertices less than 3.");
		return nullptr;
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
