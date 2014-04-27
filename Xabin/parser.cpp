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

struct XmlAttributeNotFoundException{};

std::string guaranteed_get_attribute(tinyxml2::XMLElement *el, const char *name){
	auto value = el->Attribute(name);
	if (!value)
		throw XmlAttributeNotFoundException();
	return value;
}

std::string &operator<<(std::string &str, const boost::format &format){
	str.append(format.str());
	return str;
}

struct DefinedInteger_ptr_cmp{
	bool operator()(DefinedInteger *a, DefinedInteger *b) const{
		return a->get_size() > b->get_size() || a->get_size() == b->get_size() && !a->get_signedness() && b->get_signedness();
	}
};

std::string DefinedType::generate_declaration(bool use_exceptions) const{
	std::string ret;
	boost::format namespace_open("namespace %1%{\n"),
		namespace_close("} // namespace %1%\n"),
		struct_open("struct %1%{\n"),
		constructor_and_close(
			"\t%1%(std::istream &);\n"
			"} // struct %1%\n"
		);
	for (auto &ns : this->namespaces)
		ret << namespace_open % ns;
	ret << struct_open % this->name;
	{
		std::vector<DefinedInteger *> integers;
		std::vector<DefinedDatum *> nonintegers;
		for (auto &member : this->data){
			if (member->get_type() == DataType::INTEGER)
				integers.push_back((DefinedInteger *)member.get());
			else
				nonintegers.push_back(member.get());
		}
		std::sort(integers.begin(), integers.end(), DefinedInteger_ptr_cmp());
		unsigned last_type_id = 0;
		for (auto p : integers){
			auto type_id = p->get_int_type_id();
			if (type_id != last_type_id){
				ret.append(last_type_id ? ";\n\t" : "\t");
				ret.append(p->get_c_type());
				ret.push_back(' ');
			}else
				ret.append(",\n\t\t");
			ret.append(p->get_name());
			last_type_id = type_id;
		}
		ret.append(";\n");
		if (!use_exceptions)
			ret.append("\tbool good;\n");
		for (auto p : nonintegers){
			ret.append("\t");
			ret.append(p->get_signature());
			ret.append(";\n");
		}
	}
	ret <<constructor_and_close % this->name;
	for (auto &ns : this->namespaces)
		ret << namespace_close % ns;
	return ret;
}

std::string DefinedType::generate_definition(bool use_exceptions) const{
	boost::format namespace_open("namespace %1%{\n"),
		namespace_close("} // namespace %1%\n");
	std::string ret;
	for (auto &ns : this->namespaces)
		ret << namespace_open % ns;
	ret.append(this->name);
	ret.append("::");
	ret.append(this->name);
	ret.append("(std::istream &stream): good(false){\n");
	if (!use_exceptions)
		ret.append("\tParserStatus status;\n");
	for (auto d : this->data){
		if (!use_exceptions){
			ret.append("\tstatus = ");
			ret.append(d->generate_read_code(use_exceptions));
			ret.append(
				";\n"
				"\tif (status != ParserStatus::SUCCESS)\n"
				"\t\treturn status;\n"
			);
		}else{
			ret.push_back('\t');
			ret.append(d->generate_read_code(use_exceptions));
			ret.append(";\n");
		}
		ret.append(d->generate_requirement_code(use_exceptions));
	}
	ret.append("}\n");
	for (auto &ns : this->namespaces)
		ret << namespace_close % ns;
	return ret;
}

std::string DefinedInteger::generate_read_code(bool use_exceptions) const{
	const char *with_exceptions    = "this->%1% = read_%2%_integer<%3%, %4%, correct_sign_%5%>(stream)";
	const char *without_exceptions = "read_%2%_integer_nothrow<%3%, %4%, correct_sign_%5%>(this->%1%, stream)";
	boost::format format(use_exceptions ? with_exceptions : without_exceptions);
	return (format
		% this->name
		% this->get_endianness_word()
		% this->get_c_type()
		% this->size
		% this->get_negative_mapping_word()).str();
}

std::string DefinedDatum::generate_requirement_code(bool use_exceptions) const{
	if (!this->req.get())
		return std::string();
	const char *with_exceptions =
		"\tif (!(this->%1% %2%))\n"
		"\t\tthrow ParsingException(ParserStatus::REQUIREMENT_NOT_MET);\n";
	const char *without_exceptions =
		"\tif (!(this->%1% %2%))\n"
		"\t\treturn ParserStatus::REQUIREMENT_NOT_MET;\n";
	boost::format format(use_exceptions ? with_exceptions : without_exceptions);
	return (format % this->name % this->req->generate_code()).str();
}

std::string DefinedString::generate_read_code(bool use_exceptions) const{
	const char *with_exceptions    = "this->%1% = read_%2%_string(stream%3%)";
	const char *without_exceptions = "read_%2%_string_nothrow(this->%1%, stream%3%)";
	boost::format format(use_exceptions ? with_exceptions : without_exceptions);
	return (format
		% this->name
		% this->length->get_length_word()
		% this->length->generate_length_parameter()).str();
}

//-----------------------------------------------------------------------------

static struct{
	const char *name;
	IntegerType type;
} type_pairs[] = {
	{"u8", 0, 8},
	{"s8", 1, 8},
	{"u16", 0, 16},
	{"s16", 1, 16},
	{"u32", 0, 32},
	{"s32", 1, 32},
	{"u64", 0, 64},
	{"s64", 1, 64},
};

DefinedInteger::DefinedInteger(tinyxml2::XMLElement *integer, const IntegerFormat &format, const IntegerType &type): DefinedDatum(DataType::INTEGER){
	this->name = guaranteed_get_attribute(integer, "name");
	this->format = format;
	this->signedness = type.signedness;
	this->size = type.bitness / 8;
	for (auto el = integer->FirstChildElement(); el; el = el->NextSiblingElement()){
		if (!strcmp(integer->Name(), "require")){
			this->req.reset(new Requirement(el));
		}
	}
}

DefinedString::DefinedString(tinyxml2::XMLElement *string): DefinedDatum(DataType::STRING){
	this->name = guaranteed_get_attribute(string, "name");
	auto length = string->Attribute("length");
	if (!length){
		this->length.reset(new CStyleArrayLength);
		return;
	}
	std::string stdlength = length;
	if (!stdlength.size())
		throw Parser::MetaParserStatus::MALFORMED_XML_STRUCTURE;
	switch (stdlength[0]){
		case '$':
			this->length.reset(new PrestatedArrayLength(stdlength.substr(1)));
			break;
		case '@':
			this->length.reset(new UserArrayLength(stdlength.substr(1)));
			break;
		default:
			this->length.reset(new FixedArrayLength(stdlength));
	}
}

IntegerType *find(const char *id){
	for (auto &p : type_pairs)
		if (!strcmp(p.name, id))
			return &p.type;
	return nullptr;
}

void DefinedType::parse(tinyxml2::XMLElement *type, ParserState &state){
	for (auto el = type->FirstChildElement(); el; el = el->NextSiblingElement()){
		std::string name = el->Name();
		auto i = find(name.c_str());
		if (i)
			this->add_datum(boost::shared_ptr<DefinedDatum>(new DefinedInteger(el, state.current_format, *i)));
		else if (name == "string")
			this->add_datum(boost::shared_ptr<DefinedDatum>(new DefinedString(el)));
		else if (name == "format")
			state.current_format = IntegerFormat(el);
		else if (name == "scope"){
			auto new_state = state;
			this->parse(el, new_state);
		}
	}
}

DefinedType::DefinedType(tinyxml2::XMLElement *type, ParserState &state){
	this->namespaces = state.current_namespace;
	this->name = guaranteed_get_attribute(type, "name");
	std::map<std::string, IntegerType> map;
	for (auto &pair : type_pairs)
		map[pair.name] = pair.type;
	parse(type, state);
}

Requirement::Requirement(tinyxml2::XMLElement *req){
	for (auto attr = req->FirstAttribute(); attr; attr = attr->Next()){
		std::string name = attr->Name();
		Relation rel = Relation::NONE;
		if (name == "eq")
			rel = Relation::EQ;
		else if (name == "neq")
			rel = Relation::NEQ;
		else if (name == "gt")
			rel = Relation::GT;
		else if (name == "lt")
			rel = Relation::LT;
		else if (name == "geq")
			rel = Relation::GEQ;
		else if (name == "leq")
			rel = Relation::LEQ;
		if (rel != Relation::NONE){
			this->rel = rel;
			this->value = attr->Value();
		}
	}
}

IntegerFormat::IntegerFormat(){
	this->endianness = Endianness::LITTLE;
	this->negative_mapping = NegativeMapping::TWOSCOMP;
}

IntegerFormat::IntegerFormat(tinyxml2::XMLElement *el){
	for (auto attr = el->FirstAttribute(); attr; attr = attr->Next()){
		std::string name = attr->Name();
		if (name == "end"){
			std::string val = attr->Value();
			if (val == "little")
				this->endianness = Endianness::LITTLE;
			else if (val == "big")
				this->endianness = Endianness::BIG;
			else
				throw Parser::MetaParserStatus::INVALID_FORMAT_SPECIFIER;
		}else if (name == "neg"){
			std::string val = attr->Value();
			if (val == "twoscomp")
				this->negative_mapping = NegativeMapping::TWOSCOMP;
			else if (val == "onescomp")
				this->negative_mapping = NegativeMapping::ONESCOMP;
			else if (val == "signbit")
				this->negative_mapping = NegativeMapping::SIGNBIT;
			else if (val == "excesskbiased")
				this->negative_mapping = NegativeMapping::EXCESSKBIASED;
			else
				throw Parser::MetaParserStatus::INVALID_FORMAT_SPECIFIER;
		}
	}
}

struct WordPair{
	std::string first;
	int second;
};

WordPair valid_first_line_words[] = {
	{ "format", (int)FirstLineWord::FORMAT },
	{ "begin",  (int)FirstLineWord::BEGIN },
	{ "end",    (int)FirstLineWord::END },
	{ "u8",     (int)FirstLineWord::U8 },
	{ "u16",    (int)FirstLineWord::U16 },
	{ "u32",    (int)FirstLineWord::U32 },
	{ "u64",    (int)FirstLineWord::U64 },
	{ "s8",     (int)FirstLineWord::S8 },
	{ "s16",    (int)FirstLineWord::S16 },
	{ "s32",    (int)FirstLineWord::S32 },
	{ "s64",    (int)FirstLineWord::S64 },
	{ "string", (int)FirstLineWord::STRING },
	{ "array",  (int)FirstLineWord::ARRAY },
	{ "struct", (int)FirstLineWord::STRUCT },
};

WordPair valid_formats[] = {
	{ "little",        (int)FormatWord::LITTLE },
	{ "big",           (int)FormatWord::BIG },
	{ "twoscomp",      (int)FormatWord::TWOSCOMP },
	{ "onescomp",      (int)FormatWord::ONESCOMP },
	{ "signbit",       (int)FormatWord::SIGNBIT },
	{ "excesskbiased", (int)FormatWord::EXCESSKBIASED },
};

WordPair valid_begins[] = {
	{ "namespace", (int)BeginWord::NAMESPACE },
	{ "type",      (int)BeginWord::TYPE },
};

WordPair valid_length_words[] = {
	{ "fixed",  (int)LengthTypeWord::FIXED },
	{ "seen",   (int)LengthTypeWord::SEEN },
	{ "cstyle", (int)LengthTypeWord::CSTYLE },
	{ "user",   (int)LengthTypeWord::USER },
};

Parser::Parser(){
	this->init_maps();
	this->eol_function = nullptr;
}

bool is_blank(const std::string &line){
	if (!line.size())
		return 1;
	auto first = line.find_first_not_of(" \t");
	return first == line.npos;
}

bool is_comment(const std::string &line){
	auto first = line.find_first_not_of(" \t");
	return line[first] == ';';
}

Parser::MetaParserStatus Parser::parse_specification(const char *file_path){
	std::ifstream spec_file(file_path);
	if (!spec_file)
		return MetaParserStatus::FILE_NOT_FOUND;
	while (1){
		std::string line;
		std::getline(spec_file, line);
		if (!spec_file)
			break;
		if (is_blank(line) || is_comment(line))
			continue;
		std::stringstream stream(line);
		auto status = *this <<stream;
		if (status != Parser::MetaParserStatus::SUCCESS)
			return status;
	}
	return MetaParserStatus::SUCCESS;
}

void Parser::parse(tinyxml2::XMLElement *node, ParserState &state){
	for (auto el = node->FirstChildElement(); el; el = el->NextSiblingElement()){
		std::string name = el->Name();
		if (name == "format"){
			this->state.current_format = IntegerFormat(el);
		}else if (name == "namespace"){
			auto new_state = state;
			new_state.current_namespace.push_back(guaranteed_get_attribute(el, "name"));
			parse(el, new_state);
		}else if (name == "type"){
			auto new_state = state;
			this->types.push_back(boost::shared_ptr<DefinedType>(new DefinedType(el, new_state)));
		}else if (name == "scope"){
			auto new_state = state;
			parse(el, new_state);
		}
	}
}

Parser::MetaParserStatus Parser::load_xml(const char *file_path){
	using namespace tinyxml2;
	XMLDocument doc;
	{
		switch (doc.LoadFile(file_path)){
			case XML_SUCCESS:
				break;
			case XML_ERROR_FILE_NOT_FOUND:
				return MetaParserStatus::FILE_NOT_FOUND;
			case XML_ERROR_FILE_COULD_NOT_BE_OPENED:
			case XML_ERROR_FILE_READ_ERROR:
				return MetaParserStatus::FILE_ERROR;
			default:
				return MetaParserStatus::UNKNOWN_XML_ERROR;
		}
	}
	auto spec = doc.FirstChildElement();
	if (strcmp(spec->Name(), "spec"))
		return MetaParserStatus::MALFORMED_XML_STRUCTURE;

	try{
		this->parse(spec, this->state);
	}catch (const XmlAttributeNotFoundException &){
		return MetaParserStatus::MALFORMED_XML_STRUCTURE;
	}catch (const Parser::MetaParserStatus &status){
		return status;
	}
	return MetaParserStatus::SUCCESS;
}

void Parser::init_maps(){
	for (auto &p : valid_first_line_words)
		this->first_line_words[p.first] = (FirstLineWord)p.second;
	for (auto &p : valid_formats)
		this->formats[p.first] = (FormatWord)p.second;
	for (auto &p : valid_begins)
		this->begins[p.first] = (BeginWord)p.second;
	for (auto &p : valid_length_words)
		this->length_type_words[p.first] = (LengthTypeWord)p.second;
}

Parser::MetaParserStatus Parser::pop_state(){
	if (!this->stack.size())
		return MetaParserStatus::EXTRANEOUS_END;
	switch (this->state.current_block){
		case ParserState::BlockType::TYPE:
			this->types.push_back(this->state.current_type);
			break;
	}
	this->state = this->stack.back();
	this->stack.pop_back();
	return MetaParserStatus::SUCCESS;
}

Parser::MetaParserStatus Parser::operator<<(std::istream &stream){
	TCO tco;
	auto f = tco.f = &Parser::line_start;
	MetaParserStatus ret;
	while ((ret = (this->*f)(stream, tco)) == MetaParserStatus::CONTINUE)
		f = tco.f;
	if (this->eol_function && ret == MetaParserStatus::SUCCESS)
		ret = (this->*this->eol_function)();
	this->eol_function = nullptr;
	return ret;
}

#define DEFINE_TCO_FUNCTION(name) Parser::MetaParserStatus Parser:: name (std::istream &stream, TCO &tco){ \
	std::string word;                                                                                      \
	if (!(stream >>word))                                                                                  \
		return MetaParserStatus::SUCCESS;
#define END_DEFINE_TCO_FUNCTION return MetaParserStatus::SYNTAX_ERROR; }

#define DEFINE_TCO_FUNCTION2(name, result) Parser::MetaParserStatus Parser:: name (std::istream &stream, TCO &tco){ \
	std::string word;                                                                                               \
	if (!(stream >>word))                                                                                           \
		return result;

#define DEFINE_TCO_FUNCTION_REQ_ID(name) DEFINE_TCO_FUNCTION2(name, MetaParserStatus::EXPECTED_IDENTIFIER){ \
	if (!is_valid_identifier(word))                                                                                 \
		return MetaParserStatus::INVALID_IDENTIFIER;
#define END_DEFINE_TCO_FUNCTION_REQ_ID }END_DEFINE_TCO_FUNCTION

Parser::MetaParserStatus Parser::add_datum_to_type(){
	if (!this->state.current_datum->validate()){
		this->state.current_datum.reset();
		return MetaParserStatus::DATUM_NOT_PROPERLY_DEFINED;
	}
	this->state.current_type->add_datum(this->state.current_datum);
	this->state.current_datum.reset();
	return MetaParserStatus::SUCCESS;
}

#define GO_TO_STATE(x) tco.f = &Parser::x; return MetaParserStatus::CONTINUE

DEFINE_TCO_FUNCTION(line_start){
	auto i = this->first_line_words.find(word);
	if (i == this->first_line_words.end())
		return MetaParserStatus::SYNTAX_ERROR;
	switch (i->second){
		case FirstLineWord::FORMAT:
			GO_TO_STATE(format);
		case FirstLineWord::BEGIN:
			this->stack.push_back(this->state);
			GO_TO_STATE(begin);
		case FirstLineWord::END:
			return this->pop_state();
		case FirstLineWord::U8:
		case FirstLineWord::U16:
		case FirstLineWord::U32:
		case FirstLineWord::U64:
		case FirstLineWord::S8:
		case FirstLineWord::S16:
		case FirstLineWord::S32:
		case FirstLineWord::S64:
			this->state.current_datum.reset(new DefinedInteger(this->state.current_format, word_is_signed_integer(i->second), bits_of_integer_in_word(i->second)));
			this->eol_function = &Parser::add_datum_to_type;
			GO_TO_STATE(member_name);
		case FirstLineWord::STRING:
			this->state.current_datum.reset(new DefinedString);
			this->eol_function = &Parser::add_datum_to_type;
			GO_TO_STATE(member_name);
	}
}END_DEFINE_TCO_FUNCTION

DEFINE_TCO_FUNCTION(format){
	auto i = this->formats.find(word);
	if (i == this->formats.end())
		return MetaParserStatus::SYNTAX_ERROR;
	auto &format = this->state.current_format;
	switch (i->second){
		case FormatWord::LITTLE:
			format.endianness = Endianness::LITTLE;
			break;
		case FormatWord::BIG:
			format.endianness = Endianness::BIG;
			break;
		case FormatWord::TWOSCOMP:
			format.negative_mapping = NegativeMapping::TWOSCOMP;
			break;
		case FormatWord::ONESCOMP:
			format.negative_mapping = NegativeMapping::ONESCOMP;
			break;
		case FormatWord::SIGNBIT:
			format.negative_mapping = NegativeMapping::SIGNBIT;
			break;
		case FormatWord::EXCESSKBIASED:
			format.negative_mapping = NegativeMapping::EXCESSKBIASED;
			break;
	}
	GO_TO_STATE(format);
}END_DEFINE_TCO_FUNCTION

DEFINE_TCO_FUNCTION(begin){
	auto i = this->begins.find(word);
	if (i == this->begins.end())
		return MetaParserStatus::SYNTAX_ERROR;
	switch (i->second){
		case BeginWord::NAMESPACE:
			GO_TO_STATE(namespace_name);
		case BeginWord::TYPE:
			GO_TO_STATE(type_name);
	}
}END_DEFINE_TCO_FUNCTION

bool is_valid_identifier(const std::string &s){
	if (!s.size())
		return 0;
	if (s[0] != '_' && !isalpha(s[0]))
		return 0;
	for (size_t i = 1; i < s.size(); i++)
		if (s[i] != '_' && !isalnum(s[i]))
			return 0;
	return 1;
}

DEFINE_TCO_FUNCTION_REQ_ID(namespace_name){
	this->state.current_namespace.push_back(word);
	this->state.current_block = ParserState::BlockType::NAMESPACE;
	return MetaParserStatus::SUCCESS;
}END_DEFINE_TCO_FUNCTION_REQ_ID

DEFINE_TCO_FUNCTION_REQ_ID(type_name){
	auto type = new DefinedType;
	this->state.current_type.reset(type);
	type->set_namespace(this->state.current_namespace);
	type->set_name(word);
	this->state.current_block = ParserState::BlockType::TYPE;
	return MetaParserStatus::SUCCESS;
}END_DEFINE_TCO_FUNCTION_REQ_ID

DEFINE_TCO_FUNCTION_REQ_ID(member_name){
	this->state.current_datum->set_name(word);
	GO_TO_STATE(member_name_done);
}END_DEFINE_TCO_FUNCTION_REQ_ID

DEFINE_TCO_FUNCTION(member_name_done){
	if (word == "require"){
		auto type = this->state.current_datum->get_type();
		if (type != DataType::INTEGER && type != DataType::STRING)
			return MetaParserStatus::REQUIRE_ONLY_FOR_SIMPLE_VALUES;
		GO_TO_STATE(require_operator);
	}else if (word == "length"){
		GO_TO_STATE(length_start);
	}
	return MetaParserStatus::UNKNOWN_TOKEN;
}END_DEFINE_TCO_FUNCTION

DEFINE_TCO_FUNCTION2(require_operator, MetaParserStatus::EXPECTED_RELATIONAL_OPERATOR){
	Requirement::Relation rel = Requirement::Relation::NONE;
	if (word == "==")
		rel = Requirement::Relation::EQ;
	else if (word == "!=")
		rel = Requirement::Relation::NEQ;
	else if (word == "<=")
		rel = Requirement::Relation::LEQ;
	else if (word == ">=")
		rel = Requirement::Relation::GEQ;
	else if (word == "<")
		rel = Requirement::Relation::LT;
	else if (word == ">")
		rel = Requirement::Relation::GT;
	else
		return MetaParserStatus::EXPECTED_RELATIONAL_OPERATOR;
	Requirement *req = nullptr;
	switch (this->state.current_datum->get_type()){
		case DataType::INTEGER:
			req = new IntegerRequirement(rel);
			break;
		case DataType::STRING:
			req = new StringRequirement(rel);
			break;
		default:
			assert(0);
	}
	this->state.current_requirement.reset(req);
	GO_TO_STATE(require_value);
}END_DEFINE_TCO_FUNCTION

DEFINE_TCO_FUNCTION2(require_value, MetaParserStatus::EXPECTED_VALUE){
	this->state.current_requirement->set_value(word);
	this->state.current_datum->req = this->state.current_requirement;
	this->state.current_requirement.reset();
	GO_TO_STATE(member_name_done);
}END_DEFINE_TCO_FUNCTION

DEFINE_TCO_FUNCTION2(length_start, MetaParserStatus::EXPECTED_LENGTH_SPECIFICATION){
	auto i = this->length_type_words.find(word);
	if (i == this->length_type_words.end())
		return MetaParserStatus::INVALID_LENGTH_SPECIFICATION;
	switch (i->second){
		case LengthTypeWord::FIXED:
			this->state.current_length.reset(new FixedArrayLength);
			GO_TO_STATE(length_value);
		case LengthTypeWord::SEEN:
			this->state.current_length.reset(new PrestatedArrayLength);
			GO_TO_STATE(length_name);
		case LengthTypeWord::CSTYLE:
			this->state.current_datum->set_length(new CStyleArrayLength);
			GO_TO_STATE(member_name_done);
		case LengthTypeWord::USER:
			this->state.current_length.reset(new UserArrayLength);
			GO_TO_STATE(length_name);
	}
}END_DEFINE_TCO_FUNCTION

DEFINE_TCO_FUNCTION2(length_value, MetaParserStatus::EXPECTED_LENGTH_VALUE){
	((FixedArrayLength *)this->state.current_length.get())->length = word;
	this->state.current_datum->set_length(this->state.current_length);
	this->state.current_length.reset();
	GO_TO_STATE(member_name_done);
}END_DEFINE_TCO_FUNCTION

DEFINE_TCO_FUNCTION2(length_name, MetaParserStatus::EXPECTED_LENGTH_NAME){
	((NamedArrayLength *)this->state.current_length.get())->name = word;
	this->state.current_datum->set_length(this->state.current_length);
	this->state.current_length.reset();
	GO_TO_STATE(member_name_done);
}END_DEFINE_TCO_FUNCTION
