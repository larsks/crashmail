CrashMail II
============

This was originally Johan Billing's *CrashMail II* distribution.  I
have introduced some minor bug fixes, and removed much of the
non-Linux support.

If you run into any problems with this code, plase report them to
the [project bugtracker][bugs].

Compiling
=========

To compile CrashMail II, run `make`:

    make

To run the test suite:

    make tests

You will need [Roundup][] to run the tests.

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

Or like this:

    Boss,99:99/1
    Point,1,Test_Point_1,Test_Locale,Test_Sysop,0-000-000-0000,300,INA:localhost,IBN

The `Boss` directive is common in other FTN software but is only
available in versions of Crashmail > 0.86.

Author
======

CrashMail II was originally written by Johan Billing,
<billing@df.lth.se>.

This port is maintained by Lars Kellogg-Stedman <lars@oddbit.com>
(netmail to 1:322/761).

