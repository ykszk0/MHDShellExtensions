#include "nrrd.h"
#include <iostream>
using namespace std;

int main(int argc, char* argv[])
{
  if (argc != 2) {
    cout << "Usage:" << argv[0] << " <filename>" << endl;
    return 1;
  }
//  setlocale(LC_ALL, "");
  cout << argv[1] << endl;
  std::wstring wc(256, 'a');
  mbstowcs(&wc[0], argv[1], strlen(argv[1]) + 2);
  wcout << wc.c_str() << endl;
  for (int ii = 0; ii < 100; ++ii) {
    try {
      auto parser = new Nrrd();
      //wcout << parser->get_file_extension(wc.c_str()) << endl;
      //cout << parser->check_file_extension(wc.c_str()) << endl;
      //wcout << "read" << endl;
      parser->read_header(wc.c_str());
      //parser->read_header(TEXT("Brain.nrrd"));
      //cout << "parse" << endl;
      parser->parse_header();
      auto parsed = parser->get_map();
      //cout << "size    " << parsed.size() << endl;
      //for (auto it = parsed.begin(); it != parsed.end(); ++it) {
      //  cout << it->first << ":" << it->second << endl;
      //}
      //cout << endl;
      //cout << "icon index " << parser->get_icon_index();
      auto map = parser->get_map();
      auto it = map.find("type");
      if (it != map.end()) {
        cout << it->second << endl;
      }

    } catch (exception &e) {
      cerr << e.what() << endl;
      return 1;
    }
  }
    cout << "done" << endl;
    return 0;
}
