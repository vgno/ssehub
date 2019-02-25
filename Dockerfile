FROM buildpack-deps:bionic
MAINTAINER Ole Fredrik Skudsvik <oles@vg.no>

RUN apt-get -y update && \
  apt-get -y install g++ make cmake libgoogle-glog-dev libboost-dev \
  libboost-system-dev libboost-thread-dev \
  libboost-program-options-dev librabbitmq-dev libleveldb-dev

RUN mkdir -p /usr/src/ssehub
WORKDIR /usr/src/ssehub
COPY . .

RUN make && make install

WORKDIR /tmp
RUN rm -rf /usr/src/ssehub

RUN useradd -d /tmp -s /bin/false ssehub
RUN chown -R ssehub:ssehub /etc/ssehub

USER ssehub

ENTRYPOINT ["/usr/bin/ssehub"]
