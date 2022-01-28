FROM mcr.microsoft.com/vscode/devcontainers/cpp:0-ubuntu-21.04

RUN apt update -y
RUN apt install -y catch
RUN apt install -y libboost-regex-dev

COPY Dockerfile vscode_cache.tg[z] ./
RUN tar xvzf vscode_cache.tgz -C / || echo "no cache found."
