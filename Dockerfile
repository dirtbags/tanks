FROM golang:1.27
WORKDIR /app
COPY . /app
RUN make
RUN mkdir rounds && \
    chown 65534 rounds
COPY examples /app/tanks
USER 65534
ENTRYPOINT [ "./tanksd" ]
