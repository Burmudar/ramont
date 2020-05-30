from golang:latest

RUN  apt-get update
RUN DEBIAN_FRONTEND="noninteractive" apt-get -y install gcc libc6-dev libx11-dev xorg-dev libxtst-dev libpng++-dev xcb libxcb-xkb-dev x11-xkb-utils libx11-xcb-dev libxkbcommon-x11-dev libxkbcommon-dev xsel xclip golang git

RUN mkdir /app
WORKDIR /app

ADD . /app

RUN go mod tidy

ENTRYPOINT ["/bin/bash", "-c", "go mod tidy && go build ."]
