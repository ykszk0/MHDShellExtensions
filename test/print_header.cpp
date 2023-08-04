#include <iostream>
#include <mhd.h>
#include <nrrd.h>
#include <nifti.h>
#include <libparse.h>

using namespace std;

int wmain(int argc, TCHAR *argv[])
{
  if (argc != 2) {
    wcerr << L"Usage:" << argv[0] << L" <filename>" << endl;
    return 1;
  }
  for (int i = 1; i < argc; ++i) {
    auto filename = argv[1];
    auto parser = ParserBase::select_parser(filename);
    if (!parser) {
      wcerr << L"No parser found" << endl;
      return 1;
    }
    try {
      parser->read_header(filename);
      cout << parser->get_text_representation() << endl;
    } catch (std::exception &e) {
      wcerr << e.what() << endl;
      return 1;
    }
  }
  return 0;
}
