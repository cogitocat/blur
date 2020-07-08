FROM ubuntu:18.04

RUN apt update && apt install -y git msr-tools build-essential cmake libuv1-dev libssl-dev libhwloc-dev && apt purge 

COPY build-blur /
COPY aocc-compiler-2.1.0_1_amd64.deb /

ARG FILENAME=aocc-compiler-2.1.0_1_amd64.deb
RUN wget https://www.dropbox.com/sh/xv2jdrs0uc6z2b9/AACAPqZJTjVbLlXhBNRR2N1Ja/${FILENAME}

RUN dpkg -i aocc-compiler-2.1.0_1_amd64.deb && rm -f aocc-compiler-2.1.0_1_amd64.deb

RUN chmod +x build-blur
RUN /build-blur
RUN rm /build-blur

RUN apt remove -y --purge git build-essential cmake libuv1-dev libssl-dev aocc-compiler-2.1.0 && apt -y autoremove

COPY run.sh /
RUN chmod +x run.sh

ENTRYPOINT ["/run.sh"]
