build-example:
	clang -Wall --std=c17 -I/usr/include/libevdev-1.0 -levdev example.c

build-queue:
	rm -rfv q_test
	clang -std=c11 -pedantic-errors -g -v -Wall queue_test.c queue.c -o q_test && ./q_test

build-unix: unix_socket.c
	clang -Wall --std=c17 -I/usr/include/libevdev-1.0 -levdev unix_socket.c -o unix

run-unix: build-unix
	./unix
