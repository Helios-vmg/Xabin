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
#include <exception>
#include <istream>
#include <boost/type_traits.hpp>

#define TWOS_COMPLEMENT 0
#define ONES_COMPLEMENT 1
#define SIGN_BIT 2
#define EXCESS_K_BIASED 3

#define NEGATIVE_ARITHMETIC_SYSTEM TWOS_COMPLEMENT

enum class ParserStatus{
	SUCCESS,
	UNEXPECTED_EOF,
	REQUIREMENT_NOT_MET,
	ALLOCATION_ERROR,
};

class ParsingException : public std::exception{
	ParserStatus status;
public:
	ParsingException(ParserStatus status): status(status){}
};

template <typename T>
boost::enable_if<boost::is_unsigned<T>, T> correct_sign_twoscomp(boost::make_unsigned<T> x){
	return (Dst)x;
}

template <typename T>
boost::enable_if<boost::is_signed<T>, T> correct_sign_twoscomp(boost::make_unsigned<T> x){
#if NEGATIVE_ARITHMETIC_SYSTEM == TWOS_COMPLEMENT
	return (Dst)x;
#elif NEGATIVE_ARITHMETIC_SYSTEM == TWOS_COMPLEMENT
#error
#elif NEGATIVE_ARITHMETIC_SYSTEM == ONES_COMPLEMENT
#error
#elif NEGATIVE_ARITHMETIC_SYSTEM == SIGN_BIT
#error
#elif NEGATIVE_ARITHMETIC_SYSTEM == EXCESS_K_BIASED
#error
#endif
}

template <typename T>
boost::enable_if<boost::is_unsigned<T>, T> correct_sign_onescomp(boost::make_unsigned<T> x){
	return (Dst)x;
}

template <typename T>
boost::enable_if<boost::is_signed<T>, T> correct_sign_onescomp(boost::make_unsigned<T> x){
#if NEGATIVE_ARITHMETIC_SYSTEM == TWOS_COMPLEMENT
	const auto mask = (T)1 << (sizeof(T) * 8 - 1);
	if (!(x & mask))
		return (Dst)x;
	return (Dst)(x + 1);
#elif NEGATIVE_ARITHMETIC_SYSTEM == TWOS_COMPLEMENT
#error
#elif NEGATIVE_ARITHMETIC_SYSTEM == ONES_COMPLEMENT
#error
#elif NEGATIVE_ARITHMETIC_SYSTEM == SIGN_BIT
#error
#elif NEGATIVE_ARITHMETIC_SYSTEM == EXCESS_K_BIASED
#error
#endif
}

template <typename T>
boost::enable_if<boost::is_unsigned<T>, T> correct_sign_signbit(boost::make_unsigned<T> x){
	return (Dst)x;
}

template <typename T>
boost::enable_if<boost::is_signed<T>, T> correct_sign_signbit(boost::make_unsigned<T> x){
#if NEGATIVE_ARITHMETIC_SYSTEM == TWOS_COMPLEMENT
	const auto mask = (T)1 << (sizeof(T) * 8 - 1);
	if (!(x & mask))
		return (Dst)x;
	return -(Dst)(x & ~mask);
#elif NEGATIVE_ARITHMETIC_SYSTEM == TWOS_COMPLEMENT
#error
#elif NEGATIVE_ARITHMETIC_SYSTEM == ONES_COMPLEMENT
#error
#elif NEGATIVE_ARITHMETIC_SYSTEM == SIGN_BIT
#error
#elif NEGATIVE_ARITHMETIC_SYSTEM == EXCESS_K_BIASED
#error
#endif
}

template <typename T>
boost::enable_if<boost::is_unsigned<T>, T> correct_sign_excessk_biased(boost::make_unsigned<T> x){
	return (Dst)x;
}

template <typename T>
boost::enable_if<boost::is_signed<T>, T> correct_sign_excessk_biased(boost::make_unsigned<T> x){
#if NEGATIVE_ARITHMETIC_SYSTEM == TWOS_COMPLEMENT
	const auto mask = (T)1 << (sizeof(T) * 8 - 1);
	if (x >= mask)
		return (Dst)(x - mask);
	return (Dst)x - (Dst)(mask - 1) - 1;
#elif NEGATIVE_ARITHMETIC_SYSTEM == TWOS_COMPLEMENT
#error
#elif NEGATIVE_ARITHMETIC_SYSTEM == ONES_COMPLEMENT
#error
#elif NEGATIVE_ARITHMETIC_SYSTEM == SIGN_BIT
#error
#elif NEGATIVE_ARITHMETIC_SYSTEM == EXCESS_K_BIASED
#error
#endif
}

#ifndef BIN_USE_EXCEPTIONS
#define BIN_FUNCTION_SIGNATURE(return_type, name, ...) ParserStatus name##_nothrow(return_type &dst, __VA_ARGS__)
#define BIN_HURL_ERROR(x) return x
#define BIN_RETURN(x) dst = x; return ParserStatus::SUCCESS
#else
#define FUNCTION_SIGNATURE(return_type, name, ...) return_type name(__VA_ARGS__)
#define HURL_ERROR(x) throw ParsingException(x)
#define BIN_RETURN(x) return x
#endif

template <typename T, unsigned N, T F(std::make_unsigned<T>)>
BIN_FUNCTION_SIGNATURE(T, read_little_integer, std::istream &stream){
	unsigned char bytes[N];
	stream.read((char *)bytes, N);
	if (stream.gcount() < N)
		BIN_HURL_ERROR(ParserStatus::UNEXPECTED_EOF);
	typedef boost::make_unsigned<T> u;
	u temp = 0;
	for (int i = 0; i != N; i++){
		temp <<= 8;
		temp |= bytes[i];
	}
	BIN_RETURN(F(temp));
}

BIN_FUNCTION_SIGNATURE(std::string, read_sized_string, size_t length, std::istream &stream);
BIN_FUNCTION_SIGNATURE(std::string, read_cstyle_string, std::istream &stream);
BIN_FUNCTION_SIGNATURE(std::string, read_user_length_string, std::istream &stream);

template <typename T>
ParserStatus read_cstyle_array(std::vector<std::string> &dst, std::istream &stream){

}

template <typename T>
ParserStatus read_cstyle_array(std::vector<T> &dst, std::istream &stream){
}
