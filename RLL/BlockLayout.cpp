#include "Interfaces.h"

using namespace RLL;
using namespace Math3D;
using namespace std;




//if parentProps.size == {0,0} then it means parent is not sized and should skip procedure,
//if this element is dynamic; Then
//set out_Size to its minimal size;
bool BlockLayout::Flow(ElementProps& parentProps, RLL::FlowOut& out_Size)
{
	properties.size = { 0,0 };
	if (parentProps.size.SqurMagnitude() == 0 && properties.isDynamicSize)
	{
		out_Size.size = properties.minSize;
		return true;
	}
	for (auto& i : elements)
	{
		i.out = FlowOut();
		if (i.element->Flow(properties, i.out))
		{
			i.dynSize = true;
		}
	}
	lines.clear();
	LinePart cLine;
	//horizontal only

	//calculate line metrics
	{
		for (auto& i : elements)
		{
			if (cLine.size.x + i.out.size.x > contentSize.x)
			{
				if (cLine.parts.size() > 0)
				{
					RLL::Size maxSize = { contentSize.x,cLine.size.y };
					auto restSize = (maxSize - cLine.size) / cLine.dynSizeItgr;
					if (cLine.dynSizeItgr.x == 0) restSize.x = 0;
					if (cLine.dynSizeItgr.y == 0) restSize.y = 0;

					for (auto& i : cLine.parts)
					{
						i.finalSize = i.out.size + i.out.dynRation * restSize;
					}
					lines.push_back(cLine);
					cLine.parts.clear();
					cLine.size = RLL::Size(0, 0);
					cLine.dynSizeItgr = { 0,0 };
					//continue;
				}
			}
			i.direction = i.element->Direction();
			cLine.parts.push_back(i);
			cLine.size.x += i.out.size.x;
			if (i.dynSize)
				cLine.dynSizeItgr += i.out.dynRation;

			if (i.out.size.y > cLine.size.y)
			{
				cLine.size.y = i.out.size.y;
			}
		}
		if (cLine.parts.size() > 0)
		{
			RLL::Size maxSize = { contentSize.x,cLine.size.y };
			auto restSize = (maxSize - cLine.size) / cLine.dynSizeItgr;
			if (cLine.dynSizeItgr.x == 0) restSize.x = 0;
			if (cLine.dynSizeItgr.y == 0) restSize.y = 0;

			for (auto& i : cLine.parts)
			{
				i.finalSize = i.out.size + i.out.dynRation * restSize;
			}
			
			lines.push_back(cLine);
		}
		//adjust dynamic size element in line

	}

	//bidi lines
	{
		for (auto& l : lines)
		{
			LANG_DIRECTION prevDir = direction;
			//fix two terminal of the line's COMMON_SCRIPT bidi info
			for (auto k = 0; k < l.parts.size(); k++)
			{
				if (l.parts[k].element->Script() == USCRIPT_COMMON)
				{
					l.parts[k].direction = direction;
				}
				else break;
			}
			for (auto k = l.parts.size() - 1; k >= 0; k--)
			{
				if (l.parts[k].element->Script() == USCRIPT_COMMON)
				{
					l.parts[k].direction = direction;
				}
				else break;
			}
			//TODO: combine below two for, can it?
			BIDIPart bdp;
			for (auto i = 0; i < l.parts.size(); i++)
			{
				if (l.parts[i].direction != prevDir)
				{
					if (bdp.parts.size() > 0)
					{
						l.bidis.push_back(bdp);
						bdp.parts.clear();
					}
					bdp.direction = l.parts[i].direction;
				}
				prevDir = l.parts[i].direction;
				bdp.parts.push_back(l.parts[i]);

			}
			if (bdp.parts.size() > 0)
				l.bidis.push_back(bdp);
			bdp.parts.clear();
			l.parts.clear();
			for (auto& i : l.bidis)
			{
				if (i.direction == LANG_DIRECTION_LR)
				{
					for (auto it = i.parts.begin(); it != i.parts.end(); )
					{
						l.parts.push_back(*it);
						++it;
					}

				}
				else if (i.direction == LANG_DIRECTION_RL)
				{
					for (auto it = i.parts.rbegin(); it != i.parts.rend(); )
					{
						l.parts.push_back(*it);
						++it;
					}
				}
			}
		}
	}

	return false;
};

RLL::Size BlockLayout::Size(ElementProps* parentProps)
{
	RLL::Size sz;

	return contentSize;
}
LANG_DIRECTION BlockLayout::Direction()
{
	return direction;
}
ScriptCode BlockLayout::Script()
{
	return USCRIPT_COMMON;
}
void BlockLayout::Place(Math3D::Matrix4x4* parentTransform, RenderList renderList)
{
	Math3D::Vector2 cursor(0,0);
	for (int i = 0; i < lines.size(); i++)
	{
		cursor.y += lines[i].size.y * 1.2f;
		cursor.x = 0;
		for (auto& n : lines[i].parts)
		{
			//n.element->Place(sb, cursor);
			cursor.x += n.finalSize.x;
		}
	}

}
