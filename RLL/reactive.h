#pragma once
#include <functional>
#include <vector>
#include "delegate.h"
#define reactive
template<typename T>
class Reactive
{
public:
	T* value;
	using WATCHER = void(T old, T nov);

private:
	int* ref;
	delegate<WATCHER>* listener_ptr;
	Reactive<T>* weak_target = nullptr;
	void emit(T old, T nov)
	{
		(*listener_ptr)(std::move(old), std::move(*value));
	}
public:
	Reactive()
	{
		value = new T();
		ref = new int(1);
		listener_ptr = new delegate<WATCHER>();
	}
	template <class...CTOR_VALS>
	Reactive(CTOR_VALS&&... vals)
	{
		value = new T(vals...);
		ref = new int(1);
		listener_ptr = new delegate<WATCHER>();
	}
	Reactive(const T& nov)//copy
	{
		value = new T();
		ref = new int(1);
		listener_ptr = new delegate<WATCHER>();
		*value = nov;
	}
	Reactive(const Reactive<T>& nov)//copy
	{
		ref = nov.ref;
		value = nov.value;
		listener_ptr = nov.listener_ptr;

		(*ref)++;
	}
	Reactive(T&& nov)//move
	{

		value = new T(nov);
		ref = new int(1);
		listener_ptr = new delegate<WATCHER>();
	}
	T& operator=(const T& nov)
	{
		T old = *value;
		*value = nov;
		emit(old, nov);
		return *value;
	}
	Reactive<T>& operator=(const Reactive<T>& nov)
	{
		Reactive<T>::~Reactive();
		ref = nov.ref;
		value = nov.value;
		(*ref)++;
		return *this;
	}

	T& operator->()
	{
		return *value;
	}
	delegate<WATCHER>& listener()
	{
		return *listener_ptr;
	}
	~Reactive()
	{
		(*ref)--;
		if (ref <= 0)
		{
			delete ref;
			delete value;
			delete listener_ptr;
		}
		ref = nullptr;
		value = nullptr;
		listener_ptr = nullptr;
	}
};

#define R Reactive