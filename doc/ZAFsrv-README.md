
## Usage:

1. Start ZAF background daemon: 

`./bin/zaf`

_Note_: it will rename itself in process tabel accordingly to your configuration

2. Reference client operation:

For details on a protocol please see _ZAF Command Protocol_ section below

```
Usage: ./tools/zclient.sh <cmd> [cmd args]
Example: ./tools/zclient.sh listmod
Example: ./tools/zclient.sh loadmod http://127.0.0.1:8080/hax.so
Example: ./tools/zclient.sh loadmod file://tmp/hax.so
Example: ./tools/zclient.sh unloadmod /hax.so
```
*TODO*: HTTP/S is not tested yet but should be supported

## Build

There are a couple of ways to build ZAF dependencies:  static and dynamic.
Dynamic would look for shared libraries on destination machine. 
Static would create a _mostly_ self-contained executable. The one issue I am not able to solve is to include libnss functons like `gethostbyaddr`. This needs to be tested if you are dropping a binary without compilation. 

In general, ZAF would need _libOpenSSL_, _libcURL_ and a couple of one-file libraries already included in `/extsrc` directory.
Below is how to build them. The build is mostly on Kali - Debian 9 testing. CentOS notes where applicable.

```
CentOS - libc 2.17
Kali   - libc 2.27
```

### _libOpenSSL_: Static

[Building OpenSSL](https://wiki.openssl.org/index.php/Compilation_and_Installation)

1. Fetch current (_3.0.0-dev_) OpenSSL repo: 

`git clone git://git.openssl.org/openssl.git`

_Note_:CentOS has static glibc, and so it may depend on:

`yum install gcc glibc-static libstdc++-static -y`

2. Configure:

```
./config no-shared no-threads no-dso no-engine no-err no-psk no-srp no-ec2m no-weak-ssl-ciphers no-idea no-comp no-ssl2 no-ssl3 -DOPENSSL_USE_IPV6=0 -static -static-libgcc --prefix=/root/Code/dist/openssl
```

3. Make: 

```
make 
make install_sw
```

###  _libcURL_: Static

1. Fetch current (_7.64.0_) cURL repo: `git clone https://github.com/curl/curl`

_Note_: CentOS depends:

`yum install libtool autoconf automake nroff m4 -y`

2. `./buildconf `

3. Configure:


```
CPPFLAGS="-I/root/Code/zombieant/ext/openssl/include" LDFLAGS="-L/root/Code/zombieant/ext/openssl/lib -static" ./configure --with-ssl -without-zlib --disable-debug --enable-optimize --disable-curldebug  --disable-ares  --disable-shared --disable-thread --disable-rt --disable-largefile --disable-ldap --disable-ldaps --disable-rtsp --disable-dict --disable-telnet --disable-tftp --disable-pop3 --disable-imap --disable-smtp --disable-gopher --disable-manual --disable-libcurl-option --disable-ipv6 --disable-threaded-resolver --disable-pthreads --disable-verbose --disable-tls-srp --enable-unix-sockets --disable-cookies --without-winssl --without-schannel --without-darwinssl --without-polarssl --without-mbedtls --without-cyassl --without-wolfssl --without-mesalink --without-nss --without-libpsl --without-libmetalink --without-librtmp --without-winidn --without-libidn2 --enable-static --prefix=/root/Code/zombieant/ext/curl
```


4. Make:

`make curl_LDFLAGS=-all-static`

5. Copy `libcurl.a` to ZAF `/lib`

### libcJSON

ref: [cJSON] (https://github.com/DaveGamble/cJSON)

1. Fetch source: `git clone https://github.com/DaveGamble/cJSON`

2. Build 

```
make all
make install DESTDIR="/root/Code/dist/cJSON"
```


3. Copy `libcjson.a` to ZAF `/lib`:


### Other External Source dependencies (source only):

One-file or small supporting utilities:

[septroctitle] (https://github.com/simplegeo/setproctitle/blob/master/src/spt_status.c)
[C99 log.c] (https://github.com/rxi/log.c)

### ZAF

1. Run:

`make`

Warnings are from functions from libraries I was unable to include in the static build (it will look on dest system, so may want to build on the same distro).

```
make -C ./src
make[1]: Entering directory '/root/Code/zombieant/zafsrv/src'
/usr/bin/ld: /root/Code/zombieant/zafsrv/lib/libcurl.a(libcurl_la-netrc.o): in function `Curl_parsenetrc':
netrc.c:(.text+0x434): warning: Using 'getpwuid_r' in statically linked applications requires at runtime the shared libraries from the glibc version used for linking
/usr/bin/ld: /root/Code/zombieant/zafsrv/lib/libcurl.a(libcurl_la-curl_addrinfo.o): in function `Curl_getaddrinfo_ex':
curl_addrinfo.c:(.text+0x1b0): warning: Using 'getaddrinfo' in statically linked applications requires at runtime the shared libraries from the glibc version used for linking
/usr/bin/ld: /root/Code/zombieant/zafsrv/lib/libcrypto.a(libcrypto-lib-b_sock.o): in function `BIO_gethostbyname':
b_sock.c:(.text+0x251): warning: Using 'gethostbyname' in statically linked applications requires at runtime the shared libraries from the glibc version used for linking
make[1]: Leaving directory '/root/Code/zombieant/zafsrv/src'

```

## ZAF Command Protocol

- Listmod:

```json
{"command": "listmod"}

```

- Loadmod:

```json
{ 
 "command": "loadmod", 
 "args": [ 
       { "modurl": "http://127.0.0.1:8080/hax.so" } 
  ] 
}
```

- Unloadmod:

```json
{ 
 "command": "unloadmod", 
 "args": [ 
       { "modname": "hax.so" } 
  ] 
}
```
