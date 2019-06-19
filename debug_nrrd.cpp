#include "libnrrd.h"
#include <iostream>
using namespace std;

int main(int argc, char* argv[])
{
  if (argc != 2) {
    cout << "Usage:" << argv[0] << " <filename>" << endl;
    return 1;
  }
  try {
   auto header = read_header(argv[1]);
   cout << header;
   cout << endl;
   cout << "parse" << endl;
   auto parsed = parse_header(header);
   cout << "size    " << parsed.size() << endl;
   for (auto it = parsed.begin(); it != parsed.end(); ++it) {
     cout << it->first << ":" << it->second << endl;
   }
  } catch (exception &e) {
    cerr << e.what() << endl;
    return 1;
  }
  return 0;
}
