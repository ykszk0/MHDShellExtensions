#include <iostream>
#include "libmhd.h"
using namespace std;

int main(int argc, char *argv[])
{
  if (argc <= 1) {
    cerr << "Usage:" << argv[0] << " <filename>" << endl;
    return 1;
  }
  string filename = argv[1];
  try {
    cout << read_header(filename.c_str()) << endl;
  } catch (std::exception &e) {
    cerr << e.what() << endl;
    return 1;
  }
  return 0;
}
