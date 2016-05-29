FROM debian:jessie

RUN apt-get update && \
    apt-get install -y \
        build-essential \
        gcc \
        libjpeg-turbo-progs \
        imagemagick \
        libX11-dev \
        libxinerama-dev \
        libimlib2-dev \
        libcurl4-openssl-dev \
        libxt-dev \
        giblib-dev \
        git
COPY . /src
WORKDIR /src
CMD ["make"]
