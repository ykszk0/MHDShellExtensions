#include <iostream>
#include <mhd.h>

using namespace std;
int main(int argc, char* argv[]) {

  if (argc != 2) {
    cout << "Usage:" << argv[0] << "<filename>" << endl;
    return 1;
  }
  try {
    auto header = read_header(argv[1]);
  } catch (exception &e) {
    return 0;
  }
  return 1;
}
