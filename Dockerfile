FROM golang:1.22
WORKDIR /app
COPY . /app
RUN make
RUN mkdir rounds
COPY examples /app/tanks
ENTRYPOINT [ "./tanksd" ]
