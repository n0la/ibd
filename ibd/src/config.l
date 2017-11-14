%{
#include "config.y.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunneeded-internal-declaration"

%}

%%

network return TOK_NETWORK;
plugin return TOK_PLUGIN;
filename return TOK_FILENAME;
option return TOK_OPTION;
args return TOK_ARGS;
env return TOK_ENV;

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
