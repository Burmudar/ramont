build-example:
	clang -Wall --std=c17 -I/usr/include/libevdev-1.0 -levdev example.c

build-unix: unix_socket.c
	clang -Wall --std=c17 -I/usr/include/libevdev-1.0 -levdev unix_socket.c -o unix

run-unix: build-unix
	./unix
