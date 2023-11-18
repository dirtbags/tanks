FROM docker.io/library/httpd:2.4-alpine3.18 AS builder

RUN apk --no-cache add \
    build-base \
    m4 \
    markdown

COPY . /src
RUN make -C /src DESTDIR=/tanks && \ 
    make -C /src DESTDIR=/tanks install

#    dos2unix src/go.sh src/rank.awk src/round.sh src/summary.awk && \


FROM docker.io/library/httpd:2.4-alpine3.18
COPY --from=builder /tanks/bin /tanks/bin
COPY --from=builder /tanks/www /tanks/www-orig
COPY httpd.conf /usr/local/apache2/conf/httpd.conf
#RUN sed -i 's/\/usr\/local\/apache2\/htdocs/\/tanks\/www/' /usr/local/apache2/conf/httpd.conf && \
#    sed -i 's/Options Indexes FollowSymLinks/Options Indexes FollowSymLinks ExecCGI\n    AddHandler cgi-script .cgi\n    SetEnv BASE_PATH \/tanks\/tanks\//' /usr/local/apache2/conf/httpd.conf
CMD [ "sh", "-c", "if [[ ! -d /tanks/www/ ]]; then mkdir /tanks/www/; fi && cp -ru /tanks/www-orig/* /tanks/www/ & /tanks/bin/go.sh" ]