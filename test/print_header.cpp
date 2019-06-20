#include <iostream>
#include <mhd.h>
using namespace std;

int main(int argc, char *argv[])
{
  if (argc <= 1) {
    cerr << "Usage:" << argv[0] << " <filename>" << endl;
    return 1;
  }
  for (int i = 1; i < argc; ++i) {
    string filename = argv[i];
    try {
      auto header = read_header(filename.c_str());
      cout << header << endl;
      auto parsed = parse_header(header);
      cout << parsed.size() << endl;
    } catch (std::exception &e) {
      cerr << e.what() << endl;
      return 1;
    }
  }
  return 0;
}
