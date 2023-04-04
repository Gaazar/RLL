#pragma once
#include "Metrics.h"

#include <iostream>
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
		int _ref_count = 1;
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
			if (!_ref_count)
			{
				//std::cout << "Counter Dispose:\t" << this << std::endl;
				Dispose();
			}
		}
		char* GetLastError() { return error; };
	};



}