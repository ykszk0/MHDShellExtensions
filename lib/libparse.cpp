#include "libparse.h"
#include <tchar.h>
#include "mhd.h"
#include "nrrd.h"
#include "nifti.h"

const TCHAR * ParserBase::get_file_extension(const TCHAR * filename)
{
  int i = _tcsclen(filename) - 1;
  TCHAR *ext;
  for (; i >= 0; --i) {
    if (filename[i] == '.') {
      if (_tcscmp(filename + i, TEXT(".gz")) == 0) {
        continue;
      }
      break;
    }
  }
  return filename + max(i, 0);
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
  for (const auto &target_ext : ext_set_) {
    if (_tcscmp(ext, target_ext.c_str())==0) {
      return true;
    }
  }
  return false;
}

template <typename TYPE, std::size_t SIZE>
std::size_t static_array_length(const TYPE(&array)[SIZE])
{
  return SIZE;
}

std::unique_ptr<ParserBase> ParserBase::select_parser(const TCHAR* filename) {
  std::unique_ptr<ParserBase> parsers[] = { std::make_unique<Mhd>(), std::make_unique<Nrrd>(), std::make_unique<Nifti>() };
  try {
    auto ext = ParserBase::get_file_extension(filename);
    for (int i = 0; i < static_array_length(parsers); ++i) {
      if (parsers[i]->check_file_extension(ext)) {
        return std::move(parsers[i]);
      }
    }
    return nullptr;
  }
  catch (std::exception& e) {
    return nullptr;
  }
}

