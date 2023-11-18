#!/bin/sh

cd $(dirname $0)/../www

export BASE_PATH=/tanks/www/state/

(
  httpd-foreground
) &

(
  while true; do
    ../bin/round.sh /tanks/www/state/*
    sleep 60
  done
) &

wait
