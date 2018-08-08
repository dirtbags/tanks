FROM alpine AS builder

RUN apk --no-cache add build-base m4 markdown
COPY . /src
RUN make -C /src DESTDIR=/tanks install

FROM neale/eris
COPY --from=builder /tanks /tanks
CMD [ "/tanks/bin/go.sh" ]
