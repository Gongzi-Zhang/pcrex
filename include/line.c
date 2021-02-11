#include <iostream>
#include <stdlib.h>
#include <assert.h>
#include <cstring>
#include <vector>
#include <locale>

#include "line.h"
#include "io.h"

using namespace std;

vector<char *> subs;

void Reg(char * s) {subs.push_back(s);}
void Free()
{
	while (subs.size())
	{
		char *s = subs.back();
		if (s)
			delete[] s;
		subs.pop_back();
	}
}

bool StripComment(char* line) {
  if (line == NULL) {
    cerr << ERROR << "uninitialized line";
    return false;
  }

  int i = 0;
  bool single_quote = false, double_quote = false;
  while (line[i] != '\0') {
    if (line[i] == '#' && !single_quote && !double_quote) {
      line[i] = '\0';
      return true;
    } else if (line[i] == '"') {
      if (!single_quote)
        double_quote = !double_quote;
    } else if (line[i] == '\'') {
      if (!double_quote)
        single_quote = !single_quote;
    }
    i++;
    continue;
  }
  if (single_quote || double_quote) {
    cerr << WARNING << "unmatched quote!" << ENDL;
    return false;
  }
  return true;
}

char * StripSpaces(char *line) { // remove space at the beginning and end of a line
  if (line == NULL) {
    cerr << WARNING << "uninitialized line";
    return NULL;
  }

  int ib=0, ie=0;
  while (line[ib]==' ' || line[ib]=='\t' || line[ib] == '\n')
    ib++;

  ie=ib;
  while (line[ie] != '\0')
    ie++;

  for (ie--; ie>ib && (line[ie] == ' ' || line[ie] == '\t' || line[ie] == '\n'); ie--) ;

  ie++;
  if (ie > ib)
    line[ie] = '\0';
  
  if (ib>0)
    strcpy(line, line+ib);

	return line;
}

bool IsEmpty(const char *line) {
  if (line == NULL) {
    cerr << ERROR << "uninitialized line";
    return false;
  }

  if (line[0])
    return false;
  else
    return true;
}

bool IsInteger(const char *line) {
  if (line == NULL) {
    cerr << ERROR << "uninitialized line";
    return false;
  }

  int i=0;
  while (line[i] == ' ' || line[i] == '\t') // beginning space
    i++;

  if (line[i] == '\0')	// empty line
    return false;   

  if (line[i] == '+' || line[i] == '-')	  // sign
    i++;

  while (isdigit(line[i]))   // digits
    i++;

  while (line[i] == ' ' || line[i] == '\t')   // ending space
    i++;

  if (line[i] != '\0')
    return false;

  return true;
}

bool IsNumber(const char *line) {
  if (line == NULL) {
    cerr << ERROR << "uninitialized line";
    return false;
  }

  int i=0;

  while (line[i] == ' ' || line[i] == '\t')
    i++;

  if (line[i] == '\0')	// empty line
    return false;   

  // coefficient part
  if (line[i] == '+' || line[i] == '-')	// may begin with a sign char
    i++;
  
  int j=i;
  while (isdigit(line[i]))  // integer part
    i++;

  if (line[i] == '.') {	    // decimal dot
    if (i==j && !isdigit(line[i+1]))  // decimal dot as first char must be followed by a digit
      return false;

    i++;
  }
  
  while (isdigit(line[i]))  // fractional part
    i++;
  
  if (line[i] == 'e' || line[i] == 'E') {
    i++;
		if (line[i] == '\0')	// end with e/E
			return false;
	} else {	// non-digit char
    while (line[i] == ' ' || line[i] == '\t') // can only be space/tab
      i++;

    if (line[i] != '\0')
      return false;

    return true;
  }

  // exponent part
  if (line[i] == '+' || line[i] == '-')
    i++;

  while (isdigit(line[i]))  // consider only integer exponents, no dot in exponent part
    i++;

  while (line[i] == ' ' || line[i] == '\t')
    i++;

  if (line[i] != '\0')	
    return false;

  return true;
}

int Index(const char *line, const char c, const int start) { // first index of char in line since start position
  if (line == NULL) {
    cerr << WARNING << "uninitialized line";
    return -2;
  }
  
  const int n = strlen(line);
  if (abs(start) >= n) {
    cerr << ERROR << "invalid start position: " << start << " in line: " << line << ENDL;
    return -3;
  }

  int i = start>=0 ? start : n+start;
  while (i<n && line[i] != c)
    i++;

  if (i==n)
    return -1;	// no c in line
  else
    return i;
}

int Index(const char *line, const char *sub, const int start) {  // index of a sub string 
  if (line == NULL) {
    cerr << WARNING << "uninitialized line" << ENDL;
    return -2;
  }

  if (sub == NULL) {
    cerr << WARNING << "uninitialized sub" << ENDL;
    return -3;
	}
  
  const int n = strlen(line);
  const int m = strlen(sub);

  if (abs(start) >= n) {
    cerr << ERROR << "invalid start position: " << start << " in line: " << line << ENDL;
    return -4;
  }

  int i = start>=0 ? start : n+start;
  while (i<n) {
    if (line[i] != sub[0]) {
      i++;
      continue;
    }

    int j=1;
    while (j<m && i+j<n) {
      if (line[i+j] == sub[j])
        j++;
      else
        break;
    }
    if (j==m)
      return i;
    else 
      i += j;
  }

  return -1;  // not fount
}

int Count(const char *line, const char c) {  // count the occurrance of char c in line
  if (line == NULL) {
    cerr << WARNING << "uninitialized line";
    return -1;
  }
  
  int count=0;
  for (int i=0; line[i] != '\0'; i++)
    if (line[i] == c)
      count++;

  return count;
}

bool Contain(const char *line, const char *sub, const int start) {
  if (sub == NULL)
    return true;

  if (line == NULL) {
    cerr << WARNING << "uninitialized line";
    return false;
  }
  
  const int n = strlen(line);
  const int m = strlen(sub);
  if (abs(start) >= n) {
    cerr << ERROR << "invalid start position: " << start << " in line: " << line << ENDL;
    return false;
  }

  int i = start>=0 ? start : n+start;
  while (i<n) {
    if (line[i] != sub[0]) {
      i++;
      continue;
    }

    int j=1;
    while (sub[j] != '\0' && (i+j)<n) {
      if (line[i+j] != sub[j]) 
	break;

      j++;
    }
    
    if (sub[j] == '\0')
      return true;

    if (i+j == n)
      return false;

    i++;
  }

  return false;
}

char * Sub(const char *line, const int start) {
  if (line == NULL) {
    cerr << WARNING << "uninitialized line";
    return NULL;
  }

  const int n = strlen(line);
  if (abs(start) >= n) {
    cerr << ERROR << "invalid position: " << start << " in line: " << line << ENDL;
    return NULL;
  }

  // char *sub = (char*) malloc(sizeof(char) * (start>0 ? n-start+1 : -start+1));
  char *sub = new char[start>=0 ? n-start+1 : -start+1];
	Reg(sub);	// register new allocated memory
  if (start >= 0)
    strcpy(sub, line+start);
  else
    strcpy(sub, line+n+start);

  return sub;
}

char * Sub(const char *line, const int start, const int length) {
  if (line == NULL) {
    cerr << WARNING << "uninitialized line";
    return NULL;
  }

  if (length < 0) { // length can be 0
    cerr << ERROR << "negative length: " << length << ENDL;
    return NULL;
  }

  const int n = strlen(line);
  if (abs(start) >= n) {
    cerr << ERROR << "invalid start position: " << start << " in line: " << line << ENDL;
    return NULL;
  }

  int size = start>=0 ? n-start : -start;
  if (size > length)
    size = length;

  // char *sub = (char*) malloc(sizeof(char) * (size+1));
  char *sub = new char[size+1];
	Reg(sub);
  strncpy(sub, (start>=0 ? line+start : line+n+start), size);
  sub[size] = '\0';
  return sub;
}

vector<char*> Split(const char *line) {
  vector<char*> fields;

  if (line == NULL) {
    cerr << WARNING << "uninitialized line";
    return fields; // FIXME: what should be return?
  }

  int index1=0, index2=0;
  while (line[index2] != '\0') {
    if (line[index2] != ' ' && line[index2] != '\t') {
      index2++;
      continue;
    }

    if (index2 > index1) {
      fields.push_back(Sub(line, index1, index2-index1));
      index1 = index2;
    }

    index1++;
    index2++;
  }

  if (index2 > index1)
    fields.push_back(Sub(line, index1));	// push back the last field

  return fields;
}

vector<char*> Split(const char *line, const char del) {
  vector<char*> fields;

  if (line == NULL) {
    cerr << WARNING << "uninitialized line";
    return fields; // FIXME: what should be return?
  }
  int n=0;
  while (line[n] != '\0')
    n++;

  int index1=0, index2;
  while (index1 < n && (index2=Index(line, del, index1)) >= index1) 
	{
    // if (index2 == index1) {	// empty field
    //   index1 += 1;
    //   continue;
    // }
    fields.push_back(Sub(line, index1, index2-index1));
    index1 = index2+1;
  }
  if (line[index1] != '\0')
    fields.push_back(Sub(line, index1));	// push back the last field

  return fields;
}

vector<char*> Split(const char *line, const char *del) {
  vector<char*> fields;
  if (line == NULL) {
    cerr << WARNING << "uninitialized line";
    return fields; // FIXME: what should be return?
  }

  int n=strlen(line);
	int m=strlen(del);
	int pi = 0;
	int i = Index(line, del, pi);	// if del is null, return minus value
	while (i>=pi) {
		fields.push_back(Sub(line, pi, i-pi));
		pi = i+m;
		i = Index(line, del, pi);
	}
	if (line[pi] != '\0')
		fields.push_back(Sub(line, pi));	// last field

  return fields;
}

void StringTests() {
  const char * line = "hello, world";
  assert(IsEmpty(" \t ") == false);
  assert(IsEmpty("") == true);
  assert(IsInteger(" +00101 ") == true);
  assert(IsInteger(" - 00101 ") == false);
  assert(IsInteger("00101.") == false);
  assert(IsNumber("1.34e2") == true);
  assert(Index(line, ',') == 5);
  assert(Index(line, ',', 6) == -1);
  assert(Index(line, ',', -4) == -1);
  assert(Index(line, "world") == 7);
  assert(Count(line, 'o') == 2);
  assert(Contain(line, "llo") == true);
  assert(Contain(line, "llo", 5) == false);
  assert(strcmp(Sub(line, 7), "world") == 0);
  assert(strcmp(Sub(line, 2, 2), "ll") == 0);
  assert(strcmp(Sub(line, 2, 2), "llo") != 0);
  assert(strcmp(Sub(line, 2, 3), "ll") != 0);
  assert(strcmp(Sub(line, -3), "rld") == 0);
  assert(strcmp(Sub(line, -3, 1), "r") == 0);
  assert(strcmp(Sub(line, -3, 2), "r") != 0);
  assert(strcmp(Sub(line, -3, 2), "rld") != 0);
  assert(Split(line, ',').size() == 2);
  assert(strcmp(Split(line, ',')[0], "hello") == 0);
  assert(strcmp(Split("abc    xyz")[1], "xyz") == 0);
  cout << INFO << "pass all tests." << ENDL;
}
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
