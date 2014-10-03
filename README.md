sasllo
======

A very SASL hello world.

Building
--------

Just running `make` should take care of all building needs, provided all
dependencies needed are installed.  It will create two executables: **client**
and **server**.

Usage
-----

SASL is fundamentally geared toward networks, so this isn't a "hello world" in
the traditional sense.  Think of it more as an echo server.

First, configure the desired SASL mechanisms in **/etc/sasl2/sasllo.conf**.
One possible configuration is:

    mech_list: ANONYMOUS PLAIN

To run the system, first pick a port (like 9001).  Then launch the server:

    $ ./server 9001

and in another shell, launch the client:

    $ ./client 127.0.0.1 9001

If you are not running the client and server on the same machine,
replace 127.0.0.1 with the address of the machine running the server.

**client** reads messages from stdin, delimited by the end-of-file character
(C-d in most shells).  If your message does not end in a newline, your shell
may require you to press C-d twice.

**server** logs all messages it receives to stdout, and sends them back to
**client**, which also logs all messages it receives to stdout.

Bugs
----

Yes.  This application should not be considered secure.
