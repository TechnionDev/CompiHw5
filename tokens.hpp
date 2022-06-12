#ifndef TOKENS_HPP_
#define TOKENS_HPP_
#include <stdlib.h>
  extern int yylineno;
  extern char* yytext;
// Check if MacOS
#ifdef __APPLE__
  extern size_t yyleng;
#else
  extern int yyleng;
#endif
  extern int yylex();
#endif /* TOKENS_HPP_ */
