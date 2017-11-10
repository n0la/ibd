%{
#include "config.y.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunneeded-internal-declaration"

%}

%%

network return TOK_NETWORK;

\{ return TOK_PAR_OPEN;
\} return TOK_PAR_CLOSE;
\= return TOK_EQUAL;
; return TOK_SEMI_COLON;

[a-z]+ {
    yylval.string = strdup(yytext);
    return TOK_STRING;
}

\"(\\.|[^"\\])*\" {
    yylval.string = strdup(yytext);
    return TOK_QUOTED_STRING;
}

[ \t\n] ;

%
