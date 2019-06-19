#ifndef LIBNRRD_H
#define LIBNRRD_H
#include <string>
#include <vector>
#include <map>
#include <Windows.h>

std::string read_header(const char* filename);
std::string read_header(IStream *pstream);
std::string read_header(const wchar_t* filename);

typedef std::map<std::string, std::string> header_map_type;
header_map_type parse_header(std::string header);

#endif /* LIBNRRD_H */
