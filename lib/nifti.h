#ifndef NIFTI_H
#define NIFTI_H
#include "libparse.h"
#include <Windows.h>
#include "nifti1.h"

namespace nifti
{
  std::string read_header(const TCHAR* filename);
  std::string read_header(const char* filename);
//  std::string read_header(IStream *pstream);
//  std::string read_header(const wchar_t* filename);

  header_map_type parse_header(std::string header);
}

class Nifti : public ParserBase
{
public:
  Nifti();
  ~Nifti() = default;
  void read_header(const TCHAR* filename);
  void read_header(const char* filename);
  void parse_header();
  std::string get_text_representation();
  int get_icon_index();
private:
  nifti_1_header header_struct_;
};

#endif /* NIFTI_H */
