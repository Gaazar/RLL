#pragma once
#include "Metrics.h"
#include "ThirdParty.h"

#include <iostream>
#include <vector>
namespace RLL
{
	enum EVENT_TYPE
	{
		EVENT_TYPE_MOUSE_MOVE,
		EVENT_TYPE_MOUSE_CLICK,
		EVENT_TYPE_SCROOL,
		EVENT_TYPE_TOUCH,
		EVENT_KEY,
		EVENT_CHAR,
		EVENT_INPUT,
		EVENT_FW_FRAME_SIZE_MOVE
	};
	struct ControlKeys
	{
		int keys;
		bool IsControl()
		{
			return false;
		}
		bool IsShift()
		{
			return false;
		}
		bool IsAlt()
		{
			return false;
		}
		bool IsFn()
		{
			return false;
		}
	};
	union EventArgs
	{
		struct MouseMove
		{
			Math3D::Vector2 pos;
			Math3D::Vector2 delta;
		} mouseMove;
		struct MouseClick
		{
			short button;
			float pressedTime;
			ControlKeys keys;
		} mouseClick;
		struct Scrool
		{
			float delta;
			bool direct;
			ControlKeys keys;
		};
		struct Key
		{
			char code;
			bool down;
			bool IsKeyDown()
			{
				return down;
			}
			bool IsKeyUp()
			{
				return !down;
			}
		} key;
		wchar_t character;
		wchar_t* input;
		struct FrameMoveSize
		{
			int flag;
			Math3D::Vector2 pos;
			bool IsMoving()
			{
				return flag & 0x1;
			}
			bool IsMoved()
			{
				return flag & 0x2;
			}
			bool IsSizing()
			{
				return flag & 0x4;
			}
			bool IsSized()
			{
				return flag & 0x8;
			}
			bool IsFullscreen()
			{
				return flag & 0x16;
			}
			bool IsRestord()
			{
				return flag & 0x32;
			}
			bool IsMinized()
			{
				return flag & 0x64;
			}
		} frameMove;
	};
	class IFrame;
	class IPaintDevice;
	class ISVGBuilder;
	class ISVG;
	void Initiate(int flag = 0);
	int GetFlags();
	wchar_t* GetLocale();
	Math3D::Vector2 GetScale();
	IFrame* CreateFrame(IFrame* parent, Math3D::Vector2 size, Math3D::Vector2 pos);

	IPaintDevice* CreatePaintDevice();


	class View
	{
		void* EventProcess(EVENT_TYPE, EventArgs)
		{
			return nullptr;
		}
	};
	class IFrame
	{
	public:
		virtual void Show() = 0;
		virtual void Run() = 0;
	};

	class IBase
	{
		int _ref_count = 0;
		virtual void Dispose() = 0;
		char* error = nullptr;
	protected:
		void SetError(char* e)
		{
			error = e;
			std::cout << this << ":\t" << error << std::endl;
		};
	public:
		void Aquire() { AddRef(); }
		void AddRef()
		{
			_ref_count++;
		}
		void Release()
		{
			_ref_count--;
			if (_ref_count < 0)
			{
				//std::cout << "Counter Dispose:\t" << this << std::endl;
				Dispose();
			}
		}
		char* GetLastError() { return error; };
	};
	struct FlowOut
	{
		RLL::Size size;
		RLL::Size dynRation;
	};
	class RenderList
	{
	public:
		void operator() (Math3D::Matrix4x4 globalTransform, RLL::ISVG* renderItem, int zIndex = 0)
		{
			join(globalTransform, renderItem, zIndex);
		};
		void join(Math3D::Matrix4x4 globalTransform, RLL::ISVG* renderItem, int zIndex = 0)
		{
		}
	};
	class IElement
	{
	public:
		//return true if its size is control by parent
		virtual bool Flow(ElementProps& parentProps, RLL::FlowOut& out_Size) { return false; };
		//virtual RLL::Size Size(ElementProps* parentProps) = 0;
		virtual LANG_DIRECTION Direction() = 0;
		virtual UScriptCode Script() { return USCRIPT_COMMON; };
		virtual void Place(Math3D::Matrix4x4* parentTransform, RenderList renderList) = 0;
	};
	struct ElementData
	{
		IElement* element = nullptr;
		RLL::FlowOut out;
		RLL::Size finalSize;
		float dynSize = 0;
		LANG_DIRECTION direction;

	};

	class BlockLayout //: public IElement
	{
	private:
		struct BIDIPart
		{
			std::vector<ElementData> parts;
			LANG_DIRECTION direction = LANG_DIRECTION_LR;
		};
		struct LinePart
		{
			std::vector<ElementData> parts;
			std::vector<BIDIPart> bidis;
			RLL::Size size;
			RLL::Size dynSizeItgr;
		};

		BlockProps properties;
		BlockMetrics metrics;

		RLL::Size contentSize;
		LANG_DIRECTION direction = LANG_DIRECTION_LR_TB;
		LINE_ALIGN lineAlign = LINE_ALIGN_START;
		PARA_ALIGN paraAlign = PARA_ALIGN_START;

		std::vector<ElementData> elements;
		std::vector<LinePart> lines;

	public:
		RLL::Size Size(ElementProps* parentProps);
		LANG_DIRECTION Direction();
		ScriptCode Script();
		void Place(Math3D::Matrix4x4* parentTransform, RenderList renderList);
		bool Flow(ElementProps& parentProps, RLL::FlowOut& out_Size);
	};


}