#include "libmhd.h"
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
constexpr int max_header_size = 2048;
constexpr int max_line_size = 512;
}
std::string read_header(const char* filename)
{
  int header_size = 0;
  char header[max_header_size];
  header[0] = '\0';
  char buf[max_line_size];
  std::ifstream ifs(filename);
  if (ifs.fail()) {
    throw std::runtime_error(std::string("Failed to open file : ") + filename);
  }
  for (int nol = 0; !ifs.eof(); ++nol) {
    buf[0] = '\0';
    for (int i = 0; i < max_line_size-1 && !ifs.eof(); ++i) {
      ifs.get(buf[i]);
      if (buf[i] == '\n') {
        buf[i + 1] = '\0';
        break;
      }
      if (++header_size > max_header_size) {
        throw std::runtime_error(std::string("Invalid header : Too large"));
      }
    }
    strcat(header, buf);
    if (start_with(buf, "ElementDataFile"))
    {
      return header;
    }
  }
  throw std::runtime_error(std::string("Invalid header : End of file"));
  return "Unreachable";
}

std::string read_header(IStream * pstream)
{
  int header_size = 0;
  char header[max_header_size];
  header[0] = '\0';
  char buf[max_line_size];
  char *buf_ptr = buf;
  ULONG cbRead;
  HRESULT result = S_OK;
  while (result != S_FALSE) {
    buf[0] = '\0';
    for (int i = 0; i < max_line_size-1; ++i) {
      result = pstream->Read(buf+i, 1, &cbRead);
      if (result == S_FALSE || buf[i] == '\n') {
        buf[i + 1] = '\0';
        break;
      }
      if (++header_size > max_header_size) {
        throw std::runtime_error(std::string("Invalid header : Too large"));
      }
    }
    strcat(header, buf);
    if (start_with(buf, "ElementDataFile"))
    {
      return header;
    }
  }
  throw std::runtime_error(std::string("Invalid header : End of file"));
  return "Unreachable";
}

std::string read_header(const wchar_t * wfilename)
{
  constexpr int buf_size = 4096;
  std::vector<char> filename(buf_size);
  wcstombs(filename.data(), wfilename, buf_size);
  return read_header(filename.data());
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
