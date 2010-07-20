#! /bin/sh

cat <<EOF
<html>
  <head>
    <title>Forf Manual</title>
    <meta charset="utf-8">
    <link rel="stylesheet" href="dirtbags.css" type="text/css">
  </head>
  <body>
EOF
markdown forf/forf.txt
cat <<EOF
  </body>
</html>
EOF
