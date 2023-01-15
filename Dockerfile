FROM ubuntu:latest

ENV DEBIAN_FRONTEND noninteractive
COPY . /src
WORKDIR /src
RUN apt-get update && apt-get -y install sudo ca-certificates git
RUN ./install-deps.sh
RUN ./build.sh
RUN ./test.sh
