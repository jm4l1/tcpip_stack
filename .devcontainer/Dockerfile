FROM debian:bookworm-slim

ENV DEBIAN_FRONTEND noninteractive
WORKDIR /usr/src/app

RUN apt-get update
RUN apt-get install -y  git build-essential clang-format cmake
COPY .bash_profile /root/.bashrc
