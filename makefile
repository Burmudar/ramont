GO_RUN_CMD = go run main.go
WEBPACK_CMD = node_modules/.bin/webpack --config webpack.config.js --colors --progress --display-error-details

run-server:
	$(GO_RUN_CMD)

build-ts:
	$(WEBPACK_CMD)

run:
	$(WEBPACK_CMD)
	$(GO_RUN_CMD)
