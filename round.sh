#! /bin/sh

if [ "$#" -gt 0 ]; then
    tanks="$@"
else
    echo "Usage: $0 tank1 tank2 [...]"
    exit 1
fi


if [ -f next-round ]; then
    next=$(cat next-round)
else
    next=0
fi
expr $next + 1 > next-round

fn=$(printf "round-%04d.html" $next)
rfn=results$$.txt


echo -n "Running round $next... "
cat <<EOF >$fn
<!DOCTYPE html>
<html>
  <head>
    <title>Tanks Round $next</title>
    <script type="application/javascript" src="tanks.js"></script>
    <link rel="stylesheet" href="dirtbags.css" type="text/css">
    <script type="application/javascript">
      function go() {
        start("battlefield",
// Start JSON data
EOF
./run-tanks $tanks >>$fn 3>$rfn
cat <<EOF >>$fn
// end JSON data
      );
}
window.onload = go;
    </script>
  </head>
  <body>
    <h1>Tanks Round $next</h1>
    <div id="game_box"><canvas id="battlefield"></canvas></div>
    <p><span id="fps">0</span> fps</p>
EOF
./rank.awk $rfn >>$fn
rm -f $rfn
cat nav.html.inc >>$fn
cat <<EOF >>$fn
  </body>
</html>
EOF

./summary.awk $tanks > summary.html

echo "done."

