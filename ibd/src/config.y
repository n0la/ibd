%{
#include <stdio.h>
#include <string.h>

#include <ibd/config.h>
#include <ibd/network.h>

extern int yylex(void);

static network_t *n = NULL;

void yyerror(char const *str)
{
    fprintf(stderr, "error reading configuration file: %s\n", str);
}

int yywrap(void)
{
    return 1;
}

%}

%union {
    char *string;
    int integer;
}

%token <integer>       TOK_NETWORK TOK_PAR_OPEN TOK_PAR_CLOSE TOK_EQUAL TOK_SEMI_COLON
%token <string>        TOK_STRING
%token <string>        TOK_QUOTED_STRING

%%

grammar: /* empty */
                {
                }
        |       grammar network
                {
                }
                ;

network:        TOK_NETWORK TOK_PAR_OPEN network_vars TOK_PAR_CLOSE TOK_SEMI_COLON
                {
                    config_add_network(n);
                    n = NULL;
                }
                ;

network_vars:
        |       network_vars network_var
        ;

network_var:    TOK_STRING TOK_EQUAL TOK_QUOTED_STRING TOK_SEMI_COLON
                {
                    char *val = $3;
                    int sz = 0;
                    if (n == NULL) {
                        n = network_new();
                        if (n == NULL) {
                            yyerror("memory allocation");
                        }
                    }

                    if (*val == '"') {
                        ++val;
                    }
                    sz = strlen(val);
                    if (sz > 0 && val[sz-1] == '"') {
                        val[sz-1] = '\0';
                    }

                    if (network_set(n, $1, val)) {
                        yyerror("invalid attribute for network");
                    }

                    free($1);
                    free($3);
                }
        ;

%%
