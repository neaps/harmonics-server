FROM ubuntu:18.04
MAINTAINER Kevin Miller <kevin@kevee.net>

WORKDIR /tmp

RUN apt-get update
RUN apt-get -y install build-essential nodejs wget npm libtool-bin gfortran octave
RUN apt-get -y install less

COPY lib .

ENV LD_LIBRARY_PATH=/usr/local/lib/

RUN cd congen-1.7/ && ./configure && make && make install
RUN cd harmgen-json-3.1.3/ && ./configure  && make && make install


WORKDIR /usr/src/app
COPY app .

RUN npm install
RUN  harmgen /usr/local/share/harmgen/congen_1yr.txt levels-hourly.txt out.json

#CMD ["less", "out.json"]
CMD [ "npm", "start" ]