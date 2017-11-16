#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <getopt.h>

#include <irc/message.h>

static char *nick = NULL;
static char *pass = NULL;

static void usage(void)
{
    puts("nickserv -n nick -p password");
}

static void print_message(irc_message_t m)
{
    char *line = NULL;
    size_t linelen = 0;
    irc_error_t r;

    if (m == NULL) {
        fprintf(stderr, "nickserv: memory exhaustion\n");
        return;
    }

    r = irc_message_string(m, &line, &linelen);

    if (IRC_SUCCESS(r)) {
        printf("%s", line);
        fflush(stdout);
    }

    free(line);
}

static void send_regain(void)
{
    irc_message_t r = NULL;
    char *altnick = NULL;

    /* first change nick, and then regain
     */
    asprintf(&altnick, "%s_alt", nick);
    if (altnick == NULL) {
        fprintf(stderr, "nickserv: memory exhaustion\n");
        return;
    }

    r = irc_message_make(nick, IRC_COMMAND_NICK, altnick, NULL);
    print_message(r);

    irc_message_free(r);
    r = NULL;

    /* regain command will either disconnect us, or change our nick
     * to the correct one.
     */
    r = irc_message_privmsg(nick, "nickserv", "REGAIN %s %s", nick, pass);
    print_message(r);

    irc_message_free(r);
    r = NULL;
}

static void send_register(void)
{
    irc_message_t id = NULL;
    /* send register account
     */
    id = irc_message_privmsg(nick, "nickserv", "IDENTIFY %s %s", nick, pass);
    print_message(id);

    irc_message_free(id);
    id = NULL;
}

int main(int ac, char **av)
{
    char *line = NULL;
    size_t linelen = 0;
    irc_message_t m = NULL;
    irc_error_t r;
    int c = 0;
    bool regain = false;

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
        fprintf(stderr, "nickserv: no nick specified\n");
        return 3;
    }

    if (pass == NULL) {
        pass = getenv("NICKSERV_PASSWORD");
        if (pass == NULL) {
            fprintf(stderr, "nickserv: no password specified\n");
            return 3;
        }
    }

    while (getline(&line, &linelen, stdin) != -1) {
        m = irc_message_new();
        if (m == NULL) {
            continue;
        }

        r = irc_message_parse(m, line, linelen);

        if (IRC_FAILED(r)) {
            irc_message_free(m);
            m = NULL;
            continue;
        }

        if (!irc_message_prefix_nick(m, nick)) {
            /* we have no use for messages that are not directed at us
             */
            irc_message_free(m);
            m = NULL;
            continue;
        }

        if (irc_message_is(m, IRC_COMMAND_MODE) && !regain) {
            send_register();
        } else if (irc_message_is(m, IRC_ERR_NICKNAMEINUSE)) {
            /* nick is in use, so identify and then try to regain
             */
            send_regain();
            regain = true;
        }
    }

    fprintf(stderr, "nickserv: quitting\n");

    return 0;
}
