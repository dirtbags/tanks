#! /bin/sh

if [ -f next-round ]; then
    next=$(cat next-round)
else
    next=0
fi
expr $next + 1 > next-round

fn=$(printf "round-%04d.html" $next)


echo -n "Running round $next... "
cat <<EOF > $fn
<!DOCTYPE html>
<html>
  <head>
    <title>Tanks Round $next</title>
    <script type="application/javascript" src="tanks.js"></script>
    <style type="text/css">
      body {
        background-color: #444444;
        color: white;
      }
    </style>
    <script type="application/javascript">
      function go() {
        start(
// Start JSON data
EOF
./run-tanks players/* >> $fn
cat <<EOF >> $fn
// end JSON data
      );
}
window.onload = go;
    </script>
  </head>
  <body>
    <div id="game_box"><canvas id="battlefield"></canvas></div>
    <p><span id="fps">0</span> fps</p>
  </body>
</html>
EOF
echo "done."

