FROM debian:latest
MAINTAINER Ole Fredrik Skudsvik <oles@vg.no>

RUN apt-get -y update
RUN apt-get -y install g++ make libgoogle-glog-dev libboost-dev libboost-system-dev libboost-thread-dev librabbitmq-dev
RUN mkdir /ssehub

ADD . /ssehub
WORKDIR /ssehub
RUN make clean
RUN make

EXPOSE 8090

CMD ["./ssehub"]
