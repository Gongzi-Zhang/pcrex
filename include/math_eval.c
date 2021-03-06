#include <iostream>
#include <cstring>
#include <cmath>
#include <map>
#include <set>
#include <vector>
#include "line.h"
#include "io.h"
#include "math_eval.h"

using namespace std;

int __MATH_EVAL_DEBUG = 0;

map<ExpType, const char*> TypeName {
  {opt,       "operator"},
  {number,    "number"},
  {name,      "name"},
  {separator, "separator"},
  {open_prt,  "open_prt"},
  {close_prt, "close_prt"},
  {variable,  "variable"},
  {func,      "function"},
  {function1, "function1"},
  {function2, "function2"},
  {constant,  "constant"},
  {null,      "null"},
};

map<string, double (*) ()> f0 = {  // function with 0 parameter
};
map<string, double (*) (double)> f1 = {  // function with 1 parameter
	{"abs",		&abs},
  {"sqrt", &sqrt},
};
map<string, double (*) (double, double)> f2 = {  // function with 1 parameter
  {"pow", &pow},
};


Node * ParseExpression(const char *line) {
  if (line == NULL) {
    cerr << ERROR << "NULL input" << ENDL;
    return NULL;
  }

  vector<Token> result;
  // get tokens
  Token t = {null, ""};   // current token
  Token pt = {null, ""};  // previous token
  int pi = -1;  // previous token open index
  int i = 0;    // current char index
  bool eflag = false; // exponent part flag
  while (line[i] != '\0') {
    switch (line[i]) {
      // single char token, which is also a separator that closes previous token
      case '+':
      case '-':
        if (eflag) {  // single operator in number expression
          break;
        }
      case '*':
      case '/':
      case '%':
        if (t.type == null)
          t = {opt, Sub(line, i, 1)};
      case '(':
        if (t.type == null)
          t = {open_prt, Sub(line, i, 1)};
      case ')':
        if (t.type == null)
          t = {close_prt, Sub(line, i, 1)};
      case ',':
        if (t.type == null)
          t = {separator, Sub(line, i, 1)};
      case ' ':
      case '\t':
      case '\n':
        if (pi != -1) {
          pt.value = Sub(line, pi, i-pi);
          result.push_back(pt);
          pt = {null, ""};
          pi = -1; // close a token
          eflag = false;
        }
        if (t.type != null) {
          result.push_back(t);
          t = {null, ""};
        }
        break;
      // numbers
      case '.': // dot can only show up in numbers or names
        // if (pi != -1 && pt.type != number && pt.type != name) {
        //   cerr << ERROR << "dot can't be part of anything except number or name" <<  ENDL;
        //   return NULL;
        // }
      case '0' ... '9':   // case range of value, this is an extension of gcc, not portable
        if (pi == -1) {
          pt.type = number;
          pi = i;
        }
        break;
      // name
      case '_':
      case 'a' ... 'z':
      case 'A' ... 'Z':
        if (pi == -1) {
          pt.type = name;
          pi = i;
        } else if ( (line[i] == 'e' || line[i] == 'E') && pt.type == number) {  // exponent part within a number
          eflag = true;
        }
        break;
      // other undefined chars
      default:
        cerr << ERROR << "unknown char in math expression: " << line[i] << ENDL;
        return NULL;
    }
    i++;
  }
  if (pi != -1) { // the last token
    pt.value = Sub(line, pi, i-pi);
    result.push_back(pt);
    pt = {null, ""};
    pi = -1; // close a token
  }

  if (__MATH_EVAL_DEBUG) {
    for (int i = 0; i<result.size(); i++)
      printf("%d\t%-10s\t%s\n", i+1, TypeName[result[i].type], result[i].value);
		cout << endl;
  }

	/* token value check: currently not seperated from type check
   * number token can have at most 1 dot, 1 [eE], no other alphabets
	 * dot is not allowed in the exponential part of a number
	 * variables can have at most 2 dots
	 */

  /* token type check 
   * operator token must be surrounded by other types of token (except +-): 
   *    * operator token can't be the first/last token (except +-)
   *    * no consecutive operator tokens: a + -b is also not allowed, write it as a + (-b)
   *    * operator can't be followed by separator
   *    * +- can be single operator, which must be followed by numbers
   * open_prt must have corresponding close_prt
   * close_prt can only be followed by separator, operator or another close_prt;
   * number can only be followed by separator, close_prt, operator
   * functions are followed by (
   * function must be one of f1 or f2
   * separator can only appears within () of a function
   */
  int nfunc = 0; // number of functions
  int nprt = 0;
  vector<vector<Token>::iterator> it_f;   // position of each function
  vector<int> param;      // number of parameters of each function
  vector<int> param_buf;  // vector buffer to store temp functions' position in it_f
  vector<int> prt_type;   // 1: function parenthesis; 0: normal parenthesis
  t  = {null, ""};
  pt = {null, ""};
  Token nt = {null, ""};  // next token

  for (vector<Token>::iterator it = result.begin(); it != result.end(); it++) {
    t = *it;
    if ( (it+1) != result.end() )
      nt = *(it+1);
    else 
      nt = {null, ""};

		switch (t.type) {
			case opt:
				if (nt.type == null) {
					cerr << ERROR << "expression can't ended with operator: " << t.value 
							 << "\n\t" << line << ENDL;
					return NULL;
				}

				if ( nt.type == opt    // two consecutive operators
						 || nt.type == separator  // operator followed by separator
						 || nt.type == close_prt )  // operator followed by closing parenthesis
				{
					cerr << ERROR << "invalid type following single operator: " << t.value << " " << nt.value 
							 << "\n\t" << line << ENDL;
					return NULL;
				}
				
				// is it single operator: first token, token after , or (
				if (pt.type == null || pt.type == open_prt || pt.type == separator) {
					if ( (strcmp(t.value, "+") != 0 && strcmp(t.value, "-") != 0) )
					{
						cerr << ERROR << "single operator can only be +/- and must be followed by numbers: " << t.value << " " << nt.value
								 << "\n\t" << line << ENDL;
						return NULL;
					}

					// insert 0 before single operator
					it = result.insert(it, {number, "0"});
					it++;
				}
				break;
			case number:
				if (!IsNumber(t.value)) {	// value check
					cerr << ERROR << "invalid number expr: " << t.value
							 << "\n\t" << line << ENDL;
					return NULL;
				}

				if ( nt.type == name 
						 || nt.type == number
						 || (nt.type == separator && nfunc == 0) ) // followed by a separator, but not in a function
				{
					cerr << ERROR << "invalid type following number in: " << t.value << " " << nt.value
							 << "\n\t" << line << ENDL;
					return NULL;
				}
				break;
			case name:
				if ( nt.type == name
						 || nt.type == number )
				{
					cerr << ERROR << "name can't be followed by another name or number " << t.value << " " << nt.value
							 << "\n\t" << line << ENDL;
					return NULL;
				}

				if ( nt.type == open_prt ) {  // function
					if (Contain(t.value, ".")) {
						cerr << ERROR << "function name can't contain dot: " << t.value 
								 << "\n\t" << line << ENDL;
						return NULL;
					}
					it->type = func;
					param_buf.push_back(it_f.size());
					it_f.push_back(it);
					param.push_back(0);
					nfunc++;
				} else {  // variable
					if (Count(t.value, '.') > 2) {	// value check
						cerr << ERROR << "invalid variable expression (at most 2 dots): " << t.value
								 << "\n\t" << line << ENDL;
						return NULL;
					}
					if (nt.type == separator && nfunc == 0) {
						cerr << ERROR << "variable can't be followed by separator outside a funciton: " << t.value << " " << nt.value
								 << "\n\t" << line << ENDL;
						return NULL;
					}
					it->type = variable;
				}
				break;
			case separator:
				if (nfunc == 0) {        // separator outside of a function
					cerr << ERROR << "separator must be within a function "
							 << "\n\t" << line << ENDL;
					return NULL;
				}
				if ( nt.type == separator  // two consecutive separator
						 || nt.type == close_prt
						 || (nt.type == opt && strcmp(nt.value, "+") != 0 && strcmp(nt.value, "-") != 0) ) // non +/- operator
				{
					cerr << ERROR << "invalid type following separator : " << t.value << " " << nt.value
							 << "\n\t" << line << ENDL;
					return NULL;
				}
				param[param_buf.back()]++;
				break;
			case open_prt:
				if ( (nt.type == opt && (strcmp(nt.value, "+") != 0 && strcmp(nt.value, "-") != 0) )
						 || nt.type == separator )
				{
					cerr << ERROR << "invalid type following ( : " << t.value << " " << nt.value
							 << "\n\t" << line << ENDL;
					return NULL;
				}
				if (pt.type == func) {
					prt_type.push_back(1);
				} else {
					prt_type.push_back(0);
				}
				nprt++;
				break;
			case close_prt:
				if (prt_type.size() == 0) {
					cerr << ERROR << "more close prt. ) than opening prt. ( : "
							 << "\n\t" << line << ENDL;
					return NULL;
				}

				if ( nt.type == name
						 || nt.type == open_prt )
				{
					cerr << ERROR << "invalid type following ) : " << t.value << " " << nt.value
							 << "\n\t" << line << ENDL;
					return NULL;
				}

				if (prt_type.back() == 1) {
					param[param_buf.back()]++;
					nfunc--;
					param_buf.pop_back();
				}
				prt_type.pop_back();
				nprt--;
				break;
			default:
				cerr << ERROR << "unknown token type: " << t.type << " of value: " << t.value 
						 << "\n\t" << line << ENDL;
				return NULL;
    }
    pt = *it;
  }

  if (nprt > 0) {
    cerr << ERROR << "unmatched parentheses in expression:"
         << "\n\t" << line << ENDL;
    return NULL;
  }

  if (param.size() != it_f.size()) {
    cerr << ERROR << "something went wrong when parsing functions"
         << "\n\t" << line << ENDL;
    cerr << "\tFunctions: " << endl;
    for (vector<vector<Token>::iterator>::iterator it = it_f.begin(); it != it_f.end(); it++) {
      cerr << "\t\t" << (*it)->value << endl;
    }
    cout << "\tNumber of Parameters: " << endl;
    for (vector<int>::iterator it = param.begin(); it != param.end(); it++) {
      cerr << "\t\t" << *it << endl;
    }
    return NULL;
  }

  for (int i=0; i<it_f.size(); i++) {
    switch (param[i]) {
      /*
      case 0:
        if (f0.find((*(it_f[i])).value) != f0.cend()) {
          (*(it_f[i])).type = function0;
          break;
        }
      */
      case 1:
        if (f1.find((*(it_f[i])).value) != f1.end()) {
          (*(it_f[i])).type = function1;
          break;
        }
      case 2:
        if (f2.find((*(it_f[i])).value) != f2.end()) {
          (*(it_f[i])).type = function2;
          break;
        }
      default:
        cerr << ERROR << "Incorrected # of parameters (" << param[i] << ") for function: " << (*(it_f[i])).value 
             << "\n\t" << line << ENDL;
        return NULL;
    }
  }

  if (__MATH_EVAL_DEBUG) {
		for (int i = 0; i<result.size(); i++)
			printf("%d\t%-10s\t%s\n", i+1, TypeName[result[i].type], result[i].value);
		cout << endl;
  }

  Node * token_tree = SortToken(result);
  // PrintNode(token_tree);
  return token_tree;
}

Node * SortToken (vector<Token> &vt) 
{
  // sort token by operator priority: 
  /* operation priority
   * 1. function/()
   * 2. * / %
   * 3. + -
   */
  if (vt.size() == 0)
    return new Node( {{null, ""}, NULL, NULL, NULL} );

  if (__MATH_EVAL_DEBUG) {
    for (int i = 0; i<vt.size(); i++)
      cout << vt[i].value;
    cout << endl;
  }

	auto GetPriority = [](const char * opt) -> int {
		if (strcmp(opt, "") == 0 )
			return 0;
		else if (strcmp(opt, "+") == 0 || strcmp(opt, "-") == 0)
			return 1;
		else if (strcmp(opt, "*") == 0 || strcmp(opt, "/") == 0 || strcmp(opt, "%") == 0)
			return 2;
		else
			return -1;
	};

  Token  t = {null, ""};
  Node * pnode;
  for (vector<Token>::iterator it=vt.begin(); it != vt.end(); it++) {
    t = *it;
    Node * node;
    switch (t.type) {
      case opt:
        { 
          node = new Node({t, NULL, NULL, NULL});
          node->lchild = pnode;
          int nprt = 0;
          vector<Token>::iterator it1 = it+1;
          while ( it1 != vt.end()
                  && (nprt !=0 
                      || it1->type != opt 
                      || GetPriority(it1->value) > GetPriority(t.value) ) )
          {
            if (it1->type == open_prt)
              nprt++;
            else if (it1->type == close_prt)
              nprt--;

            it1++;
          }
          vector<Token> sub_vt(it+1, it1);
          node->rchild = SortToken(sub_vt);
          it = it1 - 1;
          pnode = node;
        }
        break;
      case open_prt:
        it--;
      // case function0: 
      case function1:
      case function2:
        {
          it++;   // open_prt, rm it
          int nprt = 1;
          vector<Token>::iterator it1 = it+1;
          while (nprt > 0) {
            if (it1->type == open_prt)
              nprt++;
            else if (it1->type == close_prt)
              nprt--;

            it1++;
          }
          vector<Token> sub_vt(it+1, it1-1);
          if (t.type == open_prt) {
            pnode = SortToken(sub_vt);
          } else {
            node = new Node({t, NULL, NULL, NULL});
            node->lchild = SortToken(sub_vt);
            pnode = node;
          }
          it = it1-1; // close_prt, rm it
        }
        break;
      case number:
      case variable:
        node = new Node({t, NULL, NULL, NULL});
        if (pnode && (pnode->token).type == opt) {
          pnode->rchild = node;
        } else {
          pnode = node;
        }
        break;
      case separator:
        {
          vector<Token> sub_vt(it+1, vt.end());
          pnode->sibling = SortToken(sub_vt);
          return pnode;
        }
      default:
        cerr << ERROR << "fail to create syntax tree at: " << TypeName[t.type] << "\t" << t.value
             << " from expression: " << ENDL;
        for (int i=0; i<vt.size(); i++)
          printf("%d\t%-10s\t%s\n", i+1, TypeName[vt[i].type], vt[i].value);

        return NULL;
    }
  }
  return pnode;
}

set<string> GetVariables(Node * node) {
	set<string> vars;
	if (node) {
		if (node->token.type == variable)
			vars.insert(node->token.value);

		for(string var : GetVariables(node->lchild))
			vars.insert(var);
		for(string var : GetVariables(node->rchild))
			vars.insert(var);
		for(string var : GetVariables(node->sibling))
			vars.insert(var);
	}
	return vars;
}

const char * GetTypeName(ExpType t)
{
	return TypeName[t];
}

double Getf1Value(const char *f, double arg)
{
	if (f1[f])
		return f1[f](arg);
	else
		return 0./0.;
}

double Getf2Value(const char *f, double arg1, double arg2)
{
	if (f2[f])
		return f2[f](arg1, arg2);
	else
		return 0./0.;
}

void PrintNode(Node * node) {
  switch ((node->token).type) {
    case opt:
      cout << "(";
      PrintNode(node->lchild);
      cout << (node->token).value;
      PrintNode(node->rchild);
      cout << ")";
      break;
    case function1:
    case function2:
      cout << (node->token).value;
      cout << "(";
      node = node->lchild;
      if (node) {
        while (node->sibling) {
          PrintNode(node);
          cout << ", ";
          node = node->sibling;
        }
        PrintNode(node);
      }
      cout << ")";
      break;
    case number:
    case variable:
      cout << (node->token).value;
      break;
    default:
      cerr << ERROR << "Invalid node in token tree: " << TypeName[(node->token).type] << "\t" << (node->token).value
           << " from expression: " << ENDL;
      return;
  }
}

void DeleteNode(Node * node) {
	if (node) {
		DeleteNode(node->lchild);
		DeleteNode(node->rchild);
		DeleteNode(node->sibling);
		delete node;
		node = NULL;
	}
}
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
