FROM alpine AS builder

RUN apk --no-cache add build-base
COPY . /src
RUN make -C /src DESTDIR=/tanks install

FROM neale/eris
COPY builder:/opt/tanks /tanks
CMD [ "/opt/tanks/go.sh" ]
