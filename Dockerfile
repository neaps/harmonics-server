FROM ubuntu:18.04
MAINTAINER Kevin Miller <kevin@kevee.net>

WORKDIR /tmp

RUN apt-get update
RUN apt-get -y install build-essential nodejs wget npm libtool-bin gfortran octave
RUN apt-get -y install less

RUN wget https://flaterco.com/files/xtide/congen-1.7.tar.xz && tar -xf congen-1.7.tar.xz
RUN wget https://flaterco.com/files/xtide/harmgen-3.1.3.tar.xz && tar -xf harmgen-3.1.3.tar.xz

COPY patches .

ENV LD_LIBRARY_PATH=/usr/local/lib/

RUN cd congen-1.7/ && ./configure && make && make install
RUN cd harmgen-3.1.3/ && patch harmgen-orig.cc < ../harmgen-json.patch && ./configure  && make && make install


WORKDIR /usr/src/app

COPY app/package*.json ./
RUN npm install

COPY app .

EXPOSE 8080
CMD [ "npm", "start" ]