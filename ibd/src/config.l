%{
#include "config.y.h"

#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunneeded-internal-declaration"
#pragma GCC diagnostic ignored "-Wimplicit-function-declaration"

%}

%%

network return TOK_NETWORK;
plugin return TOK_PLUGIN;
filename return TOK_FILENAME;
option return TOK_OPTION;
args return TOK_ARGS;
env return TOK_ENV;
channel return TOK_CHANNEL;
user return TOK_USER;
group return TOK_GROUP;

\{ return TOK_PAR_OPEN;
\} return TOK_PAR_CLOSE;
\= return TOK_EQUAL;
; return TOK_SEMI_COLON;
# {
    int c;

    while ((c = input()) != '\n' && c != EOF)
        ;

    if (c == EOF || c == '\n') {
        unput(c);
    }

    ++yylineno;
}

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
