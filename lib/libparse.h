#ifndef LIBPARSE_H
#define LIBPARSE_H
#include <map>
#include <string>
#include <vector>
#include <memory>
#include <Windows.h>

typedef std::map<std::string, std::string> header_map_type;

class ParserBase
{
public:
  static std::unique_ptr<ParserBase> select_parser(const TCHAR* filename);
  bool check_file_extension(const TCHAR* filename);
  virtual void read_header(const TCHAR* fileyname) = 0;
  virtual void read_header(const char* fileyname) = 0;
  virtual void parse_header() = 0;
  virtual int get_icon_index() = 0;
  virtual ~ParserBase() = default;
  static const TCHAR* get_file_extension(const TCHAR* filename);
  const std::string& get_header();
  virtual std::string get_text_representation() = 0;
  const header_map_type& get_map();

  enum IconIndex : int
  {
    UnknownIcon = -100,
    CharIcon = -101,
    UCharIcon = -102,
    ShortIcon = -103,
    UShortIcon = -104,
    IntIcon = -105,
    UIntIcon = -106,
    LongIcon = -107,
    ULongIcon = -108,
    FloatIcon = -109,
    DoubleIcon = -110,
    RGBIcon = -111,
  };
protected:
  std::string header_;
  std::vector<std::basic_string<TCHAR>> ext_set_;
  header_map_type map_;
};
#endif /* LIBPARSE_H */
