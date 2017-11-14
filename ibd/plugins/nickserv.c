#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include <irc/message.h>

static void usage(void)
{
    puts("nickserv -n nick -p password");
}

static void print_message(irc_message_t m)
{
    char *line = NULL;
    size_t linelen = 0;
    irc_error_t r;

    r = irc_message_string(m, &line, &linelen);
    if (IRC_SUCCESS(r) && linelen > 2) {
        line[linelen-2] = '\0';
        printf("%s\n", line);
    }

    free(line);
}

int main(int ac, char **av)
{
    char *line = NULL;
    size_t linelen = 0;
    char *nick = NULL;
    char *pass = NULL;
    irc_message_t m = NULL, id = NULL;
    irc_error_t r;
    int c = 0;

    static char const *optstr = "n:p:";
    static struct option opts[] = {
        { "nick", required_argument, NULL, 'n' },
        { "password", required_argument, NULL, 'p' },
        { NULL, 0, NULL, 0 }
    };

    while ((c = getopt_long(ac, av, optstr, opts, NULL)) != -1) {
        switch (c) {
        case 'n': free(nick); nick = strdup(optarg); break;
        case 'p': free(pass); pass = strdup(optarg); break;
        case '?':
        default: usage(); return 3;
        }
    }

    if (nick == NULL) {
        fprintf(stderr, "no nick specified\n");
        return 3;
    }

    if (pass == NULL) {
        pass = getenv("NICKSERV_PASSWORD");
        if (pass == NULL) {
            fprintf(stderr, "no password specified\n");
            return 3;
        }
    }

    while (getline(&line, &linelen, stdin) != -1) {
        r = irc_message_parse(m, line, linelen);

        if (IRC_FAILED(r)) {
            continue;
        }

        if (m->command != NULL && strcmp(m->command, "MODE") == 0) {
            /* send register account
             */
            id = irc_message_make(NULL, "PRIVMSG", "nickserv", "identify",
                                  nick, pass, NULL);
            print_message(id);

            irc_message_free(id);
            id = NULL;
        }
    }

    return 0;
}
