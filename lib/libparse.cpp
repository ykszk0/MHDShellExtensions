#include "libparse.h"
#include "libparse.h"
#include "libparse.h"
#include <tchar.h>

const TCHAR * ParserBase::get_file_extension(const TCHAR * filename)
{
  int i = _tcsclen(filename) - 1;
  for (; i >= 0; --i) {
    if (filename[i] == '.') {
      break;
    }
  }
  return filename + i;
}

const std::string & ParserBase::get_header()
{
  return header_;
}

const header_map_type & ParserBase::get_map()
{
  return map_;
}

bool ParserBase::check_file_extension(const TCHAR* filename)
{
  auto ext = ParserBase::get_file_extension(filename);
  for (int i = 0; i < sizeof(ext_set_); ++i) {
    if (_tcscmp(ext, ext_set_[i])) {
      return true;
    }
  }
  return false;
}

