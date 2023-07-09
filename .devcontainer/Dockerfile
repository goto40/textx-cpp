FROM mcr.microsoft.com/vscode/devcontainers/cpp:0-ubuntu-22.04

RUN apt update -y
RUN apt install -y catch
RUN apt install -y libboost-regex-dev

RUN git clone https://github.com/andreasbuhr/cppcoro.git
RUN cp -r cppcoro/include/cppcoro /usr/include 

RUN curl -fsSL https://deno.land/x/install/install.sh | sh

COPY Dockerfile vscode_cache.tg[z] ./
RUN tar xvzf vscode_cache.tgz -C / || echo "no cache found."

RUN chmod -R a+rx /root

RUN apt install -y libboost-program-options-dev

ENV DENO_INSTALL="/root/.deno"
ENV PATH="${DENO_INSTALL}/bin:${PATH}"