``ibd`` is a framework to build IRC bots. It is totally
shit, insecure, drowns kittens and will totally pretend
to help your babushka over the road, only to abandon
her in the middle of a traffic line, while also stealing
her handbag.

## Requirements

You will need the following:

* cmake
* gcc/clang
* flex
* bison
* libtls (is part of LibreSSL)
* OpenBSD and/or libbsd

# ibd

IBD is the IRC bot daemon that is responsible for connecting
to networks, and launching child processes. It reads a
configuration file from ``/etc/ibd.conf``. Run it as
root at your own peril. Just kidding, I would never allow
you to run this heap of dung
[as root](https://github.com/n0la/ibd/blob/master/ibd/src/ibd.c#L84).

## ibd.conf

This configuration file configures ``ibd`` to do its magic.
You need at least one ``network`` block to make the daemon
do anything. It is read per default from ``/etc/ibd.conf``.

### Network block

This describes one network the daemon should connect to:

```
# This is a comment!
network "freenode" {
  host = "chat.freenode.net";
  # SSL port on freenode is 6697
  port = "6697";
  ssl = "true";
};
```

It takes the following options:

 Option          |  Value
 ----------------|----------
 host            | IP, or DNS host
 port            | IP port (default 6667)
 ssl             | ``true`` to use SSL, ``false`` otherwise
 nick            | nickname on this network for the daemon

### Plugin Block

The plugin block specifies one plugin to run upon connection.
Plugins receive all the incoming IRC messages through ``stdin``,
and should print valid IRC messages out on ``stdout``. Anything
the plugin sends on ``stderr`` goes straight into the log file
as ``LOG_ERR`` facility on syslog. ``stdin`` uses blocking IO,
while ``stdout`` and ``stderr`` use non-blocking IO. Please
note that buffered IO (such as ``FILE*`` in C) may delay the
delivery of messages.

Multiple plugin blocks are allowed per ``network`` block.

Plugin blocks must have at least a ``filename`` statement to
specify the location (absolute) of the plugin to execute.
Optional additional args can specified as a list of strings
through ``args``, and special environment variables can be
set through multiple ``env`` statements:

```
plugin "nickserv" {
  # Executable
  filename "/usr/local/libexec/ibd/nickserv";
  # nickserv takes '-n alice' as arguments to determine
  # nickname it should authenticate for
  args "-n" "ibd";
  # Setting an environment variable is safer than passing
  # the password through arguments
  env "NICKSERV_PASSWORD" "SOMEPASS";
};
```

### Channel block

The channel block currently states a list of channels to auto join upon
connection. It will support additional options in the future, but for
now the lazy developer has not implemented them yet. Multiple ``channel``
blocks are allowed per ``network`` block:

```
network "freenode" {
  # ...
  channel "#d&d";
  channel "#d&d-inn";
}
```

## Plugins

### log

This is an example plugin to show how to use different languages (in this
case Perl) to write plugins for ``ibd``. It should not be used.

### nickserv

The nickserv plugin allows authentication of a given nick (through the
``-n`` command line parameter) with the given password (either through
the ``-p`` command line parameter, or the ``NICKSERV_PASSWORD``
environment variable) on *nickserv* style authentication bots (such
as found on freenode). It will authenticate the nickname, and will
also use the ``REGAIN`` command on nickserv to reclaim a registered
nick that is currently in use by someone else.

```
plugin "nickserv" {
  filename "/usr/local/libexec/ibd/nickserv";
  args "-n" "ibd";
  env "NICKSERV_PASSWORD" "SOMEPASS"
};
```

# libirc

The library ``libirc`` is what ``ibd`` uses to make sense of the
IRC protocol. Client bots written in C are encouraged to use this
library to parse and handle IRC messages received from ``stdin``.

Core of the library is the ``irc_t`` state machine, and the ``irc_message_t``
structure to parse and handle IRC messages. See the ``nickserv`` plugin
for a good example on how to use this library:

```C
int main(int ac, char **av)
{
    irc_message_t m;
    char *line = NULL;
    size_t linelen = 0;

    while (getline(&line, &linelen, stdin) != -1) {
        m = irc_message_parse2(line, linelen);

        if (irc_message_is(m, IRC_COMMAND_PRIVMSG)) {
             /* do something */
        }
    }

    free(line);
    return 0;
}
```
