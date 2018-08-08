#! /bin/sh

(
  cd /tanks/www
  s6-tcpserver -u 80 -g 80 0.0.0.0 80 /usr/bin/eris -c -.
) &

(
  cd /tanks
  while true; do
    ./round.sh user/*/
    sleep 60
  done
) &

wait
