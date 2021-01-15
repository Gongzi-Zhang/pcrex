#ifndef __MATH_EVAL_H
#define __MATH_EVAL_H
#include <map>
#include <set>
#include <vector>
#include "line.h"
#include "io.h"

using namespace std;

/* Expression Type: 
 *  operator: + - * / %
 *  number:   [+-]?((\d+) | (\d+)?(.\d+))([eE]((\d+) | (\d+)?(.\d+))?
 *  variable: begin with [a-zA-Z_], contains only [0-9a-zA-Z_], can't start with [0-9]
 *  function: same as variable, except that function is always followed by ();
 *  open_prt: open parenthesis
 *  close_prt: close parenthesis
 *  separator: ',' to separate function parameters
 *  name: auxiliary type: combi of variable and function
 *  constant: auxiliary type, some functions can be evaluated in advanced as an optimization
 *  null:   auxiliary type
 */
enum ExpType { 
  // types used when parse input
  opt,    // taboo
  number, 
  name,
  separator, 
  open_prt,
  close_prt, 

  // more types used when check parsed tokens
  variable, // name can be varirable or funciton 
  func,     // taboo
  // function0,  // do I really need it? why not replace it with constant
  function1, function2,  // functions with different parameters

  // other auxiliary types
  constant,
  null,
};
typedef struct { ExpType type; const char * value; } Token;
struct Node { 
  Token token; 
  Node * lchild;
  Node * rchild;
  Node * sibling; // use for hold function parameters
};  // Token Node

Node * ParseExpression(const char *line);
Node * SortToken (vector<Token> &vt);
set<string> GetVariables(Node *node);
double Getf1Value(const char * func, double arg);
double Getf2Value(const char * func, double arg1, double arg2);
const char * GetTypeName(ExpType);
// int GetPriority( const char *opt);
void PrintNode(Node *node);		// print node recursively
void DeleteNode(Node *node);	// delete node recursively

#endif
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
