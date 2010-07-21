#! /usr/bin/awk -f

function esc(s) {
    gsub(/&/, "&amp;", s);
    gsub(/</, "&lt;", s);
    gsub(/>/, "&gt;", s);
    return s;
}

BEGIN {
    print "<!DOCTYPE html>";
    print "<html>";
    print "  <head>";
    print "    <title>Dirtbags Tanks</title>";
    print "    <link rel=\"stylesheet\" href=\"dirtbags.css\" type=\"text/css\">";
    print "  </head>";
    print "  <body>";
    print "    <h1>Dirtbags Tanks</h1>";

    print "    <h2>Resources</h2>";
    print "    <ul>";
    print "      <li><a href=\"intro.html\">Introduction</a></li>";
    print "      <li><a href=\"forf.html\">Forf manual</a></li>";
    print "      <li><a href=\"procs.html\">Tanks procedures</a></li>";
    print "      <li><a href=\"designer.html\">Tanks designer</a></li>";
    print "    </ul>";

    print "    <h2>Rankings</h2>";
    print "    <ol>";
    for (i = 1; i < ARGC; i += 1) {
        id = ARGV[i];

        if (1 == getline < (id "/name")) {
            names[id] = esc($0);
        } else {
            names[id] = "<i>Unnamed</i>";
        }

        getline < (id "/color");
        if (/^#[0-9A-Fa-f]+$/) {
            color[id] = $0;
        } else {
            color[id] = "#c0c0c0";
        }

        p = 0;
        while (1 == getline < (id "/points")) {
            p += $0;
        }
        scores[p] = p;
        points[id] = p;
        nscores
    }
    while (1) {
        # Find highest score
        maxscore = -1;
        for (p in scores) {
            if (p > maxscore) {
                maxscore = p;
            }
        }
        if (maxscore == -1) {
            break;
        }
        delete scores[maxscore];

        for (id in points) {
            if (points[id] == maxscore) {
                printf("<li><span class=\"swatch\" style=\"background-color: %s;\">#</span> %s (%d wins)</li>\n", color[id], names[id], points[id]);
            }
        }
    }
    print "    </ol>";

    print "    <h2>Rounds</h2>";
    print "    <ul>";
    getline rounds < "next-round";
    for (i = rounds - 1; i >= 0; i -= 1) {
        printf("<li><a href=\"round-%04d.html\">%04d</a></li>\n", i, i);
    }
    print "    </ul>";
    print "  </body>";
    print "</html>";
}
