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
  virtual void read_header(const char* filename);
  virtual void parse_header();
  ~Mhd() = default;
};

#endif /* MHD_H */
