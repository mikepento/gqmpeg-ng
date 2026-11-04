# gqmpeg-ng
The classic GQmpeg audio player front-end with modern day fixes.

## Building
Clone the repository to your local machine using `git clone`, then do the following:

```
./configure --prefix=/usr LDFLAGS="-Wl,--copy-dt-needed-entries"
make
```
