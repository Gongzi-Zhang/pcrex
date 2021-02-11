#ifndef LINE_H
#define LINE_H

#include <vector>

#define MAX 256	  // max line chars

/****************** LINE PROCESSING FUNCTIONS ******************
 * all the following text read functions that have a start position
 * argument support negative value, meaning counts from the end of a 
 * string
 *
 *  -- 2020.03.06
 */
bool StripComment(char *line);
char* StripSpaces(char *line);
bool IsEmpty(const char *line);
bool IsInteger(const char *line);
bool IsNumber(const char *line);
// int  Size(const char *line);
int  Index(const char *line, const char c, const int start=0);
int	 Index(const char *line, const char *sub, const int start=0);
int  Count(const char *line, const char c);
bool Contain(const char *line, const char *sub, const int start=0);
std::vector<char*> Split(const char *line);	// split on space (tab), all space will be regarded as one del.
std::vector<char*> Split(const char *line, const char del);
std::vector<char*> Split(const char *line, const char *del);
char* Sub(const char *line, const int start=0);
char* Sub(const char *line, const int start, const int length);
void  Reg(char *);	// assistant func, remember allocated memory address
void  Free();	// assistant func, help to free memory allocated in Sub functions
void StringTests();

#endif
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
