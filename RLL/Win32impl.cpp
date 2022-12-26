#include "Sharp.h"
#include "windows.h"
using namespace Sharp;
#ifdef NOSTL
FileStream::FileStream(String path, FileMode mode, FileAccess accs)
{
	if (path.IsEmpty())
	{
		throw Exception("Invalid argument: empty path");
	}
	this->path = path;
	fmode = mode;
	faccs = accs;
	DWORD m = 0, a = 0;
	switch (mode)
	{
	case FileMode::Create:
		m = OPEN_ALWAYS;
		break;
	case FileMode::CreateNew:
		m = CREATE_NEW;
		break;
	case FileMode::Open:
		m = OPEN_EXISTING;
		break;
	case FileMode::OpenCreate:
		m = CREATE_ALWAYS;
		break;
	case FileMode::Truncate:
		m = TRUNCATE_EXISTING;
		break;
	case FileMode::Append:
		m = OPEN_EXISTING;
		break;
	default:
		break;
	}
	switch (accs)
	{
	case FileAccess::Read:
		a = GENERIC_READ;
		break;
	case FileAccess::Write:
		a = GENERIC_WRITE;
		break;
	case FileAccess::ReadWrite:
		a = GENERIC_READ | GENERIC_WRITE;
		break;
	default:
		break;
	}
	handle = CreateFile(path.CString(), a, 0, 0, m, FILE_ATTRIBUTE_NORMAL, 0);
	if (handle == 0) throw Exception("Open|Create file failed.");
	if (mode == FileMode::Append)
		SetFilePointer(handle, 0, 0, FILE_END);
}
bool FileStream::CanRead()
{
	return handle && (faccs == FileAccess::Read || faccs == FileAccess::ReadWrite);
}
bool FileStream::CanWrite()
{
	return handle && (faccs == FileAccess::Write || faccs == FileAccess::ReadWrite);

}
bool FileStream::CanSeek()
{
	return true;
}
int64_t FileStream::Length()
{
	LARGE_INTEGER len;
	GetFileSizeEx(handle, &len);
	return len.QuadPart;
}
int64_t FileStream::Read(Byte* buffer, int64_t len)
{
	DWORD rlen = 0;
	if (!ReadFile(handle, buffer, len, &rlen, NULL)) throw Exception("ReadFile failed.");
	return rlen;
}
int64_t FileStream::Write(Byte* buffer, int64_t len)
{
	DWORD rlen = 0;
	if (!WriteFile(handle, buffer, len, &rlen, NULL)) throw Exception("WriteFile failed.");
	return rlen;
}
void FileStream::Seek(int64_t offset, SeekOrigin origin)
{
	DWORD mm;
	switch (origin)
	{
	case SeekOrigin::Current:
		mm = FILE_CURRENT;
		break;
	case SeekOrigin::Start:
		mm = FILE_BEGIN;
		break;
	case SeekOrigin::End:
		mm = FILE_END;
		break;
	default:
		break;
	}
	LARGE_INTEGER li;
	li.QuadPart = offset;
	if (!SetFilePointerEx(handle, li, NULL, mm)) throw Exception("SeekFile failed.");
}
void FileStream::Flush()
{
	FlushFileBuffers(handle);
}
int64_t FileStream::Position()
{
	LARGE_INTEGER li;
	li.QuadPart = 0;
	LARGE_INTEGER rli;
	if (!SetFilePointerEx(handle, li, &rli, FILE_CURRENT)) throw Exception("Get file position failed.");
	return li.QuadPart;

}
void FileStream::Position(int64_t pos)
{
	LARGE_INTEGER li;
	li.QuadPart = pos;
	if (!SetFilePointerEx(handle, li, NULL, FILE_BEGIN)) throw Exception("Get file position failed.");
}
void FileStream::Close()
{
	CloseHandle(handle);
}
#endif