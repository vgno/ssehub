FROM buildpack-deps:jessie
MAINTAINER Ole Fredrik Skudsvik <oles@vg.no>

RUN apt-get -y update && \
  apt-get -y install g++ make libgoogle-glog-dev libboost-dev \
  libboost-system-dev libboost-thread-dev \
  libboost-program-options-dev librabbitmq-dev libleveldb-dev

WORKDIR /ssehub
ADD . /ssehub
RUN make

EXPOSE 8080

CMD ["./ssehub"]
