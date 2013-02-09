CrashMail II
============

CrashMail II is a Fidonet tosser/scanner with a built-in AreaFix
implementation, support for Binkley style outbound (BSO), and message
filtering capabilities.

This is a fork of Johan Billing's *CrashMail II* distribution.  Major
changes in this fork include:

- Support for 64-bit operating (POSIX) operating systems.
- A revamped build system with integrated testing
- No more support for non-POSIX operating systems.

If you run into any problems with this code, plase report them to
the [project bugtracker][bugs].

Compiling
=========

To compile CrashMail II, run `make`:

    make

To run the test suite:

    make tests

To install everything:

    make install

If you are building packages, the `Makefile` supports the standard
`DESTDIR=` option.

[bugs]: https://github.com/larsks/crashmail/issues
[roundup]: http://bmizerany.github.com/roundup/

Pointlists
==========

The man page for `crashlist` says:

> Pointlists should be in BinkleyTerm format and should be specified after the
> real nodelists.

I wasn't sure what exactly a BinkleyTerm format pointlist should look
like.  Looking through the code, the following seems to work.  Given a
nodelist that looks like this:

    Zone,99,Test_Zone,Test_Locale,Test_Sysop,0-000-000-0000,300,INA:localhost,IBN
    Host,99,Test_Net,Test_Locale,Test_Sysop,0-000-000-0000,300,INA:localhost,IBN
    ,1,Test_Host_1,Test_Locale,Test_Sysop,0-000-000-0000,300,INA:localhost,IBN
    ,99,Test_Host_2,Test_Locale,Test_Sysop,0-000-000-0000,300,INA:localhost,IBN

Then your pointlist could look like this:

    Zone,99,Test_Zone,Test_Locale,Test_Sysop,0-000-000-0000,300,INA:localhost,IBN
    Host,99,Test_Net,Test_Locale,Test_Sysop,0-000-000-0000,300,INA:localhost,IBN
    ,1,Test_Host_1,Test_Locale,Test_Sysop,0-000-000-0000,300,INA:localhost,IBN
    Point,1,Test_Point_1,Test_Locale,Test_Sysop,0-000-000-0000,300,INA:localhost,IBN

Or like this (if you are running CrashMail > 0.86):

    Boss,99:99/1
    Point,1,Test_Point_1,Test_Locale,Test_Sysop,0-000-000-0000,300,INA:localhost,IBN

Author
======

CrashMail II was originally written by Johan Billing,
<billing@df.lth.se>.

This port is maintained by Lars Kellogg-Stedman <lars@oddbit.com>
(netmail to 1:322/761).

