#include <iostream>
#include <cmath>
#include <map>
#include <vector>
#include "line.h"
#include "math_eval.h"

using namespace std;

int main() {
  __MATH_EVAL_DEBUG = 1;  // show debug message
  string input;
  Node * node;
  while (input[0] != 'q') {
    cout << "enter your math expression: " << endl;
    getline(cin, input);
    cout << "your input is: " << input << endl;
    node = ParseExpression(input.c_str());
    PrintTokenTree(node);
    cout << endl;
  }
  return 0;
}
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
