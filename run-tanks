#! /bin/sh

if [ "$#" -gt 0 ]; then
    tanks="$@"
else
    echo "Usage: $0 tank1 tank2 [...]"
    echo "Writes ./next-round and ./summary.html"
    exit 1
fi

TANKS_GAME=${TANKS_GAME:-forftanks}
NAV_HTML_INC=${NAV_HTML_INC:-./nav.html.inc} export NAV_HTML_INC

# Add wherever this lives to the search path
PATH=$PATH:$(dirname $0)

if [ -f next-round ]; then
    next=$(cat next-round)
else
    next=0
fi
expr $next + 1 > next-round

fn=$(printf "round-%04d.html" $next)
rfn=results$$.txt

# Clean up old games
ofn=$(printf "round-%04d.html" $(expr $next - 720))
echo "Removing $ofn"
rm -f $ofn


echo -n "Running round $next... "
cat <<EOF >$fn
<!DOCTYPE html>
<html>
  <head>
    <title>Tanks Round $next</title>
    <link rel="stylesheet" href="//code.jquery.com/ui/1.11.0/themes/ui-darkness/jquery-ui.css" type="text/css">
    <script src="//code.jquery.com/jquery-1.11.0.min.js"></script>
    <script src="//code.jquery.com/ui/1.11.0/jquery-ui.min.js"></script>
    <script type="application/javascript" src="tanks.js"></script>
    <link rel="stylesheet" href="style.css" type="text/css">
    <script type="application/javascript">
      function go() {
        start("battlefield",
// Start JSON data
EOF
$TANKS_GAME $tanks >>$fn 3>$rfn
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
rank.awk $rfn >>$fn
rm -f $rfn
cat $NAV_HTML_INC >>$fn
cat <<EOF >>$fn
  </body>
</html>
EOF

summary.awk $tanks > summary.html.$$ && mv summary.html.$$ summary.html

echo "done."

