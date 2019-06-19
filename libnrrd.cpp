#include "libnrrd.h"
#include <fstream>
#include <iostream>
#include <algorithm>

namespace
{
constexpr int max_header_size = 4096;
constexpr int min_header_size = 9; // NRRD000X\n
}

// read file til terminator
std::string _read_header(const char* filename, const char* terminator, bool remove_terminator)
{
  char header[max_header_size];
  char *line = header;
  std::ifstream ifs(filename, std::ios::in);
  if (ifs.fail()) {
    throw std::runtime_error(std::string("Failed to open file : ") + filename);
  }
  auto terminator_size = strlen(terminator);
  int terminator_matched_index = 0;
  // look for empty line
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
  throw std::runtime_error(std::string("Invalid header : Too large"));
  return "\0"; // unreachable
}

std::string read_header(const char* filename)
{
  return _read_header(filename, "\n\n", true);
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
  if (index >= line_size - 2) {
      throw std::runtime_error(std::string("Invalid line of the header: No 'desc' or 'value'. ") + line);
  }
  delim[0] = '\0';
  return std::make_pair(line, line+index+2);
}

std::map<std::string, std::string> parse_header(std::string header)
{
  auto map = std::map<std::string, std::string>();
  // validate magic
  const char* magic = "NRRD000";
  if (header.size() < strlen(magic)+2) {
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
  int start = strlen(magic)+2;
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
