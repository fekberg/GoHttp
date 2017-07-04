FROM alpine 

RUN apk update && apk add \
   alpine-sdk \
   automake \
   autoconf \
   libtool \
   git \
   valgrind

RUN curl -sL https://github.com/libcheck/check/releases/download/0.11.0/check-0.11.0.tar.gz | tar xz

RUN cd check-0.11.0 && autoreconf --install && ./configure && make && make install && cd ..

RUN ldconfig /

CMD ["/bin/sh"]
