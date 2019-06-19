#ifndef NRRD_H
#define NRRD_H
#include "libparse.h"
#include <Windows.h>

namespace nrrd
{
  std::string read_header(const char* filename);
//  std::string read_header(IStream *pstream);
//  std::string read_header(const wchar_t* filename);

  header_map_type parse_header(std::string header);
}

class Nrrd : public ParserBase
{
public:
  virtual void read_header(const char* filename);
  virtual void parse_header();
  ~Nrrd() = default;
};

#endif /* NRRD_H */
