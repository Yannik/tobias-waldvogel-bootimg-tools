#!/bin/sh

cflags="-I ."
#del 2&gt;nul $(OutputPath)$(TargetName)_x86.a 
rm libsepol_x86.a
unset obj
for f in *.c; do
  echo "X86 CC $f"
  gcc -c "$f" -o "${f%.*}"_x86.o -ffunction-sections -fdata-sections -Wall -Wno-unused-function -Wl,--gc-sections $cflags
  obj="${f%.*}_x86.o $obj"
done
ar rcs libsepol_x86.a $obj
#$(X86_TOOLCHAIN)gcc -c %%f -o $(IntermediateOutputPath)/%%~nf_x86.o -ffunction-sections -fdata-sections -Wall -Wno-unused-function -Wl,--gc-sections  --sysroot=$(X86_SYSROOT) !cflags! &amp;&amp; set obj=$(IntermediateOutputPath)/%%~nf_x86.o !obj!) &amp;&amp; echo X86 AR $(OutputPath)$(TargetName)_x86.a &amp;&amp; $(X86_TOOLCHAIN)ar rcs $(OutputPath)$(TargetName)_x86.a !obj!"</Command>

