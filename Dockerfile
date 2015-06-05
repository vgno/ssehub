FROM debian:latest

RUN apt-get -y update
RUN apt-get -y install g++ make libgoogle-glog-dev libboost-dev librabbitmq-dev
RUN mkdir /ssehub

ADD . /ssehub
WORKDIR /ssehub
RUN make

EXPOSE 8090

CMD ["./ssehub"]


