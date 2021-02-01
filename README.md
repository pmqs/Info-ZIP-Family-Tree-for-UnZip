# unzip + zstd support
Fork of http://www.info-zip.org/

Added support for zstd compression method.

The zip 6.3 format has official support for zstd.

https://en.wikipedia.org/wiki/ZIP_(file_format)

www.zstd.net

## linux build

option1: using cmake
```
cmake .
make
./unzip -v
```

option2: using make
```
make -f unix/Makefile generic_zstd
./unzip -v
```
