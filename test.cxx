#include <iostream>
#include "line.h"
#include "TRun.h"
#include "TConfig.h"

using namespace std;

void sub(char * line) {
  vector<char *> v1 = Split(line, ';');
}

int main() {
  StringTests();
  RunTests();

  TConfig fConf("check.conf");
  fConf.ParseConfFile();
}
