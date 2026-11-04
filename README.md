# gqmpeg-ng
The classic GQmpeg audio player front-end with modern day fixes.

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


