
### Embedding Interpeters: Lua

Building a lua-5.3.5 Lua shared library:
- Download Lua sources: 
`curl -R -O http://www.lua.org/ftp/lua-5.3.5.tar.gz ; tar zxf lua-5.3.5.tar.gz`

- Make Lua with `-fPIC`, this generates object files
`make MYCFLAGS="$CFLAGS -fPIC -DLUA_COMPAT_5_2 -DLUA_COMPAT_5_1" MYLDFLAGS="$LDFLAGS" linux`

- Compile Lua objects into a shared library (skil luac.o due to redefinition of `main`):
`gcc -o liblua.so -shared $(ls -1 *.o  | egrep -v '(luac.o)' | tr '\n' ' ') -fPIC`

- Use the newly minted `liblua.so` to build a `C` shim:
`gcc -o invoke_lua invoke_lua.c -I/root/Code/zombieant/ext/luadist-static-lua/include/ -L`pwd` -llua -lm -ldl -lreadline`

- Invoke lua script via `C` shim preloading `liblua.so` as dependency:
`LD_LIBRARY_PATH=. LD_PRELOAD=./liblua.so ./invoke_lua hello.lua`


####  Building Lua libraries
- Manual:
wget https://luarocks.org/manifests/gunnar_z/lsocket-1.4.1-1.rockspec
wget $(grep url lsocket-1.4.1-1.rockspec  | cut -d= -f2 | sed 's/"//g')

- with luarocks:
`luarocks instal luasocket`

Find luarocks install location and copy to your deployment dir where lbraries will live:
`rsync -rvz /usr/local/lib/lua/5.3/* slibs/`


### SQLite3 Build

#### Shared:
```
gcc -c sqlite.c
gcc -shared -o libsqlite3.so -fPIC sqlite3.o -ldl -lpthr
```

#### Build code with SQLite3 library
`gcc -O0  driver.c -o driver -L`pwd` -lsqlite3  -I`pwd` -ldl -lpthread`

#### Static:

sqlite3:
`gcc -DSQLITE_THREADSAFE=0 -DSQLITE_OMIT_LOAD_EXTENSION -c  sqlite3.c`

code:
`gcc -Os -static  driver.c -o driver sqlite3.o   -I`pwd` `

or:

`ar rcs sqlite3.a sqlite3.o`
`gcc -Os -static  driver.c -o driver sqlite3.o   -I`pwd` `

### Micropython

`cc -std=c99 -I. -I../.. -DNO_QSTR -shared -fPIC    -c -o hello-dyn.o hello-embed.c`
`cc -Wall -o hello-dyn  -L. -lmicropy`

Note: Building with more featured cp ../../ports/unix/mpconfigport.h mpconfigport.h

### Libcurl build

with zlib:
```
./configure --prefix=/root/Code/zombieant/ext/zlib --static 
make 
make install
```

produces `/root/Code/zombieant/ext/zlib/lib/libz.a`
ldd libz.a 
	not a dynamic executable

with openssl:

https://wiki.openssl.org/index.php/Compilation_and_Installation

Openssl: with zlib:
1. ./config --prefix=/root/Code/zombieant/ext/openssl  --with-zlib-include=/root/Code/zombieant/ext/zlib/include --with-zlib-lib=/root/Code/zombieant/ext/zlib/lib no-shared

Openssl: without zlib:

CentOS depends:
`yum install gcc glibc-static libstdc++-static -y`

`./config no-shared no-threads no-dso no-engine no-err no-psk no-srp no-ec2m no-weak-ssl-ciphers no-idea no-comp no-ssl2 no-ssl3 -DOPENSSL_USE_IPV6=0 -static -static-libgcc --prefix=/root/Code/dist/openssl`

```
make 
make install_sw #(no man pages)
```

Libcurl: static: 
`CFLAGS="-static -static-libgcc" make &&  make install`


### LIBCURL 
`git clone https://github.com/curl/curl`

CentOS Depends:
`yum install libtool autoconf automake nroff m4 -y`

./buildconf 

Dynamic build 

`./configure  -disable-shared -enable-static --with-ssl=/root/Code/zombieant/ext/openssl  -with-zlib=/root/Code/zombieant/ext/zlib --disable-manual --without-librtmp --prefix=/root/Code/zombieant/ext/curl`
`make`
`make install`


Static build:

Configure:

`CPPFLAGS="-I/root/Code/zombieant/ext/openssl/include" LDFLAGS="-L/root/Code/zombieant/ext/openssl/lib -static" ./configure --with-ssl -without-zlib --disable-debug --enable-optimize --disable-curldebug  --disable-ares  --disable-shared --disable-thread --disable-rt --disable-largefile --disable-ldap --disable-ldaps --disable-rtsp --disable-dict --disable-telnet --disable-tftp --disable-pop3 --disable-imap --disable-smtp --disable-gopher --disable-manual --disable-libcurl-option --disable-ipv6 --disable-threaded-resolver --disable-pthreads --disable-verbose --disable-tls-srp --enable-unix-sockets --disable-cookies --without-winssl --without-schannel --without-darwinssl --without-polarssl --without-mbedtls --without-cyassl --without-wolfssl --without-mesalink --without-nss --without-libpsl --without-libmetalink --without-librtmp --without-winidn --without-libidn2 --enable-static --prefix=/root/Code/zombieant/ext/curl`

Make:

`make curl_LDFLAGS=-all-static`

### LibcJson:
git clone https://github.com/DaveGamble/cJSON

`make all`
`make install DESTDIR="/root/Code/dist/cJSON"`
`cp libcjson.a /root/Code/dist/cJSON/usr/local/lib`

Example: `gcc -I. -o main main.c libcjson.a`
