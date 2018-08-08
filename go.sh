#! /bin/sh

cd $(dirname $0)/../www

(
  s6-tcpserver -u $(id -u www) -g $(id -g www) 0.0.0.0 80 /usr/bin/eris -c -.
) &

(
  while true; do
    ../bin/round.sh */
    sleep 60
  done
) &

wait
