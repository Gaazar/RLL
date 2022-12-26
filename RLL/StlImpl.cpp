#include "Sharp.h"
#include <fstream>
#include <windows.h>

using namespace Sharp;

FileStream::FileStream(String path, FileMode mode, FileAccess accs)
{
	if (path.IsEmpty())
	{
		throw Exception("Invalid argument: empty path");
	}
	this->path = path;
	fmode = mode;
	faccs = accs;

	{
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
		auto h = CreateFile(path.CString(), a, 0, 0, m, FILE_ATTRIBUTE_NORMAL, 0);
		if (!h) throw Exception("Open|Create file failed.");
		CloseHandle(h);
	}

	handle = new std::fstream();
	std::fstream& fs = *(std::fstream*)handle;
	auto m = std::ios::in | std::ios::out | std::ios::binary;
	switch (accs)
	{
	case FileAccess::Read:
		m = std::ios::in | std::ios::binary;
		break;
	case FileAccess::Write:
		m = std::ios::out | std::ios::binary;
		break;
	case FileAccess::ReadWrite:
		m = std::ios::in | std::ios::out | std::ios::binary;
		break;
	default:
		break;
	}
	fs.open(path.CString(), m);

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
	throw Exception("No impelements.");
	return 0;
}
int64_t FileStream::Read(Byte* buffer, int64_t len)
{
	std::fstream& fs = *(std::fstream*)handle;
	fs.read((char*)buffer, len);
	return !fs.eof();
}
int64_t FileStream::Write(Byte* buffer, int64_t len)
{
	std::fstream& fs = *(std::fstream*)handle;
	fs.write((char*)buffer, len);
	return !fs.eof();
}
void FileStream::Seek(int64_t offset, SeekOrigin origin)
{
	std::fstream& fs = *(std::fstream*)handle;
	std::ios_base::seekdir mm;
	switch (origin)
	{
	case SeekOrigin::Current:
		mm = std::ios::cur;
		break;
	case SeekOrigin::Start:
		mm = std::ios::beg;
		break;
	case SeekOrigin::End:
		mm = std::ios::end;
		break;
	default:
		break;
	}
	fs.seekg(offset, mm);
	fs.seekp(offset, mm);
}
void FileStream::Flush()
{
	std::fstream& fs = *(std::fstream*)handle;
	fs.flush();
}
int64_t FileStream::Position()
{
	std::fstream& fs = *(std::fstream*)handle;
	return fs.showpos;
}
void FileStream::Position(int64_t pos)
{
	Seek(pos, SeekOrigin::Start);
}
void FileStream::Close()
{
	std::fstream& fs = *(std::fstream*)handle;
	fs.close();
}