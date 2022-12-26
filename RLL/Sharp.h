#pragma once
#include <stdint.h>
#include <malloc.h>
#include <memory>
#include <vector>
#include <tuple>
#include <stdio.h>
#include <functional>
namespace Sharp
{
	typedef uint8_t uint8;
	typedef int8_t int8;
	typedef uint16_t uint16;
	typedef int16_t int16;
	typedef uint32_t uint32;
	typedef int32_t int32;
	typedef uint64_t uint64;
	typedef int64_t int64;

	typedef int8 Byte;
	typedef uint8 uByte;
	typedef int16 Short;
	typedef uint16 uShort;
	typedef int32 Int;
	typedef uint32 uInt;
	typedef int64 Long;
	typedef uint64 uLong;


	//UTF-16 Encoding
	class String
	{
	public:
		static String& EmptyString();

	private:
		struct StringView
		{
			wchar_t* str = L"";
			int length = 0;
			~StringView();

		};
		std::shared_ptr<StringView> strview = nullptr;
	public:
		String();
		String(const wchar_t* chars);
		String(const char* chars);
		String(String& str);
		~String();
		int Length();
		String& Append(String s);
		String& Prepend(String s);
		String& Insert(int index, String s);
		String& Replace(String s);
		bool Contains(String s);
		bool StartsWith(String s);
		bool EndsWith(String s);
		String Left(int count);
		String Right(int count);
		std::vector<String>& Split(String spliter);
		bool IsEmpty()
		{
			return !wcscmp(strview->str, EmptyString().strview->str);
		}
		bool IsNull()
		{
			return strview == EmptyString().strview;
		}
		const wchar_t* CString()
		{
			return strview->str;
		}
		wchar_t& operator[] (int index);
	private:
		String(wchar_t* s, int len);
	public:
		String operator+ (String& s);
		static String Format(std::tuple<String, int> fmti, String arg)
		{
			String fmt = std::get<0>(fmti);
			int index = std::get<1>(fmti);
			int left = -1;
			int right = -1;
			for (int i = 0; i < fmt.Length(); i++)
			{
				if (fmt[i] == '\\')
				{
					++i;
					continue;
				}
				else if (fmt[i] == '{')
				{
					left = i;
				}
				else if (fmt[i] == '}')
				{
					if (left != -1)
					{
						right = i;
						int num = wcstol(&fmt[left + 1], nullptr, 10);
						if (num == index)
						{
							String sl = fmt.Left(left);
							String sr = fmt.Right(fmt.Length() - right - 1);
							return sl + arg + sr;
						}
					}
				}

			}
			return fmt;
		}
		static String Format(std::tuple<String, int> fmti, const char* arg)
		{
			return Format({ std::get<0>(fmti),std::get<1>(fmti) }, String(arg));
		}
		static String Format(std::tuple<String, int> fmti, const wchar_t* arg)
		{
			return Format({ std::get<0>(fmti),std::get<1>(fmti) }, String(arg));
		}
		template<typename T>
		static String Format(std::tuple<String, int> fmti, T arg)
		{
			String fmt = std::get<0>(fmti);
			int index = std::get<1>(fmti);
			auto wsarg = std::to_wstring(arg);
			int left = -1;
			int right = -1;
			for (int i = 0; i < fmt.Length(); i++)
			{
				if (fmt[i] == '\\')
				{
					++i;
					continue;
				}
				else if (fmt[i] == '{')
				{
					left = i;
				}
				else if (fmt[i] == '}')
				{
					if (left != -1)
					{
						right = i;
						int num = wcstol(&fmt[left + 1], nullptr, 10);
						if (num == index)
						{
							String sl = fmt.Left(left);
							String sr = fmt.Right(fmt.Length() - right - 1);
							return sl + String(wsarg.c_str()) + sr;
						}
					}
				}

			}
			return fmt;
		}
		template<typename T, typename... U>
		static String Format(std::tuple<String, int> fmti, T arg, U... args)
		{
			auto s = Format({ std::get<0>(fmti),std::get<1>(fmti) }, arg);
			auto r = Format({ s,std::get<1>(fmti) + 1 }, args...);
			return r;
		}
		template<typename... U>
		static String Format(String fmt, U... args)
		{
			return Format({ fmt ,0 }, args...);
		}
	};
	class Exception
	{
	private:
		String reason;
	public:
		Exception();
		Exception(String reason);
		virtual String ToString();
	};
	inline void* Malloc(size_t size)
	{
		void* m = malloc(size);
		if (!m) throw Exception("malloc fault.");
		return m;
	}
	enum class SeekOrigin
	{
		Current = 0,
		Start,
		End
	};
	class Stream
	{
	public:
		virtual bool CanRead() { return false; }
		virtual bool CanWrite() { return false; }
		virtual bool CanSeek() { return false; }
		virtual int64_t Length() = 0;
		virtual int64_t Read(Byte* buffer, int64_t len) = 0;
		virtual int64_t Write(Byte* buffer, int64_t len) = 0;
		virtual void Seek(int64_t offset, SeekOrigin origin) = 0;
		virtual void Flush() = 0;
		virtual int64_t Position() = 0;
		virtual void Position(int64_t pos) = 0;
	};

	class MemoryStream :public Stream
	{
	private:
		void* ptr;
		size_t capacity;
		int64_t position = 0;
		bool sizable = false;
	public:
		MemoryStream(void* buffer, size_t size)
		{
			ptr = buffer;
			capacity = size;
		}
		MemoryStream(size_t capacity = 64)
		{
			this->capacity = capacity;
			sizable = true;
			ptr = malloc(capacity);
		}
		long Read(Byte* buffer, long len)
		{
			long size = len;
			if (Length() - Position() - 1 < len)
				len = Length() - Position() - 1;
			if (size > 0)
			{
				memcpy(buffer, ((char*)ptr + Position()), size);
				Position(Position() + size);
			}

			return len;
		}
		long Write(Byte* buffer, long len)
		{
			if (sizable && Position() + len >= Length())
			{
				auto nptr = realloc(ptr, capacity * 1.5);
				if (nptr)
				{
					ptr = nptr;
					capacity *= 1.5;
				}
			}
			long size = len;
			if (Length() - Position() - 1 < len)
				len = Length() - Position() - 1;

			memcpy(((uint8_t*)ptr + Position()), buffer, size);
			Position(Position() + size);

			return len;

		}
		void Seek(int64_t offset, SeekOrigin origin)
		{
			switch (origin)
			{
			case SeekOrigin::Current:
				Position(Position() + offset);
				break;
			case SeekOrigin::Start:
				Position(offset);
				break;
			case SeekOrigin::End:
				Position(Length() - offset);
				break;
			default:
				break;
			}
		}
		int64_t Position()
		{
			return position;
		}
		void Position(int64_t pos)
		{
			position = pos;
			if (position >= capacity)
				position = capacity - 1;
		}
		void Flush()
		{
		}
		int64_t Length()
		{
			return capacity;
		}



		bool CanRead() { return true; }
		bool CanWrite() { return true; }
		bool CanSeek() { return true; }

	};

	enum class FileMode
	{
		Create,
		CreateNew,
		Open,
		OpenCreate,
		Truncate,
		Append
	};
	enum class FileAccess
	{
		Read,
		Write,
		ReadWrite
	};
	class FileStream :public Stream
	{
	private:
		String path;
		void* handle;
		FileMode fmode;
		FileAccess faccs;
	public:
		FileStream(String path, FileMode mode, FileAccess accs);
		bool CanRead();
		bool CanWrite();
		bool CanSeek();
		int64_t Length();
		int64_t Read(Byte* buffer, int64_t len);
		int64_t Write(Byte* buffer, int64_t len);
		void Seek(int64_t offset, SeekOrigin origin);
		void Flush();
		int64_t Position();
		void Position(int64_t pos);
		void Close();
	};

	class BasicReader
	{
	private:
		Stream* stream = nullptr;

	public:
		bool Avaliable()
		{
			return stream != nullptr;
		}
		BasicReader(Stream& s)
		{
			if (s.CanRead())
				stream = &s;
			else
				throw Exception("Stream can not read.");
		}
		BasicReader(Stream* s)
		{
			if (s == nullptr) throw Exception("Invalid argument: Stream* s is nullptr");
			if (s->CanRead())
				stream = s;
			else
				throw Exception("Stream can not read.");
		}
		template<typename T>
		bool Read(T& value)
		{
			auto sz = stream->Read(reinterpret_cast<Byte*>(&value), sizeof T);
			return sz == sizeof T;
		}
		//length prepend utf-8 encoding
		bool Read(String& value);
		int64_t Read(Byte* buffer, int64_t len)
		{
			return stream->Read(buffer, len);
		}
		bool CanSeek()
		{
			return stream->CanSeek();
		}
		bool Seek(long offset, SeekOrigin origin)
		{
			if (CanSeek()) return false;
			stream->Seek(offset, origin);
		}
	};
	class BasicWriter
	{
	private:
		Stream* stream = nullptr;
	public:
		bool Avaliable() noexcept
		{
			return stream != nullptr;
		}
		BasicWriter(Stream& s)
		{
			if (s.CanWrite())
				stream = &s;
			else
				throw Exception("Stream can not write.");
		}
		BasicWriter(Stream* s)
		{
			if (s->CanWrite())
				stream = s;
			else
				throw Exception("Stream can not write.");
		}
		template<typename T>
		bool Write(T& value)
		{
			auto sz = stream->Write(reinterpret_cast<Byte*>(&value), sizeof T);
			return sz == sizeof T;
		}
		//length prepend utf-8 encoding
		bool Write(String& value);

		bool CanSeek() noexcept
		{
			return stream->CanSeek();
		}
		bool Seek(long offset, SeekOrigin origin)
		{
			if (CanSeek()) return false;
			stream->Seek(offset, origin);
		}
	};
	class StringReader
	{
	};
	class StringWriter
	{
	};
	class Console
	{
	public:
		static void Print(String fmt)
		{
			wprintf(fmt.CString());
		}
		template<typename... T>
		static void PrintLine(String fmt)
		{
			Print(fmt);
			wprintf(L"\n");
		}
		template<typename... T>
		static void Print(String fmt, T... args)
		{
			Print(String::Format(fmt, args...));
		}

		template<typename... T>
		static void PrintLine(String fmt, T... args)
		{
			PrintLine(String::Format(fmt, args...));
		}
		template<typename T>
		static void Print(T value)
		{
			wprintf(String::Format(L"{0}", value).CString());
		}
		template<typename T>
		static void PrintLine(T value)
		{
			wprintf(String::Format(L"{0}\n", value).CString());
		}
	};
	template<typename T>
	class XDelegate;
	template<typename R, typename... Args>
	class XDelegate<R(Args...)>
	{
		template <size_t i>
		struct _Arg
		{
			typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
		};
	};

	template<typename T>
	class Delegate
	{
		using _Fun = std::function<T>;
		//using _R = 
	private:
		std::vector<_Fun> funcs;
	public:
		Delegate(T f)
		{
			funcs.clear();
			funcs.push_back(f);
		}

	};
}
