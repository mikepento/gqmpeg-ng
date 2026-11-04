# gqmpeg-ng
The classic GQmpeg audio player front-end with modern day fixes.

GQmpeg was originally developed by John Ellis in 1999 and all of the credit goes to him. Because I like "retro" linux desktop things, I decided to create a "next-generation" repository that provides fixes that will enable John's great work to be built on modern distro's with modern compilers.

All work here is based on the 0.91.1 GQmpeg development sources. Complete information for GQmpeg can be found on its Sourceforge homepage here: [https://gqmpeg.sourceforge.net/](https://gqmpeg.sourceforge.net/)

Also, note that any source changes and/or notes I provide here for GQmpeg have only been tested on [Arch Linux](https://archlinux.org/).

## Building
Clone the repository to your local machine using `git clone`, then run configure and make.

__NOTE:__

Setting `LDFLAGS` with `-Wl` and `--copy-dt-needed-entries` is required to address linker failure:

```
/usr/bin/ld: dock.o: undefined reference to symbol 'XResizeWindow'
/usr/bin/ld: /usr/lib/libX11.so.6: error adding symbols: DSO missing from command line
```

Other options, such as `--prefix=/usr` are optional.

```
./configure --prefix=/usr LDFLAGS="-Wl,--copy-dt-needed-entries"
make
```


