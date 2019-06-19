#ifndef LIBPARSE_H
#define LIBPARSE_H
#include <map>
#include <string>

typedef std::map<std::string, std::string> header_map_type;

class ParserBase
{
public:
  virtual void read_header(const char* filename) = 0;
  virtual void parse_header() = 0;
  virtual ~ParserBase() = default;
protected:
  std::string header_;
  header_map_type map_;
};
#endif /* LIBPARSE_H */
