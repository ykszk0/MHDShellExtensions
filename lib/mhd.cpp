#include "mhd.h"
#include <fstream>
#include <vector>

bool start_with(const char* str, const char* s)
{
  if (strlen(str) < strlen(s)) {
    return false;
  }
  for (int i = 0; i < strlen(s); ++i) {
    if (str[i] != s[i]) {
      return false;
    }
  }
  return true;
}

namespace
{
constexpr int max_header_size = 4096;

  std::map<std::string,int> type2index = {
    {"MET_CHAR",ParserBase::CharIcon}, 
    {"MET_UCHAR",ParserBase::UCharIcon}, 
    {"MET_SHORT",ParserBase::ShortIcon}, 
    {"MET_USHORT",ParserBase::UShortIcon}, 
    {"MET_INT",ParserBase::IntIcon},
    {"MET_UINT",ParserBase::UIntIcon}, 
    {"MET_LONG",ParserBase::LongIcon}, 
    {"MET_ULONG",ParserBase::ULongIcon}, 
    {"MET_FLOAT",ParserBase::FloatIcon},
    {"MET_DOUBLE",ParserBase::DoubleIcon} 
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
  bool isRGBImage(const header_map_type &header)
  {
    auto it = header.find("ElementType");
    if (it != header.end()) {
      auto it2 = header.find("ElementNumberOfChannels");
      if (it2 != header.end()) {
        if (it->second == "MET_UCHAR" && it2->second == "3") {
          return true;
        }
      }
    }
    return false;
  }

}

template <typename T>
std::string read_header_t(const T* filename)
{
  char header[max_header_size];
  char *line = header;
  std::ifstream ifs(filename, std::ios::in | std::ios::binary);
  if (ifs.fail()) {
    throw std::runtime_error(std::string("Failed to open file"));
  }
  for (int i = 0; i < max_header_size - 1 && !ifs.eof(); ++i) {
    ifs.get(header[i]);
    if (header[i] == '\0') {
      header[i] = '_';
    }
    if (header[i] == '\n' || ifs.eof()) {
      header[i + 1] = '\0';
      if (start_with(line, "ElementDataFile"))
      {
        return header;
      } else {
        line = header + i + 1;
      }
    }
  }
  if (ifs.eof()) {
    throw std::runtime_error(std::string("Invalid header : Unexpected end of file"));
  } else {
    throw std::runtime_error(std::string("Invalid header : Too large"));
  }
  return "Unreachable";
}

std::string read_header(IStream * pstream)
{
  char header[max_header_size];
  char *line = header;
  ULONG cbRead;
  HRESULT result = S_OK;
  for (int i = 0; i < max_header_size - 1; ++i) {
    result = pstream->Read(header + i, 1, &cbRead);
    if (result == S_FALSE) {
      header[i] = '\0';
      if (start_with(line, "ElementDataFile"))
      {
        return header;
      } else {
        throw std::runtime_error(std::string("Invalid header : Unexpected end of file"));
      }
    }
    if (header[i] == '\0') {
      header[i] = '_';
    }
    if (header[i] == '\n') {
      header[i + 1] = '\0';
      if (start_with(line, "ElementDataFile"))
      {
        return header;
      } else {
        line = header + i + 1;
      }
    }
  }
  throw std::runtime_error(std::string("Invalid header : Too large"));
  return "Unreachable";
}

std::string read_header(const char * filename)
{
  return read_header_t(filename);
}
std::string read_header(const wchar_t * filename)
{
  return read_header_t(filename);
}

std::string Mhd::get_text_representation()
{
  if (header_.empty()) {
    return "Empty file";
  }
  else {
    return header_;
  }
}

std::map<std::string, std::string> parse_header(std::string header)
{
  auto map = std::map<std::string, std::string>();
  int start = 0;
  for (int i = 0; i < header.size(); ++i) {
    if (header[i] == ' ') {
      header[i] = '\0';
      std::string key = header.c_str() + start;
      i += 3; //skip "= "
      start = i;
      for (; i < header.size(); ++i) {
        if (header[i] == '\n') {
          int end = i-1;
          if (i > start && header[i - 1] == '\r') {
            header[i - 1] = '\0';
            --end;
          } else {
            header[i] = '\0';
          }
          // delete spaces at the end
          for (; end > start; --end) {
            if (header[end] != ' ') {
              break;
            }
            header[end] = '\0';
          }
          std::string value = header.c_str() + start;
          map.insert(std::make_pair(key, value));
          break;
        }
      }
      start = i + 1;
    }
  }
  return map;
}
Mhd::Mhd()
{
  ext_set_ = {TEXT(".mhd"), TEXT(".mha")};
}

void Mhd::read_header(const TCHAR * filename)
{
  header_ = ::read_header(filename);
}

void Mhd::read_header(const char * filename)
{
  header_ = ::read_header(filename);
}

void Mhd::parse_header()
{
  map_ = ::parse_header(header_);
}

int Mhd::get_icon_index()
{
  if (isRGBImage(map_)) {
    return ParserBase::RGBIcon;
  }
  auto it = map_.find("ElementType");
  if (it != map_.end()) {
    return ElementType2Index(it->second);
  } else {
    return ParserBase::UnknownIcon;
  }
}
