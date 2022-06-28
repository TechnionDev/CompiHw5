%{
    #include <stdlib.h>
    #include <stdio.h>
    #include "stypes.hpp"
    #include "parser.tab.hpp"
    #include "hw3_output.hpp"

    using namespace output;

    char current_str[1025];
    int current_str_length = 0;

    void error_unclosed_string();
    void error_undefined_escape(char escape_char);
    void error_undefined_hex_escape(char non_hex1, char non_hex2);
    void error_unprintable_char(char bad_char);
%}

%option yylineno
%option noyywrap

digit           ([0-9])
nozerodigit     ([1-9])
letter          ([a-zA-Z])
whitespace      ([\t\n\r ])
escapechars     ([\\"nrt0])

%%
{whitespace}                        ;
(void)                              return VOID;
(int)                               return INT;
(byte)                              return BYTE;
(b)                                 return B;
(bool)                              return BOOL;
(auto)                              return AUTO;
(and)                               return AND;
(or)                                return OR;
(not)                               return NOT;
(true)                              return TRUE;
(false)                             return FALSE;
(return)                            return RETURN;
(if)                                return IF;
(else)                              return ELSE;
(while)                             return WHILE;
(break)                             return BREAK;
(continue)                          return CONTINUE;
(\;)                                return SC;
(\,)                                return COMMA;
(\()                                return LPAREN;
(\))                                return RPAREN;
(\{)                                return LBRACE;
(\})                                return RBRACE;
(=)                                 return ASSIGN;
(\>=)                               return GEOP;
(\>)                                return GTOP;
(\<=)                               return LEOP;
(\<)                                return LTOP;
((==))                              return EQOP;
((!=))                              return NEOP;
(\+)                                return ADDOP;
(\-)                                return SUBOP;
(\*)                                return MULOP;
(\/)                                return DIVOP;
(\/\/[^\r\n]*[ \r|\n|\r\n]?)        ; // Handle comment
({letter}({letter}|{digit})*)       {yylval = NEWSTD_V(std::string, (yytext)); return ID;}
(0{digit}+)                         error_unprintable_char(*yytext);
(0|{nozerodigit}{digit}*)           {yylval = NEWSTD_V(std::string, (yytext)); return NUM;}
(\"([^\n\r\"\\]|\\[rnt"\\])+\")     {return STRING;}
.                                   {errorLex(yylineno);}
%%

void error_unclosed_string() {
    printf("Error unclosed string\n");
    exit(0);
}

void error_undefined_escape(char escape_char) {
    printf("Error undefined escape sequence %c\n", escape_char);
    exit(0);
}

void error_undefined_hex_escape(char non_hex1, char non_hex2) {
    if (non_hex1 != '\0' && non_hex2 != '\0') {
        printf("Error undefined escape sequence x%c%c\n", non_hex1, non_hex2);
    } else if (non_hex1 != '\0') {
        printf("Error undefined escape sequence x%c\n", non_hex1);
    } else {
        printf("Error undefined escape sequence x\n");
    }
    exit(0);
}

void error_unprintable_char(char bad_char) {
    printf("Error %c\n", bad_char);
    exit(0);
}
