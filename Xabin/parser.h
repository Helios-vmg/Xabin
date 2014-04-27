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
const unsigned flags_start = 8;
const unsigned integer_flag = 1 << flags_start;
const unsigned signed_integer_flag = 1 << (flags_start + 1);
template <unsigned N>
struct CountFinalZeroes{
	static const unsigned value = CountFinalZeroes<(N >> 1)>::value + 1;
};
template <>
struct CountFinalZeroes<1>{
	static const unsigned value = 0;
};
template <unsigned N>
struct BitsToLogOfBytes{
	static const unsigned value = CountFinalZeroes<N>::value - 3;
};
const unsigned integer_size_start = flags_start + 2;
const unsigned integer_size_mask = (1 << 6) - 1;
#define UINTEGER_OF_SIZE(x) (integer_flag | (BitsToLogOfBytes<x>::value << integer_size_start))
#define SINTEGER_OF_SIZE(x) (integer_flag | signed_integer_flag | (BitsToLogOfBytes<x>::value << integer_size_start))

//Largest integers allowed by this scheme: 2^66-bit
enum class FirstLineWord{
	FORMAT = 0,
	BEGIN  = 1,
	END    = 2,
	U8     = UINTEGER_OF_SIZE(8),
	U16    = UINTEGER_OF_SIZE(16),
	U32    = UINTEGER_OF_SIZE(32),
	U64    = UINTEGER_OF_SIZE(64),
	S8     = SINTEGER_OF_SIZE(8),
	S16    = SINTEGER_OF_SIZE(16),
	S32    = SINTEGER_OF_SIZE(32),
	S64    = SINTEGER_OF_SIZE(64),
	STRING = 3,
	ARRAY  = 4,
	STRUCT = 5,
};

inline bool word_is_integer(FirstLineWord word){
	return ((unsigned)word & integer_flag) == integer_flag;
}

inline bool word_is_signed_integer(FirstLineWord word){
	return ((unsigned)word & signed_integer_flag) == signed_integer_flag;
}

inline unsigned bits_of_integer_in_word(FirstLineWord word){
	unsigned n = (unsigned)word;
	n >>= integer_size_start;
	n &= integer_size_mask;
	return 1 << (n + 3);
}

enum class FormatWord{
	LITTLE = 0,
	BIG,
	TWOSCOMP,
	ONESCOMP,
	SIGNBIT,
	EXCESSKBIASED,
};

enum class BeginWord{
	NAMESPACE = 0,
	TYPE,
};

enum class LengthTypeWord{
	FIXED = 0,
	SEEN,
	CSTYLE,
	USER,
};

enum class Endianness{
	LITTLE,
	BIG,
};

enum class NegativeMapping{
	TWOSCOMP,
	ONESCOMP,
	SIGNBIT,
	EXCESSKBIASED,
};

enum class DataType{
	INTEGER,
	STRING,
	ARRAY,
	STRUCT,
};

class Requirement{
public:
	enum class Relation{
		NONE,
		EQ,
		NEQ,
		LT,
		GT,
		LEQ,
		GEQ,
	};
private:
	Relation rel;
	std::string value;
public:
	Requirement(Relation rel): rel(rel){}
	Requirement(tinyxml2::XMLElement *);
	virtual ~Requirement(){}
	const char *generate_relational() const{
		switch (this->rel){
			case Relation::NONE:
				return "";
			case Relation::EQ:
				return "==";
			case Relation::NEQ:
				return "!=";
			case Relation::LT:
				return "<";
			case Relation::GT:
				return ">";
			case Relation::LEQ:
				return "<=";
			case Relation::GEQ:
				return ">=";
		}
		return 0;
	}
	void set_value(const std::string &s){
		this->value = s;
	}
	std::string generate_code() const{
		std::string ret;
		ret.append(this->generate_relational());
		ret.append(" ");
		ret.append(this->value);
		return ret;
	}
};

typedef Requirement IntegerRequirement;
typedef Requirement StringRequirement;

class IntegerFormat{
public:
	Endianness endianness;
	NegativeMapping negative_mapping;
	IntegerFormat();
	IntegerFormat(tinyxml2::XMLElement *);
};

class ArrayLength;

class DefinedDatum{
protected:
	DataType type;
	std::string name;
public:
	boost::shared_ptr<Requirement> req;
	DefinedDatum(DataType type): type(type){}
	virtual ~DefinedDatum(){}
	DataType get_type() const{
		return this->type;
	}
	virtual bool validate() const{
		return 1;
	}
	virtual void set_length(ArrayLength *){}
	virtual void set_length(boost::shared_ptr<ArrayLength> &){}
	virtual std::string get_signature() const = 0;
	const std::string &get_name() const{
		return this->name;
	}
	void set_name(const std::string &name){
		this->name = name;
	}
	virtual std::string generate_read_code(bool use_exceptions) const = 0;
	std::string generate_requirement_code(bool use_exceptions) const;
};

struct IntegerType{
	bool signedness;
	unsigned bitness;
};

class DefinedInteger : public DefinedDatum{
	IntegerFormat format;
	bool signedness;
	unsigned size;
public:
	DefinedInteger(IntegerFormat &format, bool signedness, unsigned bitness): DefinedDatum(DataType::INTEGER), format(format), signedness(signedness), size(bitness / 8){}
	DefinedInteger(tinyxml2::XMLElement *, const IntegerFormat &format, const IntegerType &);
	unsigned get_size() const{
		return this->size;
	}
	bool get_signedness() const{
		return this->signedness;
	}
	unsigned get_int_type_id() const{
		return (this->size << 1) | (unsigned)this->signedness;
	}
	const char *get_c_type() const{
		switch (this->get_int_type_id()){
			case 1 << 1:
				return "uint8_t";
			case (1 << 1) | 1:
				return "int8_t";
			case 2 << 1:
				return "uint16_t";
			case (2 << 1) | 1:
				return "int16_t";
			case 4 << 1:
				return "uint32_t";
			case (4 << 1) | 1:
				return "int32_t";
			case 8 << 1:
				return "uint64_t";
			case (8 << 1) | 1:
				return "int64_t";
		}
		return 0;
	}
	const char *get_endianness_word() const{
		switch (this->format.endianness){
			case Endianness::LITTLE:
				return "little";
			case Endianness::BIG:
				return "big";
			default:
				assert(0);
		}
		return 0;
	}
	const char *get_negative_mapping_word() const{
		switch (this->format.negative_mapping){
			case NegativeMapping::TWOSCOMP:
				return "twoscomp";
			case NegativeMapping::ONESCOMP:
				return "onescomp";
			case NegativeMapping::SIGNBIT:
				return "signbit";
			case NegativeMapping::EXCESSKBIASED:
				return "excessk_biased";
			default:
				assert(0);
		}
		return 0;
	}
	std::string get_signature() const{
		return std::string();
	}
	std::string generate_read_code(bool use_exceptions) const;
};

class ArrayLength{
public:
	virtual ~ArrayLength(){}
	virtual const char *get_length_word() const = 0;
	virtual std::string generate_length_parameter() const = 0;
};

class FixedArrayLength : public ArrayLength{
public:
	std::string length;
	FixedArrayLength(){}
	FixedArrayLength(const std::string &length): length(length){}
	const char *get_length_word() const{
		return "sized";
	}
	std::string generate_length_parameter() const{
		std::string ret(", ");
		ret.append(this->length);
		return ret;
	}
};

class CStyleArrayLength: public ArrayLength{
public:
	const char *get_length_word() const{
		return "cstyle";
	}
	std::string generate_length_parameter() const{
		return std::string();
	}
};

class NamedArrayLength : public ArrayLength{
public:
	NamedArrayLength(){}
	NamedArrayLength(const std::string &name): name(name){}
	std::string name;
	virtual ~NamedArrayLength(){}
};

class PrestatedArrayLength : public NamedArrayLength{
public:
	PrestatedArrayLength(){}
	PrestatedArrayLength(const std::string &name): NamedArrayLength(name){}
	const char *get_length_word() const{
		return "sized";
	}
	std::string generate_length_parameter() const{
		std::string ret(", ");
		ret.append(this->name);
		return ret;
	}
};

class UserArrayLength : public NamedArrayLength{
public:
	UserArrayLength(){}
	UserArrayLength(const std::string &name): NamedArrayLength(name){}
	const char *get_length_word() const{
		return "user_length";
	}
	std::string generate_length_parameter() const{
		std::string ret(", ");
		ret.append(this->name);
		return ret;
	}
};

class DefinedString : public DefinedDatum{
	boost::shared_ptr<ArrayLength> length;
public:
	DefinedString(): DefinedDatum(DataType::STRING){}
	DefinedString(tinyxml2::XMLElement *);
	bool validate(){
		return !!this->length.get();
	}
	void set_length(ArrayLength *length){
		this->length.reset(length);
	}
	void set_length(boost::shared_ptr<ArrayLength> &length){
		this->length = length;
	}
	std::string get_signature() const{
		return "std::string " + this->name;
	}
	std::string generate_read_code(bool use_exceptions) const;
};

class DefinedArray : public DefinedDatum{
	boost::shared_ptr<DefinedDatum> type;
	boost::shared_ptr<ArrayLength> length;
public:
	DefinedArray(tinyxml2::XMLElement *);
	bool validate(){
		return !!this->length.get();
	}
	void set_length(ArrayLength *length){
		this->length.reset(length);
	}
	void set_length(boost::shared_ptr<ArrayLength> &length){
		this->length = length;
	}
	std::string get_signature() const{
		return "std::string " + this->name;
	}
	std::string generate_read_code(bool use_exceptions) const;
};

class ParserState;

class DefinedType{
	std::vector<std::string> namespaces;
	std::string name;
	std::vector<boost::shared_ptr<DefinedDatum> > data;
	void parse(tinyxml2::XMLElement *, ParserState &);
public:
	DefinedType(){}
	DefinedType(tinyxml2::XMLElement *, ParserState &);
	void add_datum(const boost::shared_ptr<DefinedDatum> &datum){
		this->data.push_back(datum);
	}
	void set_namespace(const std::vector<std::string> &ns){
		this->namespaces = ns;
	}
	void set_name(const std::string &name){
		this->name = name;
	}
	std::string generate_declaration(bool use_exceptions) const;
	std::string generate_definition(bool use_exceptions) const;
};

class ParserState{
public:
	IntegerFormat current_format;
	enum class BlockType{
		NONE,
		UNSPECIFIC,
		NAMESPACE,
		TYPE,
	} current_block;
	std::vector<std::string> current_namespace; // [] == global namespace
	boost::shared_ptr<DefinedType> current_type;
	boost::shared_ptr<DefinedDatum> current_datum;
	boost::shared_ptr<Requirement> current_requirement;
	boost::shared_ptr<ArrayLength> current_length;
	ParserState(): current_block(BlockType::NONE){}
};

class Parser{
public:
	enum class MetaParserStatus{
		SUCCESS,
		CONTINUE,
		UNKNOWN_XML_ERROR,
		FILE_NOT_FOUND,
		FILE_ERROR,
		SYNTAX_ERROR,
		EXTRANEOUS_END,
		EXPECTED_IDENTIFIER,
		INVALID_IDENTIFIER,
		UNKNOWN_TOKEN,
		EXPECTED_RELATIONAL_OPERATOR,
		EXPECTED_VALUE,
		COULD_NOT_UNDERSTAND_VALUE,
		DATUM_NOT_PROPERLY_DEFINED,
		EXPECTED_LENGTH_SPECIFICATION,
		INVALID_LENGTH_SPECIFICATION,
		EXPECTED_LENGTH_VALUE,
		EXPECTED_LENGTH_NAME,
		REQUIRE_ONLY_FOR_SIMPLE_VALUES,
		MALFORMED_XML_STRUCTURE,
		INVALID_FORMAT_SPECIFIER,
	};
private:
	ParserState state;
	std::vector<ParserState> stack;
	std::vector<boost::shared_ptr<DefinedType> > types;

	struct TCO;
	typedef MetaParserStatus (Parser::*tail_call_optimized_function)(std::istream &, TCO &);
	struct TCO{
		tail_call_optimized_function f;
	};
	typedef MetaParserStatus (Parser::*end_of_line_function)();

	std::map<std::string, FirstLineWord> first_line_words;
	std::map<std::string, FormatWord> formats;
	std::map<std::string, BeginWord> begins;
	std::map<std::string, LengthTypeWord> length_type_words;
	void init_maps();
	MetaParserStatus pop_state();
	end_of_line_function eol_function;

	MetaParserStatus add_datum_to_type();

#define DECLARE_STATE(name) MetaParserStatus name(std::istream &stream, TCO &tco)
	DECLARE_STATE(line_start);
	DECLARE_STATE(format);
	DECLARE_STATE(begin);
	DECLARE_STATE(end);
	DECLARE_STATE(member_name);
	DECLARE_STATE(namespace_name);
	DECLARE_STATE(type_name);
	DECLARE_STATE(member_name_done);
	DECLARE_STATE(require_operator);
	DECLARE_STATE(require_value);
	DECLARE_STATE(length_start);
	DECLARE_STATE(length_value);
	DECLARE_STATE(length_name);

	void parse(tinyxml2::XMLElement *, ParserState &);
public:
	Parser();
	MetaParserStatus operator<<(std::istream &stream);
	std::string generate_declarations(bool use_exceptions) const{
		std::string ret;
		for (auto &t : this->types){
			ret.append(t->generate_declaration(use_exceptions));
			ret.append("\n");
		}
		return ret;
	}
	std::string generate_definitions(bool use_exceptions) const{
		std::string ret;
		for (auto &t : this->types){
			ret.append(t->generate_definition(use_exceptions));
			ret.append("\n");
		}
		return ret;
	}
	MetaParserStatus parse_specification(const char *file_path);
	Parser::MetaParserStatus load_xml(const char *file_path);
};
