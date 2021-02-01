# unzip + zstd support
Fork of http://www.info-zip.org/

Added support for zstd compression method.

The zip 6.3 format has official support for zstd.

https://en.wikipedia.org/wiki/ZIP_(file_format)

www.zstd.net

## linux build

```
cmake .
make
./unzip -v
```

## TODO
Build and test `funzip`. The zstd support is probably broken in `funzip`, but it works in `unzip`.
