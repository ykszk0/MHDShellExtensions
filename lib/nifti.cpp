#include "nifti.h"
#include <set>
#include <tchar.h>
#include <fcntl.h>
#include <io.h>
#include <zlib.h>
#include <memory>
#include <fstream>
#include <sstream>

namespace
{

  constexpr int nifti1_header_size = 348;
  constexpr int nifti2_header_size = 540;
  std::map<short, int> type2index = {
    {0, ParserBase::UnknownIcon},
    {1, ParserBase::UCharIcon}, // bool
    {2, ParserBase::UCharIcon},
    {4, ParserBase::ShortIcon},
    {8, ParserBase::IntIcon},
    {16, ParserBase::FloatIcon},
    {32, ParserBase::UnknownIcon}, // complex
    {64, ParserBase::DoubleIcon},
    {128, ParserBase::RGBIcon},
    {255, ParserBase::UnknownIcon}, // "all"
    {256, ParserBase::CharIcon},
    {512, ParserBase::UShortIcon},
    {768, ParserBase::UIntIcon},
    {1024, ParserBase::LongIcon},
    {1280, ParserBase::ULongIcon},
    {1536, ParserBase::UnknownIcon}, // long double
    {1792, ParserBase::UnknownIcon}, // double pair
    {2048, ParserBase::UnknownIcon}, // long double pair
    {2304, ParserBase::UnknownIcon}, // RGBA
  };

  int ElementType2Index(short type)
  {
    auto it = type2index.find(type);
    if (it != type2index.end()) {
      return it->second;
    } else {
      return ParserBase::UnknownIcon;
    }
  }
}

namespace nifti
{
  void read_header(char *buf, nifti_1_header *header)
  {
    int32_t header_size;
    memcpy(&header_size, buf, 4);
    if (header_size != nifti1_header_size) {
      throw std::runtime_error(std::string("Invalid header size"));
    }
    memcpy(header, buf, sizeof(nifti_1_header));
  }

  template <typename CharType>
  void read_header_t(const CharType* filename, nifti_1_header *header)
  {
    if (_tcscmp(ParserBase::get_file_extension(filename), TEXT(".nii.gz"))==0) {
      read_header_gz_t(filename, header);
    } else {
      std::ifstream ifs(filename, std::ios_base::in|std::ios_base::binary);
      if (!ifs) {
        throw std::runtime_error(std::string("Failed to open file!"));
      }
      char buf[nifti1_header_size];
      ifs.read(buf, nifti1_header_size);
      if (ifs.gcount() < nifti1_header_size) {
        throw std::runtime_error(std::string("The file is too small"));
      }
      read_header(buf, header);
    }
  }

  template <typename CharType>
  void read_header_gz_t(const CharType* filename, nifti_1_header *header)
  {
    auto fd = _topen(filename, _O_RDONLY | _O_BINARY);
    if (fd == -1) {
      throw std::runtime_error(std::string("Failed to open file"));
    }
    auto gzf = gzdopen(fd, "rb");
    if (gzf == NULL) {
      _close(fd);
      throw std::runtime_error(std::string("Failed to open file"));
    }
    constexpr int buf_size = nifti2_header_size;
    char buf[buf_size];

    auto bytes_read = gzread(gzf, buf, buf_size);
    if (bytes_read < nifti1_header_size) {
      gzclose(gzf);
      throw std::runtime_error(std::string("The file is too small"));
    }
    read_header(buf, header);
    gzclose(gzf);
  }

  //std::string read_header(const char* filename)
  //{
  //  return "";
  //}

  std::string read_header(const TCHAR* filename)
  {
    return "";
  }
}

Nifti::Nifti()
{
  ext_set_ = {TEXT(".nii"), TEXT(".nii.gz")};
}

void Nifti::read_header(const TCHAR * filename)
{
  nifti::read_header_t(filename, &header_struct_);
}

void Nifti::read_header(const char * filename)
{
  //nifti::read_header_t(filename, &header_struct_);
}

#define DT_CASE(TYPE) case DT_##TYPE: return #TYPE;
std::string datatype2str(short datatype) {
  switch (datatype) {
  case DT_NONE:
    return "None";
    DT_CASE(BINARY);
    DT_CASE(UINT8);
    DT_CASE(INT8);
    DT_CASE(UINT16);
    DT_CASE(INT16);
    DT_CASE(INT32);
    DT_CASE(UINT32);
    DT_CASE(INT64);
    DT_CASE(UINT64);
    DT_CASE(FLOAT64);
    DT_CASE(FLOAT128);
    DT_CASE(COMPLEX64);
    DT_CASE(COMPLEX128);
    DT_CASE(COMPLEX256);
    DT_CASE(FLOAT32);
    DT_CASE(RGB24);
    DT_CASE(RGBA32);

  default:
    return "Unknown";
  }
}

#define NIFTI_UNITS_CASE(UNIT) case NIFTI_UNITS_##UNIT: return #UNIT;
std::string unit2str(short datatype) {
  switch (datatype) {
  case NIFTI_UNITS_UNKNOWN:
    return "Unknown";
    NIFTI_UNITS_CASE(METER);
    NIFTI_UNITS_CASE(MM);
    NIFTI_UNITS_CASE(MICRON);
    NIFTI_UNITS_CASE(SEC);
    NIFTI_UNITS_CASE(MSEC);
    NIFTI_UNITS_CASE(USEC);
    NIFTI_UNITS_CASE(HZ);
    NIFTI_UNITS_CASE(PPM);
    NIFTI_UNITS_CASE(RADS);
  default:
    return "Unknown";
  }
}


template <typename T>
std::string array2str(T arr[], char* delim) {
  std::stringstream ss;
  std::copy(arr, arr + sizeof(arr),
    std::ostream_iterator<T>(ss, delim));
  return ss.str();
}

std::string Nifti::get_text_representation()
{
  std::stringstream ss;
  ss << std::string("Pixel type: ") + datatype2str(header_struct_.datatype) << '\n';
  ss << std::string("Array size: ") + array2str(header_struct_.dim, ", ") << '\n';
  ss << std::string("Pixel size: ") + array2str(header_struct_.pixdim, ", ") << '\n';
  ss << std::string("Pixel spatial unit: ") + unit2str(header_struct_.xyzt_units) << '\n';
  ss << std::string("Intent name: ") + header_struct_.intent_name << '\n';
  ss << std::string("Description: ") + header_struct_.descrip << '\n';
  return ss.str();
}


int Nifti::get_icon_index()
{
  return ElementType2Index(header_struct_.datatype);
}

void Nifti::parse_header()
{
  header_map_type map;
  map.insert(std::make_pair("DimSize", array2str(header_struct_.dim, " ")));
  map.insert(std::make_pair("ElementType", datatype2str(header_struct_.datatype)));
  map.insert(std::make_pair("ElementSpacing", array2str(header_struct_.pixdim, " ")));
  map_ = map;
}

