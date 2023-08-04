#include "nrrd.h"
#include <fstream>
#include <set>
#include <tchar.h>
// http://teem.sourceforge.net/nrrd/format.html
namespace
{
  std::map<std::string, int> type2index = {
    {"signed char",ParserBase::CharIcon},
    {"int8",ParserBase::CharIcon},
    {"int8_t",ParserBase::CharIcon},

    {"uchar",ParserBase::UCharIcon},
    {"unsigned char",ParserBase::UCharIcon},
    {"uint8",ParserBase::UCharIcon},
    {"uint8_t",ParserBase::UCharIcon},

    {"short",ParserBase::ShortIcon},
    {"short int",ParserBase::ShortIcon},
    {"signed short",ParserBase::ShortIcon},
    {"signed short int",ParserBase::ShortIcon},
    {"int16",ParserBase::ShortIcon},
    {"int16_t",ParserBase::ShortIcon},

    {"ushort",ParserBase::UShortIcon},
    {"unsigned short",ParserBase::UShortIcon},
    {"unsigned short int",ParserBase::UShortIcon},
    {"uint16",ParserBase::UShortIcon},
    {"uint16_t",ParserBase::UShortIcon},

    {"int",ParserBase::IntIcon},
    {"signed int",ParserBase::IntIcon},
    {"int32",ParserBase::IntIcon},
    {"int32_t",ParserBase::IntIcon},

    {"uint",ParserBase::UIntIcon},
    {"unsigned int",ParserBase::UIntIcon},
    {"uint32",ParserBase::UIntIcon},
    {"uint32_t",ParserBase::UIntIcon},

    {"longlong",ParserBase::LongIcon},
    {"long long",ParserBase::LongIcon},
    {"long long int",ParserBase::LongIcon},
    {"signed long long",ParserBase::LongIcon},
    {"signed long long int",ParserBase::LongIcon},
    {"int64",ParserBase::LongIcon},
    {"int64_t",ParserBase::LongIcon},

    {"ulonglong",ParserBase::ULongIcon},
    {"unsigned long long",ParserBase::ULongIcon},
    {"unsigned long long int",ParserBase::ULongIcon},
    {"uint64",ParserBase::ULongIcon},
    {"uint64_t",ParserBase::ULongIcon},

    {"float",ParserBase::FloatIcon},
    {"double",ParserBase::DoubleIcon}
  };
  int ElementType2Index(const std::string &type)
  {
    auto it = type2index.find(type);
    if (it != type2index.end()) {
      return it->second;
    } else {
      return ParserBase::UnknownIcon;
    }
  }
}

namespace nrrd
{
  constexpr int max_header_size = 4096;

  // read file til the terminator. no reccurent pattern is allowed in the terminator string
  template <typename CharType>
  std::string _read_header(const CharType* filename, const char* terminator, bool remove_terminator)
  {
    char header[max_header_size];
    std::ifstream ifs(filename, std::ios::in);
    if (ifs.fail()) {
      throw std::runtime_error(std::string("Failed to open file"));
    }
    auto terminator_size = strlen(terminator);
    int terminator_matched_index = 0;
    // look for terminator
    for (int i = 0; i < max_header_size - 1; ++i) {
      if (ifs.eof()) {
        throw std::runtime_error(std::string("Invalid header : Unexpected end of file"));
      }
      ifs.get(header[i]);
      if (header[i] == '\0') {
        header[i] = '_';
      }
      if (header[i] == terminator[terminator_matched_index]) {
        ++terminator_matched_index;
        if (terminator_matched_index >= terminator_size) {
          if (remove_terminator) {
            header[i - terminator_size + 1] = '\0';
          } else {
            header[i + 1] = '\0';
          }
          return header;
        }
      } else {
        terminator_matched_index = 0;
      }
    }
    throw std::runtime_error(std::string("Invalid header : Too large\n\n"));
    return ""; // unreachable
  }

  template <typename CharType>
  std::string read_header_t(const CharType* filename)
  {
    return _read_header(filename, "\n\n", true);
  }

  std::string read_header(const char* filename)
  {
    return read_header_t(filename);
  }

  std::string read_header(const TCHAR* filename)
  {
    return read_header_t(filename);
  }

  header_map_type::value_type parse_line(char* line, int line_size)
  {
    const char* d1 = ": ";
    const char* d2 = ":=";
    auto delim = strstr(line, d1);
    if (delim == NULL) {
      delim = strstr(line, d2);
      if (delim == NULL) {
        throw std::runtime_error(std::string("Invalid line of the header: No delimiter. ") + line);
      }
    }
    auto index = delim - line;
    delim[0] = '\0';
    return std::make_pair(line, line + index + 2);
  }

  std::map<std::string, std::string> parse_header(std::string header)
  {
    auto map = std::map<std::string, std::string>();
    // validate magic
    const char* magic = "NRRD000";
    if (header.size() < strlen(magic) + 2) {
      throw std::runtime_error(std::string("Invalid header : No magic word"));
    }
    for (int i = 0; i < strlen(magic); ++i) {
      if (header[i] != magic[i]) {
        throw std::runtime_error(std::string("Invalid header : Invalid magic word"));
      }
    }
    char version = header[strlen(magic)];
    if (version < '1' || version > '5') {
      throw std::runtime_error(std::string("Invalid header : Unknown version"));
    }
    map.insert(std::make_pair("version", std::string({ version })));
    // parse header
    int start = strlen(magic) + 2;
    for (int i = start; i < header.size(); ++i) {
      if (header[i] == '\n') {
        header[i] = '\0';
        if (header[start] == '#') {
          // ignore comment
        } else {
          map.insert(parse_line(&header[start], i - start));
        }
        start = i + 1;
      }
    }
    return map;
  }
}

Nrrd::Nrrd()
{
  ext_set_ = {TEXT(".nrrd")};
}

void Nrrd::read_header(const TCHAR * filename)
{
  header_ = nrrd::read_header(filename);
}

void Nrrd::read_header(const char * filename)
{
  header_ = nrrd::read_header(filename);
}

void Nrrd::parse_header()
{
  map_ = nrrd::parse_header(header_);
}

std::string Nrrd::get_text_representation()
{
  return "Not implemented.";
}


int Nrrd::get_icon_index()
{
  auto it = map_.find("type");
  if (it != map_.end()) {
    return ElementType2Index(it->second);
  } else {
    return ParserBase::UnknownIcon;
  }
}