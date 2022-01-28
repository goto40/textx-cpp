FROM mcr.microsoft.com/vscode/devcontainers/cpp:0-ubuntu-21.04

COPY Dockerfile vscode_cache.tg[z] ./
RUN tar xvzf vscode_cache.tgz -C / || echo "no cache found."
