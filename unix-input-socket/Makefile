build:
	cmake -B_builds -DTARGET_GROUP=test
	cmake --build _builds

release:
	cmake -B_builds -DTARGET_GROUP=release
	cmake --build _builds --verbose

clean:
	rm -rf _builds
