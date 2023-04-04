#include "win32paint_d12.h"
#include <algorithm>
#include "curve_tools.h"

#define NO_TIMECHECKER
#define ELLIPSE_QUALITY 8
#include "perf_util.h"
using namespace RLL;
using namespace std;
using namespace Math3D;
#pragma region ConvexTools
bool ccw(const Vector2& a, const Vector2& b, const Vector2& c) {
	return ((b.x - a.x) * (c.y - a.y)) > ((b.y - a.y) * (c.x - a.x));
}
void convexHull(std::vector<Vector2>& p, std::vector<Vector2>& convex) {
	if (p.size() == 0) return;
	convex.clear();
	std::sort(p.begin(), p.end(), [](Vector2& a, Vector2& b) {
		if (a.x == b.x) return a.y < b.y;
		return a.x < b.x;
		});


	// lower hull
	for (const auto& pt : p) {
		while (convex.size() >= 2 && !ccw(convex.at(convex.size() - 2), convex.at(convex.size() - 1), pt)) {
			convex.pop_back();
		}
		convex.push_back(pt);
	}

	// upper hull
	auto t = convex.size() + 1;
	for (auto it = p.crbegin(); it != p.crend(); it = std::next(it)) {
		auto pt = *it;
		while (convex.size() >= t && !ccw(convex.at(convex.size() - 2), convex.at(convex.size() - 1), pt)) {
			convex.pop_back();
		}
		convex.push_back(pt);
	}

	convex.pop_back();
	return;
}
template<typename T>
T ring(std::vector<T>& v, int index)
{
	int i = index % (int)v.size();
	if (i < 0) i += v.size();
	return v[i];
}
template<typename T>
int ring_i(std::vector<T>& v, int index)
{
	int i = index % v.size();
	if (i < 0) i += v.size();
	return i;
}
bool simplifyConvex(std::vector<Vector2>& cvxh, int t_count)
{
	if (cvxh.size() <= 3 || t_count <= 2 || cvxh.size() - t_count <= 0) return false;
	for (auto i = 0; i < cvxh.size(); i++)
	{
		Vector2 v1 = ring(cvxh, i + 1) - cvxh[i];
		Vector2 v2 = ring(cvxh, i - 1) - cvxh[i];
		if (abs(Vector2::Cross(v1, v2)) < 0.0001f)
		{
			cvxh.erase(cvxh.begin() + i);
			i--;
		}
	}
	if (cvxh.size() <= 3 || cvxh.size() - t_count <= 0) return true;
	std::vector<float> len;
	len.resize(cvxh.size());
	for (auto i = 0; i < cvxh.size(); i++)
	{
		len[i] = (ring(cvxh, i + 1) - cvxh[i]).Magnitude();
	}
	int c_count = cvxh.size();
#undef min;
	for (; c_count > t_count && len.size() > 0;)
	{
		auto mlen = std::min_element(len.begin(), len.end());
		int i = mlen - len.begin();
		Vector2 v_b = ring(cvxh, i) - ring(cvxh, i - 1);
		Vector2 v_a = ring(cvxh, i + 1) - ring(cvxh, i + 2);
		Vector2 v_c = ring(cvxh, i) - ring(cvxh, i + 1);
		float rad_c = acosf(Vector2::Dot(v_a, v_b) / v_a.Magnitude() / v_b.Magnitude());
		if (rad_c < 15 * DEG2RAD)//test less
		{
			*mlen = 9e16f;
			continue;
		}
		float c = v_c.Magnitude();

		float rad_b = acosf(Vector2::Dot(v_c, v_a) / v_c.Magnitude() / v_a.Magnitude());
		float b = c / sinf(rad_c) * sinf(rad_b);
		cvxh.erase(cvxh.begin() + ring_i(len, i + 1));
		len.erase(len.begin() + i);
		if (i >= cvxh.size())
		{
			i--;
		}
		cvxh[i] = cvxh[i] + v_b.Normalized() * b;
		for (auto i = 0; i < cvxh.size(); i++)
		{
			len[i] = (ring(cvxh, i + 1) - cvxh[i]).Magnitude();
		}

		c_count--;
		if (c_count == 0)
			break;
	}

	return true;
}
#pragma endregion
constexpr UINT CalcConstantBufferByteSize(UINT byteSize)
{
	return (byteSize + 255) & ~255;
}

IBrush* D3D12PaintDevice::CreateSolidColorBrush(RLL::Color c)
{
	D3D12Brush* br = new D3D12Brush(this);
	br->id = paths.brushes.size();
	paths.brushes.push_back({});
	br->brush = &paths.brushes[br->id];
	*(br->brush) = D3D12GPUBrush::Solid(c);
	flag |= FLAG_D12PD_BRUSH_DIRTY;
	return br;
}
IBrush* D3D12PaintDevice::CreateDirectionalBrush(Math3D::Vector2 direction, RLL::ColorGradient* grad)
{
	D3D12Brush* br = new D3D12Brush(this);
	br->id = paths.brushes.size();
	paths.brushes.push_back({});
	br->brush = &paths.brushes[br->id];
	*(br->brush) = D3D12GPUBrush::Directional(direction,
		grad->positions[0], grad->colors[0],
		grad->positions[1], grad->colors[1],
		grad->positions[2], grad->colors[2],
		grad->positions[3], grad->colors[3],
		grad->positions[4], grad->colors[4],
		grad->positions[5], grad->colors[5],
		grad->positions[6], grad->colors[6],
		grad->positions[7], grad->colors[7]);
	flag |= FLAG_D12PD_BRUSH_DIRTY;
	return br;
}
IBrush* D3D12PaintDevice::CreateRadialBrush(Math3D::Vector2 center, float radius, RLL::ColorGradient* grad)
{
	D3D12Brush* br = new D3D12Brush(this);
	br->id = paths.brushes.size();
	paths.brushes.push_back({});
	br->brush = &paths.brushes[br->id];
	*(br->brush) = D3D12GPUBrush::Radial(center, radius,
		grad->positions[0], grad->colors[0],
		grad->positions[1], grad->colors[1],
		grad->positions[2], grad->colors[2],
		grad->positions[3], grad->colors[3],
		grad->positions[4], grad->colors[4],
		grad->positions[5], grad->colors[5],
		grad->positions[6], grad->colors[6],
		grad->positions[7], grad->colors[7]);
	flag |= FLAG_D12PD_BRUSH_DIRTY;
	return br;
}
IBrush* D3D12PaintDevice::CreateSweepBrush(Math3D::Vector2 center, float degree, RLL::ColorGradient* grad)
{
	D3D12Brush* br = new D3D12Brush(this);
	br->id = paths.brushes.size();
	paths.brushes.push_back({});
	br->brush = &paths.brushes[br->id];
	*(br->brush) = D3D12GPUBrush::Sweep(center, degree,
		grad->positions[0], grad->colors[0],
		grad->positions[1], grad->colors[1],
		grad->positions[2], grad->colors[2],
		grad->positions[3], grad->colors[3],
		grad->positions[4], grad->colors[4],
		grad->positions[5], grad->colors[5],
		grad->positions[6], grad->colors[6],
		grad->positions[7], grad->colors[7]);
	flag |= FLAG_D12PD_BRUSH_DIRTY;
	return br;
}
IGeometryBuilder* D3D12PaintDevice::CreateGeometryBuilder()
{
	return new D3D12GeometryBuilder(this);
}
ISVGBuilder* D3D12PaintDevice::CreateSVGBuilder()
{
	return new D3D12SVGBuilder(this);

}
ID3D12Resource* D3D12PaintDevice::CreateDefaultBuffer(const void* initData, UINT64 size, D3D12_RESOURCE_FLAGS flag)
{
	ID3D12Resource* uploadBuffer = nullptr;
	//创建上传堆，作用是：写入CPU内存数据，并传输给默认堆
	SUCCESS(d3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), //创建上传堆类型的堆
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),//变体的构造函数，传入byteSize，其他均为默认值，简化书写
		D3D12_RESOURCE_STATE_GENERIC_READ,	//上传堆里的资源需要复制给默认堆，所以是可读状态
		nullptr,	//不是深度模板资源，不用指定优化值
		IID_PPV_ARGS(&uploadBuffer)));

	//创建默认堆，作为上传堆的数据传输对象
	ID3D12Resource* defaultBuffer = nullptr;
	SUCCESS(d3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),//创建默认堆类型的堆
		D3D12_HEAP_FLAG_SHARED,
		&CD3DX12_RESOURCE_DESC::Buffer(size
			, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET),
		D3D12_RESOURCE_STATE_COMMON,//默认堆为最终存储数据的地方，所以暂时初始化为普通状态
		nullptr,
		IID_PPV_ARGS(&defaultBuffer)));



	//将资源从COMMON状态转换到COPY_DEST状态（默认堆此时作为接收数据的目标）
	cmdList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer,
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_COPY_DEST));

	//将数据从CPU内存拷贝到GPU缓存
	D3D12_SUBRESOURCE_DATA subResourceData;
	subResourceData.pData = initData;
	subResourceData.RowPitch = size;
	subResourceData.SlicePitch = subResourceData.RowPitch;
	//核心函数UpdateSubresources，将数据从CPU内存拷贝至上传堆，再从上传堆拷贝至默认堆。1是最大的子资源的下标（模板中定义，意为有2个子资源）
	UpdateSubresources<1>(cmdList, defaultBuffer, uploadBuffer, 0, 0, 1, &subResourceData);

	//再次将资源从COPY_DEST状态转换到GENERIC_READ状态(现在只提供给着色器访问)
	cmdList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer,
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_GENERIC_READ));



	return defaultBuffer;

}
ResourceBlob D3D12PaintDevice::CreateUploadBuffer(UINT64 size)
{
	ResourceBlob blob;
	ID3D12Resource* gpuBuffer = nullptr;
	UINT elementByteSize;
	blob.size = size;
	elementByteSize = CalcConstantBufferByteSize(size);
	blob.size = elementByteSize;
	SUCCESS(d3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(elementByteSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&gpuBuffer)));
	//返回欲更新资源的指针
	gpuBuffer->Map(0,//子资源索引，对于缓冲区来说，他的子资源就是自己
		nullptr,//对整个资源进行映射
		&blob.data);//返回待映射资源数据的目标内存块
	blob.gpuBuffer = gpuBuffer;

	return blob;

}
ID3D12Resource* D3D12PaintDevice::MakeDefaultBuffer(ID3D12Resource* upload, int size)
{
	//创建默认堆，作为上传堆的数据传输对象
	ID3D12Resource* defaultBuffer = nullptr;
	size = CalcConstantBufferByteSize(size);
	SUCCESS(d3dDevice->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),//创建默认堆类型的堆
		D3D12_HEAP_FLAG_SHARED,
		&CD3DX12_RESOURCE_DESC::Buffer(size
			, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET),
		D3D12_RESOURCE_STATE_COMMON,//默认堆为最终存储数据的地方，所以暂时初始化为普通状态
		nullptr,
		IID_PPV_ARGS(&defaultBuffer)));


	//将资源从COMMON状态转换到COPY_DEST状态（默认堆此时作为接收数据的目标）
	cmdList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer,
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_COPY_DEST));

	cmdList->CopyResource(defaultBuffer, upload);
	//再次将资源从COPY_DEST状态转换到GENERIC_READ状态(现在只提供给着色器访问)
	cmdList->ResourceBarrier(1,
		&CD3DX12_RESOURCE_BARRIER::Transition(defaultBuffer,
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_GENERIC_READ));

	return defaultBuffer;

}

void D3D12PaintDevice::CommitChange()
{
	if (flag & FLAG_D12PD_PATH_DIRTY)
	{
		if (gpuPath) gpuPath->Release();
		gpuPath = CreateDefaultBuffer(&paths.paths[0], sizeof(D3D12GPUPath) * paths.paths.size(), D3D12_RESOURCE_FLAG_NONE);
		flag &= ~FLAG_D12PD_PATH_DIRTY;
	}
	if (flag & FLAG_D12PD_CURVE_DIRTY)
	{
		if (gpuCurve) gpuCurve->Release();
		gpuCurve = CreateDefaultBuffer(&paths.curves[0], sizeof(D3D12GPUCurve) * paths.curves.size(), D3D12_RESOURCE_FLAG_NONE);
		flag &= ~FLAG_D12PD_CURVE_DIRTY;

	}
	if (flag & FLAG_D12PD_BRUSH_DIRTY)
	{
		if (gpuBrush) gpuBrush->Release();
		gpuBrush = CreateDefaultBuffer(&paths.brushes[0], sizeof(D3D12GPUBrush) * paths.brushes.size(), D3D12_RESOURCE_FLAG_NONE);
		flag &= ~FLAG_D12PD_BRUSH_DIRTY;

	}
	if (flag & FLAG_D12PD_MESH_DIRTY)
	{
		TimeCheck("Batch upload begin");
		if (cMeshGroup.meshs.size() != 0)
		{
			int ind = 0, vex = 0;
			for (auto& i : cMeshGroup.meshs)
			{
				ind += i->idxCount;
				vex += i->vertCount;
			}
			cMeshGroup.uploads[0] = CreateUploadBuffer(ind * sizeof(*CoreMesh::indices));
			cMeshGroup.uploads[1] = CreateUploadBuffer(vex * sizeof(*CoreMesh::vertices));
			cMeshGroup.uploads[2] = CreateUploadBuffer(vex * sizeof(*CoreMesh::uvs));
			cMeshGroup.uploads[3] = CreateUploadBuffer(vex * sizeof(*CoreMesh::pnbs));
			ind = 0; vex = 0;
#define arrcpyi(dst,src,di,sz) memcpy( (( (decltype(src))dst ) + (di) ) ,(src),sizeof(*src)*(sz))
			for (auto i : cMeshGroup.meshs)
			{
				arrcpyi(cMeshGroup.uploads[0].data, i->indices, ind, i->idxCount);
				arrcpyi(cMeshGroup.uploads[1].data, i->vertices, vex, i->vertCount);
				arrcpyi(cMeshGroup.uploads[2].data, i->uvs, vex, i->vertCount);
				arrcpyi(cMeshGroup.uploads[3].data, i->pnbs, vex, i->vertCount);

				ind += i->idxCount;
				vex += i->vertCount;
			}
			cMeshGroup.resource[0] = MakeDefaultBuffer(cMeshGroup.uploads[0].gpuBuffer, ind * sizeof(*CoreMesh::indices));
			cMeshGroup.resource[1] = MakeDefaultBuffer(cMeshGroup.uploads[1].gpuBuffer, vex * sizeof(*CoreMesh::vertices));
			cMeshGroup.resource[2] = MakeDefaultBuffer(cMeshGroup.uploads[2].gpuBuffer, vex * sizeof(*CoreMesh::uvs));
			cMeshGroup.resource[3] = MakeDefaultBuffer(cMeshGroup.uploads[3].gpuBuffer, vex * sizeof(*CoreMesh::pnbs));

			ind = 0; vex = 0;
			for (auto i : cMeshGroup.meshs)
			{
				i->Uploaded(cMeshGroup.resource, ind, vex);

				ind += i->idxCount;
				vex += i->vertCount;
			}


			cMeshGroup.uploads[0].gpuBuffer->Unmap(0, nullptr);
			cMeshGroup.uploads[1].gpuBuffer->Unmap(0, nullptr);
			cMeshGroup.uploads[2].gpuBuffer->Unmap(0, nullptr);
			cMeshGroup.uploads[3].gpuBuffer->Unmap(0, nullptr);
			cMeshGroup.uploads[0].data = nullptr;
			cMeshGroup.uploads[1].data = nullptr;
			cMeshGroup.uploads[2].data = nullptr;
			cMeshGroup.uploads[3].data = nullptr;
		}
		TimeCheck("Batch upload end");
		TimeCheckSum();
	}
	//flag = 0;
}
void D3D12PaintDevice::PostFlush()
{
	if (flag & FLAG_D12PD_MESH_DIRTY)
	{
		for (auto& i : cMeshGroup.uploads)
		{
			i.gpuBuffer->Release();
		}
		meshGroups.push_back(cMeshGroup);
		cMeshGroup.meshs.clear();
		flag &= ~FLAG_D12PD_MESH_DIRTY;
	}
}
void D3D12PaintDevice::UploadMesh(CoreMesh* m)
{
	flag |= FLAG_D12PD_MESH_DIRTY;
	cMeshGroup.meshs.push_back(m);
}
void D3D12PaintDevice::DrawSVG(RLL::ISVG* svg)
{
	D3D12SVG* s = (D3D12SVG*)svg;
	CoreMesh* m = s->mesh;
	//cmdList->DrawIndexedInstanced();
	NOIMPL;
}

void D3D12GeometryBuilder::Begin(Vector2 p)
{
	if (opening)
	{
		SetError("Can not end a non ended GeometryBuilder.");
		return;
	}
	o = p;
	opening = true;
	points.push_back(o);
}
void D3D12GeometryBuilder::End(bool close)
{
	if (!opening)
	{
		SetError("Can not end a non opened GeometryBuilder.");
		return;
	}
	if (cPath.curves.size() == 0)
	{
		SetError("Empty GeometryBuilder. No path built.");
		return;
	}
	if (close)
	{
		LineTo(cPath.curves[0].begin);
	}
	opening = false;
	cPath.closed = close;
	paths.push_back(cPath);
	cPath.curves.clear();
}

void D3D12GeometryBuilder::LineTo(Math3D::Vector2 to)
{
	D3D12GPUCurve c;
	c.begin = o;
	c.control = (o + to) * 0.5f;
	c.end = to;
	if ((o - to).SqurMagnitude() == 0)
		return;
	cPath.curves.push_back(c);
	points.push_back(to);
	o = to;
}
void D3D12GeometryBuilder::QuadraticTo(Math3D::Vector2 control, Math3D::Vector2 to)
{
	D3D12GPUCurve c;
	c.begin = o;
	c.control = control;
	c.end = to;

	cPath.curves.push_back(c);
	points.push_back(control);
	points.push_back(to);
	o = to;

}
void D3D12GeometryBuilder::CubicTo(Math3D::Vector2 c1, Math3D::Vector2 c2, Math3D::Vector2 e)
{
	int segs = 8;
	Vector2 b = o;
	for (int i = 0; i < segs; i++)
	{
		float t = float(i + 1) / float(segs);
		float t_s = float(i) / float(segs);
		float t_m = ((float)i + 0.5f) / float(segs);
		auto s = util::CubicBezier(t_s, b, c1, c2, e);
		auto p = util::CubicBezier(t, b, c1, c2, e);
		auto m = util::CubicBezier(t_m, b, c1, c2, e);
		auto l_m = (s + p) * 0.5;

		QuadraticTo(m * 2 - l_m, p);
		//LineTo(p);
	}
	return;
}
void D3D12GeometryBuilder::Ellipse(Math3D::Vector2 center, Math3D::Vector2 radius, bool inv)
{
	if (opening)
	{
		SetError("Can not build a closed shape on a non closed builder.");
		return;
	}
	if (radius.x == radius.y)
	{
		int segs = ELLIPSE_QUALITY;
		float rad = radius.x;
		float rat = 2 - cosf(3.1415926 / segs);
		Begin(center + Vector2(rad, 0));
		for (int i = 0; i < segs; i++)
		{
			float r1 = 2.f * 3.1415926f / (float)segs * -((float)i + 0.5f);
			float r2 = 2.f * 3.1415926f / (float)segs * -((float)i + 1.f);
			if (!inv)
			{
				r1 = 2.f * 3.1415926f / (float)segs * ((float)i + 0.5f);
				r2 = 2.f * 3.1415926f / (float)segs * ((float)i + 1.f);
			}
			QuadraticTo(
				center + Vector2(cosf(r1) * rad * rat, sinf(r1) * rad * rat),
				center + Vector2(cosf(r2) * rad, sinf(r2) * rad));
		}
	}
	else
	{
		int segs = ELLIPSE_QUALITY;
		for (int i = 0; i < segs; i++)
		{
			float a0 = 2.f * 3.1415926f / (float)segs * -(float)i;
			float a1 = 2.f * 3.1415926f / (float)segs * -((float)i + 1.f);
			if (!inv)
			{
				a0 = 2.f * 3.1415926f / (float)segs * (float)i;
				a1 = 2.f * 3.1415926f / (float)segs * ((float)i + 1.f);

			}
			//#define ellr(a,r) (sqrtf(r.x*cosf(a) * r.x*cosf(a) + r.y*r.y*sinf(a)*sinf(a)))
			auto tb = util::EllipseTangent(radius, a0);
			auto te = util::EllipseTangent(radius, a1);
			auto pb = util::Ellipse(radius, a0);
			auto pe = util::Ellipse(radius, a1);
			Vector2 c;
			util::TrianglePPTT(pb, pe, tb, te, c);
			if (i == 0)
			{
				Begin(center + pb);
			}

			QuadraticTo(
				center + c,
				center + pe);
			//LineTo(center + Vector2(cosf(a2) * radius.x, sinf(a2) * radius.y));
		}

	}
	End(true);

}
void D3D12GeometryBuilder::Rectangle(Math3D::Vector2 lt, Math3D::Vector2 rb, bool inv)
{
	lt.x = min(lt.x, rb.x);
	lt.y = min(lt.y, rb.y);
	rb.x = max(lt.x, rb.x);
	rb.y = max(lt.y, rb.y);
	if (opening)
	{
		SetError("Can not build a closed shape on a non closed builder.");
		return;
	}
	Begin(lt);
	if (inv)
	{
		LineTo({ lt.x,rb.y });
		LineTo({ rb.x,rb.y });
		LineTo({ rb.x,lt.y });
		LineTo({ lt.x,lt.y });
	}
	else
	{
		LineTo({ rb.x,lt.y });
		LineTo({ rb.x,rb.y });
		LineTo({ lt.x,rb.y });
		LineTo({ lt.x,lt.y });
	}
	End(true);
}
void D3D12GeometryBuilder::Triangle(Math3D::Vector2 p0, Math3D::Vector2 p1, Math3D::Vector2 p2, bool inv)
{
	if (opening)
	{
		SetError("Can not build a closed shape on a non closed builder.");
		return;
	}
	Begin(p0);
	if (inv)
	{
		LineTo(p2);
		LineTo(p1);
	}
	else
	{
		LineTo(p1);
		LineTo(p2);
	}
	End(true);
}
D3D12GPUCurve getEllipseArc(Vector2 radius, float from_rad, float to_rad)
{
	float a0 = from_rad;
	float a1 = to_rad;
	auto tb = util::EllipseTangent(radius, a0);
	auto te = util::EllipseTangent(radius, a1);
	auto pb = util::Ellipse(radius, a0);
	auto pe = util::Ellipse(radius, a1);
	Vector2 c;
	util::TrianglePPTT(pb, pe, tb, te, c);

	D3D12GPUCurve curve;
	curve.begin = pb;
	curve.control = c;
	curve.end = pe;
	return curve;
}
void D3D12GeometryBuilder::RoundRectangle(Math3D::Vector2 lt, Math3D::Vector2 rb, Math3D::Vector2 r, bool inv)
{
	if (opening)
	{
		SetError("Can not build a closed shape on a non closed builder.");
		return;
	}
	lt.x = min(lt.x, rb.x);
	lt.y = min(lt.y, rb.y);
	rb.x = max(lt.x, rb.x);
	rb.y = max(lt.y, rb.y);

	int segs = ELLIPSE_QUALITY / 4;
	float d = 2 * 3.1415926f / segs / 4.f;
	float b;
	if (!inv)
	{
		Begin({ lt.x,lt.y + r.y });
		b = 2 * 3.1415926f * 0.5f;
		for (int i = 1; i <= segs; i++)
		{
			auto c = getEllipseArc(r, b + d * i - d, b + d * i);
			QuadraticTo(c.control + Vector2(r.x, r.y) + lt, c.end + Vector2(r.x, r.y) + lt);
		}
		LineTo({ rb.x - r.x,lt.y });
		b = 2 * 3.1415926f * 0.75f;
		for (int i = 1; i <= segs; i++)
		{
			auto c = getEllipseArc(r, b + d * i - d, b + d * i);
			QuadraticTo(c.control + Vector2(-r.x, r.y) + Vector2(rb.x, lt.y), c.end + Vector2(-r.x, r.y) + Vector2(rb.x, lt.y));
		}
		LineTo({ rb.x ,rb.y - r.y });
		b = 2 * 3.1415926f * 1.f;
		for (int i = 1; i <= segs; i++)
		{
			auto c = getEllipseArc(r, b + d * i - d, b + d * i);
			QuadraticTo(c.control + Vector2(-r.x, -r.y) + rb, c.end + Vector2(-r.x, -r.y) + rb);
		}
		LineTo({ lt.x + r.x ,rb.y });
		b = 2 * 3.1415926f * 0.25f;
		for (int i = 1; i <= segs; i++)
		{
			auto c = getEllipseArc(r, b + d * i - d, b + d * i);
			QuadraticTo(c.control + Vector2(r.x, -r.y) + Vector2(lt.x, rb.y), c.end + Vector2(r.x, -r.y) + Vector2(lt.x, rb.y));
		}
		LineTo({ lt.x  ,lt.y + r.y });
	}
	else
	{
		Begin({ lt.x + r.x, lt.y });
		b = 2 * 3.1415926f * 0.75f;
		for (int i = 1; i <= segs; i++) //lt
		{
			auto c = getEllipseArc(r, b - d * i + d, b - d * i);
			QuadraticTo(c.control + Vector2(r.x, r.y) + lt, c.end + Vector2(r.x, r.y) + lt);
		}
		LineTo({ lt.x , rb.y - r.y });

		b = 2 * 3.1415926f * 0.5f;
		for (int i = 1; i <= segs; i++) //lb
		{
			auto c = getEllipseArc(r, b - d * i + d, b - d * i);
			QuadraticTo(c.control + Vector2(r.x, -r.y) + Vector2(lt.x, rb.y), c.end + Vector2(r.x, -r.y) + Vector2(lt.x, rb.y));
		}
		LineTo({ rb.x - r.x ,rb.y });

		b = 2 * 3.1415926f * 0.25f;
		for (int i = 1; i <= segs; i++) //rb
		{
			auto c = getEllipseArc(r, b - d * i + d, b - d * i);
			QuadraticTo(c.control + Vector2(-r.x, -r.y) + rb, c.end + Vector2(-r.x, -r.y) + rb);
		}
		LineTo({ rb.x  ,lt.y + r.y });

		b = 2 * 3.1415926f * 1.0f;
		for (int i = 1; i <= segs; i++) //rt
		{
			auto c = getEllipseArc(r, b - d * i + d, b - d * i);
			QuadraticTo(c.control + Vector2(-r.x, r.y) + Vector2(rb.x, lt.y), c.end + Vector2(-r.x, r.y) + Vector2(rb.x, lt.y));
		}
		LineTo({ lt.x + r.x, lt.y });

	}
	End(true);
}
void D3D12GeometryBuilder::ArcTo(Math3D::Vector2 center, Math3D::Vector2 radius, float begin_rad, float end_rad, ARC_TYPE t)
{
	//begin_rad = Math3D::RadRange2Pi(begin_rad);
	//end_rad = Math3D::RadRange2Pi(end_rad);
	int segs = fabsf((end_rad - begin_rad) - 0.0001f) / Math3D::PI / 2 * ELLIPSE_QUALITY + 1;
	float d = (end_rad - begin_rad) / segs;
	for (int i = 0; i < segs; i++)
	{
		auto c = getEllipseArc(radius, begin_rad + i * d, begin_rad + i * d + d);
		if (i == 0)
		{
			LineTo({ center.x + cosf(begin_rad) * radius.x,center.y + sinf(begin_rad) * radius.y });
		}
		QuadraticTo(c.control + center, c.end + center);
	}
}

IGeometry* D3D12GeometryBuilder::Fill(Math3D::Matrix4x4* tf)
{
	if (opening)
	{
		SetError("Filling a non ended GeometryBuilder, auto end with close (End(true);).");
		End(true);
	}
	Matrix4x4 transform = Matrix4x4::Identity();
	if (tf)
		transform = *tf;

	if (!paths.size())
	{
		SetError("Empty GeometryBuilder. No path built.");
		return nullptr;
	}
	if (points.size() < 3)
	{
		SetError("Empty Area. No pixels will be painted.");
		return nullptr;
	}
	D3D12Geometry* ifGeom = new D3D12Geometry(device);
	ifGeom->mesh = new D3D12GeometryMesh();
	D3D12GPUPath p;
	PathBuffer* buffer = &device->paths;
	vector<D3D12GPUCurve> hBand;
	vector<D3D12GPUCurve> vBand;

	for (auto& i : paths)
	{
		for (auto& c : i.curves)
		{
			if (!c.IsHorizontal())
			{
				hBand.push_back(c);
			}
			if (!c.IsVertical())
			{
				vBand.push_back(c);
			}
		}
	}
	sort(hBand.begin(), hBand.end(), [](D3D12GPUCurve& l, D3D12GPUCurve& r) {return l.Max().x > r.Max().x; });
	sort(vBand.begin(), vBand.end(), [](D3D12GPUCurve& l, D3D12GPUCurve& r) {return l.Max().y > r.Max().y; });
	p.hi = buffer->curves.size();
	p.hl = hBand.size();
	p.vi = p.hi + p.hl;
	p.vl = vBand.size();
	ifGeom->pathID = buffer->paths.size();
	buffer->paths.push_back(p);
	ifGeom->path = &buffer->paths[ifGeom->pathID];
	for (auto& c : hBand)
	{
		D3D12GPUCurve fc;
		fc.begin = c.begin * transform;
		fc.control = c.control * transform;
		fc.end = c.end * transform;
		buffer->curves.push_back(fc);
	}
	for (auto& c : vBand)
	{
		D3D12GPUCurve fc;
		fc.begin = c.begin * transform;
		fc.control = c.control * transform;
		fc.end = c.end * transform;
		buffer->curves.push_back(fc);
	}

	D3D12GeometryMesh& cvx = *ifGeom->mesh;
	convexHull(points, cvx.verts);
	simplifyConvex(cvx.verts, 6);
	cvx.MakeIndex();
	cvx.MakeNormal();
	cvx.MakeUV(transform);
	cvx.SetPath(ifGeom->pathID);
	return ifGeom;
}
RLL::StrokeStyle _internal_default_stroke_style;
void StrokeQBezier(IGeometryBuilder* bd, D3D12GPUCurve& i, float hfstroke)
{
	Vector2 b[2];
	Vector2 c[2];
	Vector2 e[2];
	auto segs = util::QBezierExpand(i.begin, i.control, i.end, hfstroke,
		b, c, e
	);

	Vector2 bi[2];
	Vector2 ci[2];
	Vector2 ei[2];
	util::QBezierExpand(i.end, i.control, i.begin, hfstroke,
		bi, ci, ei
	);
	if (segs == 1)
	{
		bd->Begin(b[0]);
		bd->QuadraticTo(c[0], e[0]);
		bd->LineTo(bi[0]);
		bd->QuadraticTo(ci[0], ei[0]);
		bd->End(true);
		//std::cout << "QBez:\t" << ovec(b[0]) << "\t" << ovec(c[0]) << "\t" << ovec(e[0]) << std::endl;
		//std::cout << "QBez:\t" << ovec(bi[0]) << "\t" << ovec(ci[0]) << "\t" << ovec(ei[0]) << std::endl;
	}
	else if (segs == 2)
	{
		bd->Begin(b[0]);
		bd->QuadraticTo(c[0], e[0]);
		bd->QuadraticTo(c[1], e[1]);
		bd->LineTo(bi[0]);
		bd->QuadraticTo(ci[0], ei[0]);
		bd->QuadraticTo(ci[1], ei[1]);
		bd->End(true);
	}
	else
	{
		NOIMPL;
	}

}
RLL::IGeometry* D3D12GeometryBuilder::Stroke(float stroke, RLL::StrokeStyle* type, Math3D::Matrix4x4* tf)
{
	//return nullptr;
	if (type == nullptr)
	{
		type = &_internal_default_stroke_style;
		//type->joint = JOINT_TYPE::MITER;
		//type->cap = CAP_TYPE::ROUND;
	}
	if (opening)
	{
		SetError("Stroking a non ended GeometryBuilder, auto end without close (End(false);).");
		End(false);
	}
	if (!paths.size())
	{
		SetError("Empty GeometryBuilder. No path built.");
		return nullptr;
	}
	stroke /= 2;
	IGeometryBuilder* bd = device->CreateGeometryBuilder();
#define ovec(v) (v).x << ", " <<(v).y 

	for (auto p : paths)
	{
		std::vector<D3D12GPUCurve> pos;
		std::vector<D3D12GPUCurve> neg;
		for (auto n = 0; n < p.curves.size(); n++)
		{
			auto& i = p.curves[n];
			Vector2 b[2];
			Vector2 c[2];
			Vector2 e[2];
			auto segs = util::QBezierExpand(i.begin, i.control, i.end, stroke,
				b, c, e
			);

			Vector2 bi[2];
			Vector2 ci[2];
			Vector2 ei[2];
			util::QBezierExpand(i.end, i.control, i.begin, stroke,
				bi, ci, ei
			);
			if (segs == 1)
			{
				pos.push_back({ b[0],c[0],e[0] });
				neg.push_back({ bi[0],ci[0],ei[0] });

			}
			else if (segs == 2)
			{
				pos.push_back({ b[0],c[0],e[0] });
				pos.push_back({ b[1],c[1],e[1] });
				neg.push_back({ bi[1],ci[1],ei[1] });
				neg.push_back({ bi[0],ci[0],ei[0] });
			}
			else
			{
				NOIMPL;
			}
		}
		switch (type->joint)
		{
		case JOINT_TYPE::FLAT:
		{
			for (auto n = 0; n < p.curves.size(); n++)
			{
				StrokeQBezier(bd, p.curves[n], stroke);
				//i.begin += (i.control - i.begin).Normalized() * stroke;
				//i.end += (i.control - i.end).Normalized() * stroke;
			}
			break;
		}
		case JOINT_TYPE::MITER:
		{
			bd->Begin(pos[0].begin);
			for (int n = 0; n < pos.size(); n++)
			{
				if (n > 0)
				{
					auto tf = pos[n - 1].end - pos[n - 1].control;
					auto tt = pos[n].control - pos[n].begin;
					auto cro = Vector2::Cross(tf, tt);
					float d = (pos[n - 1].end - pos[n].begin).SqurMagnitude();
					if (d > 1e-16)
					{
						if (cro > 0)
						{
							Vector2 mitp;
							util::TrianglePPTT(pos[n - 1].end, pos[n].begin, tf, tt, mitp);
							bd->LineTo(mitp);
						}
						bd->LineTo(pos[n].begin);
						//std::cout << "insert line pos" << std::endl;
					}
				}
				bd->QuadraticTo(pos[n].control, pos[n].end);
			}
			auto ni = neg.size() - 1;
			if (p.closed)
			{
				auto pi = pos.size() - 1;
				auto tf = pos[pi].end - pos[pi].control;
				auto tt = pos[0].control - pos[0].begin;
				auto cro = Vector2::Cross(tf, tt);
				if (cro > 0)
				{
					Vector2 mitp;
					util::TrianglePPTT(pos[pi].end, pos[0].begin, tf, tt, mitp);
					bd->LineTo(mitp);
				}
				bd->End(true);
				bd->Begin(neg[ni].begin);
			}
			else
			{
				bd->LineTo(neg[ni].begin);
			}
			for (int n = ni; n >= 0; n--)
			{
				if (n < ni)
				{
					auto tf = neg[n + 1].end - neg[n + 1].control;
					auto tt = neg[n].control - neg[n].begin;
					auto cro = Vector2::Cross(tf, tt);
					//std::cout << cro << "\n";
					float d = (neg[n + 1].end - neg[n].begin).SqurMagnitude();
					if (d > 1e-16)
					{
						if (cro > 0)
						{
							Vector2 mitp;
							util::TrianglePPTT(neg[n + 1].end, neg[n].begin, tf, tt, mitp);
							bd->LineTo(mitp);
						}
						bd->LineTo(neg[n].begin);
						//std::cout << "insert line neg" << std::endl;
					}
				}
				bd->QuadraticTo(neg[n].control, neg[n].end);
			}
			if (p.closed)
			{
				auto tf = neg[0].end - neg[0].control;
				auto tt = neg[ni].control - neg[ni].begin;
				auto cro = Vector2::Cross(tf, tt);
				if (cro > 0)
				{
					Vector2 mitp;
					util::TrianglePPTT(neg[0].end, neg[ni].begin, tf, tt, mitp);
					bd->LineTo(mitp);
				}
			}
			bd->End(true);
			break;
		}
		case JOINT_TYPE::BEVEL:
		{
			bd->Begin(pos[0].begin);
			for (int n = 0; n < pos.size(); n++)
			{
				if (n > 0)
				{
					float d = (pos[n - 1].end - pos[n].begin).SqurMagnitude();
					if (d > 1e-16)
					{
						bd->LineTo(pos[n].begin);
						//std::cout << "insert line pos" << std::endl;
					}
				}
				bd->QuadraticTo(pos[n].control, pos[n].end);
			}
			auto ni = neg.size() - 1;
			if (p.closed)
			{
				bd->End(true);
				bd->Begin(neg[ni].begin);
			}
			else
			{
				bd->LineTo(neg[ni].begin);
			}
			for (int n = ni; n >= 0; n--)
			{
				if (n < ni)
				{
					float d = (neg[n + 1].end - neg[n].begin).SqurMagnitude();
					if (d > 1e-16)
					{
						bd->LineTo(neg[n].begin);
						//std::cout << "insert line neg" << std::endl;
					}
				}
				bd->QuadraticTo(neg[n].control, neg[n].end);
			}

			bd->End(true);

			break;
		}
		case JOINT_TYPE::ROUND:
		{
			bd->Begin(pos[0].begin);
			for (int n = 0; n < pos.size(); n++)
			{
				if (n > 0)
				{
					auto tf = pos[n - 1].end - pos[n - 1].control;
					auto tt = pos[n].control - pos[n].begin;
					auto cro = Vector2::Cross(tf, tt);
					float d = (pos[n - 1].end - pos[n].begin).SqurMagnitude();
					if (d > 1e-16)
					{
						if (cro > 0)
						{
							auto& pe = pos[n - 1].end;
							auto& pb = pos[n].begin;
							auto norm = -util::Normal(tf).Normalized();
							auto cent = pe + norm * stroke;
							auto start_ang = atan2f(-norm.y, -norm.x);
							auto bnorm = pb - cent;
							auto end_ang = atan2f(bnorm.y, bnorm.x);
							bd->LineTo(pos[n].begin);
							bd->End(false);
							bd->Ellipse(cent, stroke);
							bd->Begin(pos[n].begin);//bd->ArcTo(cent, stroke, start_ang, end_ang);
						}
						else
						{
							bd->LineTo(pos[n].begin);
						}
						//std::cout << "insert line pos" << std::endl;
					}
				}
				bd->QuadraticTo(pos[n].control, pos[n].end);
			}
			auto ni = neg.size() - 1;
			if (p.closed)
			{
				auto pi = pos.size() - 1;
				auto tf = pos[pi].end - pos[pi].control;
				auto tt = pos[0].control - pos[0].begin;
				auto cro = Vector2::Cross(tf, tt);
				if (cro > 0)
				{
					auto& pe = pos[pi].end;
					auto& pb = pos[0].begin;
					auto norm = -util::Normal(tf).Normalized();
					auto cent = pe + norm * stroke;
					auto start_ang = atan2f(-norm.y, -norm.x);
					auto bnorm = pb - cent;
					auto end_ang = atan2f(bnorm.y, bnorm.x);
					bd->LineTo(pos[0].begin);
					bd->End(false);
					bd->Ellipse(cent, stroke);
					//bd->ArcTo(cent, stroke, start_ang, end_ang);
				}
				else
					bd->End(true);
				bd->Begin(neg[ni].begin);
			}
			else
			{
				bd->LineTo(neg[ni].begin);
			}
			for (int n = ni; n >= 0; n--)
			{
				if (n < ni)
				{
					auto tf = neg[n + 1].end - neg[n + 1].control;
					auto tt = neg[n].control - neg[n].begin;
					auto cro = Vector2::Cross(tf, tt);
					//std::cout << cro << "\n";
					float d = (neg[n + 1].end - neg[n].begin).SqurMagnitude();
					if (d > 1e-16)
					{
						if (cro > 0)
						{
							auto& pe = neg[n + 1].end;
							auto& pb = neg[n].begin;
							auto norm = -util::Normal(tf).Normalized();
							auto cent = pe + norm * stroke;
							auto start_ang = atan2f(-norm.y, -norm.x);
							auto bnorm = pb - cent;
							auto end_ang = atan2f(bnorm.y, bnorm.x);
							bd->LineTo(neg[n].begin);
							bd->End(false);
							bd->Ellipse(cent, stroke);
							bd->Begin(neg[n].begin);
						}
						else
							bd->LineTo(neg[n].begin);
						//std::cout << "insert line neg" << std::endl;
					}
				}
				bd->QuadraticTo(neg[n].control, neg[n].end);
			}
			if (p.closed)
			{
				bd->LineTo(neg[ni].begin);
				auto tf = neg[0].end - pos[0].control;
				auto tt = neg[ni].control - neg[ni].begin;
				auto cro = Vector2::Cross(tf, tt);
				if (cro > 0)
				{
					auto& pe = neg[0].end;
					auto& pb = neg[ni].begin;
					auto norm = -util::Normal(tf).Normalized();
					auto cent = pe + norm * stroke;
					auto start_ang = atan2f(-norm.y, -norm.x);
					auto bnorm = pb - cent;
					auto end_ang = atan2f(bnorm.y, bnorm.x);
					bd->End(false);
					bd->Ellipse(cent, stroke);
					//bd->ArcTo(cent, stroke, start_ang, end_ang);
				}
				else
					bd->End(false);
			}
			else
			{
				bd->LineTo(pos[0].begin);
				bd->End(false);
			}
			break;
		}
		}
		if (!p.closed)
		{
			auto psi = p.curves.size() - 1;
			auto h = p.curves[0];
			auto t = p.curves[psi];
			h.end = p.curves[0].begin;
			h.begin = h.end + (h.end - h.control).Normalized() * stroke;
			h.control = (h.begin + h.end) * 0.5f;
			t.begin = p.curves[psi].end;
			t.end = t.begin + (t.begin - t.control).Normalized() * stroke;
			t.control = (t.begin + t.end) * 0.5f;
			if (type->cap == CAP_TYPE::SQUARE)
			{
				StrokeQBezier(bd, h, stroke);
				StrokeQBezier(bd, t, stroke);
			}
			else if (type->cap == CAP_TYPE::TRIANGLE)
			{
				auto hnorm = util::Normal(h.end - h.begin).Normalized() * stroke;
				auto tnorm = util::Normal(t.end - t.begin).Normalized() * stroke;

				bd->Triangle(h.begin, h.end + hnorm, h.end - hnorm);
				bd->Triangle(t.begin + tnorm, t.end, t.begin - tnorm);
			}
			else if (type->cap == CAP_TYPE::ROUND)
			{
				bd->Ellipse(h.end, stroke);
				bd->Ellipse(t.begin, stroke);
			}
		}
	}


	auto res = bd->Fill(tf);
	bd->Release();
	return res;
};

void D3D12GeometryBuilder::Dispose()
{
	delete this;
}
void D3D12GeometryBuilder::Reset()
{
	o = { 0,0 };
	paths.clear();
	points.clear();
	opening = false;
}

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
	TimeCheck("Commit begin.");

	vector<Vector2> uv;
	vector<Vector3> pos;
	vector<Vector3> pnb;
	vector<short> ind;
	int vb = 0, ib = 0;
	uv.reserve(512);
	pos.reserve(512);
	pnb.reserve(512);
	ind.reserve(512);
	for (auto& i : layers)
	{
		switch (i.type)
		{
		case Layer::TYPE::TYPE_GEOMETRY:
		{
			D3D12GeometryMesh& m = *i.geometry->mesh;
			for (auto& n : m.verts)
			{
				pos.push_back(n * i.transform);
			}
			for (auto& n : m.uv)
			{
				uv.push_back(n );
			}
			for (auto& n : m.indices)
				ind.push_back(n + vb);
			for (auto& n : m.path_norm)
			{
				if (i.brush)
					pnb.push_back({ n.x,n.y,(float)i.brush->id });
				else
					pnb.push_back({ n.x,n.y,0.f });

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

	TimeCheck("CPU done.");
	auto svg = new D3D12SVG(device);
	svg->id = device->paths.meshes.size();
	device->paths.meshes.push_back(new CoreMesh());
	auto corem = device->paths.meshes[svg->id];
	svg->mesh = corem;
	corem->vertCount = uv.size();
	corem->idxCount = ind.size();
	corem->indices = new unsigned short[corem->idxCount];
	corem->uvs = new Vector2[corem->vertCount];
	corem->pnbs = new Vector3[corem->vertCount];
	corem->vertices = new Vector3[corem->vertCount];
#define vecize(x) (sizeof(x[0]) * x.size())
	memcpy(corem->indices, &ind[0], vecize(ind));
	memcpy(corem->uvs, &uv[0], vecize(uv));
	memcpy(corem->pnbs, &pnb[0], vecize(pnb));
	memcpy(corem->vertices, &pos[0], vecize(pos));
	TimeCheck("Copy done.");

	//corem.UploadToGPUMemory
	TimeCheck("Upload begin.");
	corem->Upload(device);
	device->flag |= FLAG_D12PD_PATH_DIRTY | FLAG_D12PD_CURVE_DIRTY;
	TimeCheck("Upload done.");
	TimeCheckSum();
	return svg;
}
void D3D12SVGBuilder::Dispose()
{
	delete this;
}

void D3D12Brush::Dispose()
{
	if (brush)
		brush->pad = 1;
	delete this;
}

void D3D12Geometry::Dispose()
{
	delete mesh;
	NOIMPL;
}

void D3D12SVG::Dispose()
{
	delete mesh;
	device->paths.meshes[id] = nullptr;
	id = -1;
	delete this;
}