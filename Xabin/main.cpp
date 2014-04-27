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
#include "stdafx.h"
#include "parser.h"

#define PROGRAM_NAME "placeholder"

int main(int argc, char **argv){
	const char *path;
#if 0
	if (argc < 2){
		std::cerr <<"Usage: " PROGRAM_NAME " <specification file>\n";
		return -1;
	}
	path = argv[1];
#else
	//path = "test.txt";
	path = "test.xml";
#endif
	Parser parser;
	parser.load_xml(path);
	parser.parse_specification(path);
	std::cout <<parser.generate_declarations(1);
	std::cout <<parser.generate_definitions(1);
	return 0;
}
