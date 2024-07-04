#pragma once
#include "Alias.h"

#include <iostream>

namespace Gz
{
	class Object
	{
		Atomic<int> _ref_count = 1;
		Object* _parent = nullptr;
		Vector<Object*> _children;
		String _name = "UnnamedObject";
		String _error;
	public:
		Object(Object* parent = nullptr)
		{
			_parent = parent;
			if (parent) parent->_children.push_back(this);
		};
		Object(String name, Object* parent = nullptr) : Object(parent)
		{
			_name = name;
		};
		virtual ~Object()
		{
			if (_ref_count > 0)
			{
				std::cout << "Delete an Object whose reference count is not 0." << this << std::endl;
			}
			for (auto& child : _children)
			{
				child->_parent = nullptr;
				child->Release();
			}
			_children.clear();
		}
		void Aquire() { AddRef(); }
		void AddRef()
		{
			_ref_count++;
		}
		void Release()
		{
			_ref_count--;
			if (_ref_count <= 0)
			{
				//std::cout << "Counter Dispose:\t" << this << std::endl;
				delete this;
			}
		}
		String GetLastError() { return _error; };
	protected:
		void SetError(String e)
		{
			_error = e;
			std::cout << this << ":\t" << _error << std::endl;
		};

	public:
		Object* parent() { return _parent; };
		String name() { return _name; };
		void name(String name) { _name = name; };

	};

}