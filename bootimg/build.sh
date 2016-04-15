#!/bin/sh

cflags="-I ../libsepol"
libs="../libsepol/libsepol_x86.a"
echo X86 compile 
unset list
for f in *.c; do
  list="$f $list"
done
#requires glibc-static
gcc -o buildimg_x86 $list $cflags -static $libs -ffunction-sections -fdata-sections -Wall -Wno-unused-function -Wl,--gc-sections

#$(X86_TOOLCHAIN)gcc -o $(OutputPath)$(TargetName)_x86 !list! !cflags! -static !libs! -ffunction-sections -fdata-sections -Wall -Wno-unused-function -Wl,--gc-sections  --sysroot=$(X86_SYSROOT) &amp;&amp; $(X86_TOOLCHAIN)strip $(OutputPath)$(TargetName)_x86"</Command>

