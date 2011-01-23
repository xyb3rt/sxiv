sxiv: Simple (or small or suckless) X Image Viewer

sxiv is a really simple alternative to feh and qiv. Its only dependency is
imlib2. The primary goal for writing sxiv is to create an image viewer, which
only implements the most basic features required for fast image viewing. It
works nicely with tiling window managers and its code base should be kept small
and clean to make it easy for you to dig into it and customize it for your
needs.

Installation
------------
sxiv is built using the commands:

    $ make
    # make install

Please note, that the latter one requires root privileges.
By default, sxiv is installed using the prefix "/usr/local", so the full path
of the executable will be "/usr/local/sbin/sxiv".

You can install it into a directory of your choice by changing the second
command to:

    # PREFIX="/your/dir" make install

All build-time specific settings can be found in the file "config.h". Please
check and change them, so that they fit your needs.

Usage
-----
sxiv has no useful command line options yet, but they will be added in the next
releases. Right now, it simply displays all files given on the command line.

Use the following keys to control sxiv:

    q            Quit sxiv
    Escape       Quit sxiv and return an exit value of 2 (useful for scripting)
    Space,n      Go to the next image
    Backspace,p  Go to the previous image
    g/G          Go to first/last image
    [/]          Go 10 images backward/forward
    +,=          Zoom in
    -            Zoom out
    h,j,k,l      Scroll left/down/up/right
    f            Toggle fullscreen mode (requires an EWMH/NetWM compliant
                 window manager)
