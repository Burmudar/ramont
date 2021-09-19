UNIX_BASE_PATH =./unix-input-socket/_builds
UNIX_CMD = ./uv_socket
GO_RUN_CMD = go run cmd/ramont/main.go
WEBPACK_CMD = node_modules/.bin/webpack --config webpack.config.js --colors --progress --display-error-details

run-server:
	$(GO_RUN_CMD)

build-ts:
	$(WEBPACK_CMD)

unix:
	rm -f ${UNIX_BASE_PATH}/uv.socket
	cd ${UNIX_BASE_PATH} && $(UNIX_CMD)

run:
	$(WEBPACK_CMD)
	RAMONT_STATIC=static $(GO_RUN_CMD)
