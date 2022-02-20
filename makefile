UNIX_BASE_PATH =./unix-input-socket/_builds
UNIX_CMD = ./uv_socket
SERVER_NAME = ramont-server
GO_RUN_CMD = go run cmd/ramont/main.go
GO_BUILD_CMD = go build -o ${SERVER_NAME} cmd/ramont/main.go
WEBPACK_CMD = node_modules/.bin/webpack --config webpack.config.js --colors --progress --display-error-details

run-server:
	$(GO_RUN_CMD)

build-ts:
	$(WEBPACK_CMD)

build-unix:
	cd unix-input-socket && cmake -B_builds -DTARGET_GROUP=release && cmake --build _builds
unix:
	rm -f ${UNIX_BASE_PATH}/uv.socket
	cd ${UNIX_BASE_PATH} && $(UNIX_CMD)

run:
	$(WEBPACK_CMD)
	RAMONT_STATIC=static $(GO_RUN_CMD)

build-server:
	CGO_ENABLED=0 $(GO_BUILD_CMD)

build-pyramont:
	cd pyramont && poetry build

release-backend:
	#$(MAKE) build-unix
	$(MAKE) build-pyramont
	$(MAKE) build-server
	#cp -f ${UNIX_BASE_PATH}/uv_socket ${SRC}/vm/vm-share/
	cp -f pyramont/dist/*.whl ${SRC}/vm/vm-share
	cp -f ramont-server ${SRC}/vm/vm-share/
