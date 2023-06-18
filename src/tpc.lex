%{

#include "tree.h"
#include "tpc-s.tab.h"

int lineno = 1;
int colno = 1;
#define YY_USER_ACTION colno+=yyleng;

void new_line(){
   lineno++;  
   colno = 0;
}

%}

%option noinput
%option nounput
%option noyywrap


%x INLINE_COMMENT
%x MULTILINE_COMMENT

%%

[ \t]+ ;


\/\* BEGIN MULTILINE_COMMENT;
<MULTILINE_COMMENT>\*\/          BEGIN INITIAL;
<MULTILINE_COMMENT>.|\r|\t       ;
<MULTILINE_COMMENT>\n            new_line();            

\/\/ BEGIN INLINE_COMMENT;
<INLINE_COMMENT>\n               { new_line(); BEGIN INITIAL; }
<INLINE_COMMENT>.|\t             ;

\'"\\"[0-9]{1,3}\'               {
                                    yylval.byte = atoi(yytext+2);

                                    return CHARACTER;
                                 }
\'"\\"[\'\"\\]\'                   {
                                    yylval.byte = yytext[1];

                                    return CHARACTER;
                                 }

\'"\\"[abfnrtv]\'                {

                                    char byte = 0;
                                    switch(yytext[2]){
                                        case 'a': byte = '\a'; break;
                                        case 'b': byte = '\b'; break;
                                        case 'f': byte = '\f'; break;
                                        case 'n': byte = '\n'; break;
                                        case 'r': byte = '\r'; break;
                                        case 't': byte = '\t'; break;
                                    }
                                    yylval.byte = byte;
                                    return CHARACTER;
                                 }
\'.\'                            {
                                    yylval.byte = yytext[1];
                                    return CHARACTER;
                                 }

int|char                         {
                                    strcpy(yylval.type, yytext);
                                    return TYPE;
                                 }
void                             return VOID;

while                            return WHILE;
return                           return RETURN;
if                               return IF;
else                             return ELSE;

"=="|"!="                        {
                                    strcpy(yylval.comp, yytext);
                                    return EQ;
                                 }
"<"|"<="|">"|">="                {
                                    strcpy(yylval.comp, yytext);
                                    return ORDER;
                                 }
"||"                             return OR;
"&&"                             return AND;
"*"|"/"|"%"                      {
                                    yylval.byte = yytext[0];
                                    return DIVSTAR;
                                 }
[0-9]+                           {
                                    yylval.num = atoi(yytext);
                                    return NUM;
                                 }
"-"|"+"                          {
                                    yylval.byte = yytext[0];
                                    return ADDSUB;
                                 }
"="                              return yytext[0];
"!"                              return yytext[0];
";"|","                          return yytext[0];
"("|")"|"{"|"}"                  return yytext[0];
[_a-zA-Z][_a-zA-Z0-9]*           {
                                    strcpy(yylval.ident, yytext);
                                    return IDENT;
                                 }
\n                               new_line();
[ \r\t]+                         ;
.                                return yytext[0];

%%

