/*
Copyright (c) 2014 Helios (helios.vmg@gmail.com)

This software is provided 'as-is', in the hope that it will be useful, but
without any express or implied warranty. In no event will the authors be held
liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including
commercial applications, and to alter it and redistribute it freely, subject to
the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
       claim that you wrote the original software. If you use this software
       in a product, an acknowledgment in the product documentation would
       be appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not
       be misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
       distribution.

    4. The user is granted all rights over the textual output of this
       software.

*/
#include "library.h"

template <typename T>
class auto_array_ptr
{
	T *pointer;
public:
	auto_array_ptr(T *p = 0): pointer(p), func(0){}
	~auto_array_ptr()
	{
		reset();
	}
	void reset(T *p = 0)
	{
		delete[] pointer;
		pointer = p;
	}
	operator bool() const
	{
		return !!pointer;
	}
	bool operator!() const
	{
		return !pointer;
	}
	T *get()
	{
		return pointer;
	}
};

#define BIN_USE_EXCEPTIONS

BIN_FUNCTION_SIGNATURE(std::string, read_sized_string, size_t length, std::istream &stream){
#ifdef BIN_USE_EXCEPTIONS
	auto_array_ptr<char> temp(new char[length]);
	stream.read(temp.get(), length);
	if (stream.gcount() < length)
		BIN_HURL_ERROR(ParserStatus::UNEXPECTED_EOF);
	return std::string(temp.get(), length);
#else
	auto_array_ptr<char> temp(new (std::nothrow) char[length]);
	if (!temp)
		BIN_HURL_ERROR(ParserStatus::ALLOCATION_ERROR);
	stream.read(temp.get(), length);
	if (stream.gcount() < length)
		BIN_HURL_ERROR(ParserStatus::UNEXPECTED_EOF);
	dst.assign(temp.get(), length);
	return ParserStatus::SUCCESS;
#endif
}

BIN_FUNCTION_SIGNATURE(std::string, read_cstyle_string, std::istream &stream){
#ifdef BIN_USE_EXCEPTIONS
	std::string temp;
	while (1){
		char c = stream.get();
		if (!stream)
			BIN_HURL_ERROR(ParserStatus::UNEXPECTED_EOF);
		if (!c)
			break;
		temp.push_back(c);
	}
	return temp;
#else
	while (1){
		char c = stream.get();
		if (!stream)
			BIN_HURL_ERROR(ParserStatus::UNEXPECTED_EOF);
		if (!c)
			break;
		dst.push_back(c);
	}
	return ParserStatus::SUCCESS;
#endif
}
/*
BIN_FUNCTION_SIGNATURE(std::string, read_user_length_string, std::istream &stream){
#ifdef BIN_USE_EXCEPTIONS
#else
#endif
}
*/