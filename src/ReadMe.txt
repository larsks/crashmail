How to compile:

Just type

 make linux
 
or

 make win32
 
depending on the platform which you want to compile CrashMail for. You will 
now find the binaries in the 'bin' directory.

If you want to remove all object files that were created during the compilation,
type

 make cleanlinux
 
or

 make cleanwin32

Note: CrashMail was developed using gcc. If you are using another compiler, you
will probably have to make some adjustments to the makefiles and perhaps also 
to the source.
