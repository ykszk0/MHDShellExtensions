#ifndef NRRD_H
#define NRRD_H
#include "libparse.h"
#include <Windows.h>

namespace nrrd
{
  std::string read_header(const TCHAR* filename);
  std::string read_header(const char* filename);
//  std::string read_header(IStream *pstream);
//  std::string read_header(const wchar_t* filename);

  header_map_type parse_header(std::string header);
}

class Nrrd : public ParserBase
{
public:
  Nrrd();
  ~Nrrd() = default;
  void read_header(const TCHAR* filename);
  void read_header(const char* filename);
  void parse_header();
  std::string get_text_representation();
  int get_icon_index();
};

#endif /* NRRD_H */
