#ifndef MHD_H
#define MHD_H
#include "libparse.h"
#include <Windows.h>

std::string read_header(const char* filename);
std::string read_header(IStream *pstream);
std::string read_header(const wchar_t* filename);

header_map_type parse_header(std::string header);

class Mhd : public ParserBase
{
public:
  Mhd();
  ~Mhd() = default;
  void read_header(const TCHAR* filename);
  void read_header(const char* filename);
  void parse_header();
  int get_icon_index();
};

#endif /* MHD_H */
