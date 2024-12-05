FROM golang:1.22
WORKDIR /app
COPY *.go *.c *.h Makefile www /app
RUN make
RUN mkdir rounds
COPY examples /app/tanks
ENTRYPOINT [ "./tanksd" ]
