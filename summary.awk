#! /usr/bin/awk -f

function esc(s) {
	gsub(/&/, "&amp;", s);
	gsub(/</, "&lt;", s);
	gsub(/>/, "&gt;", s);
	return s;
}

BEGIN {
    ngames = 20;
    getline rounds < "next-round";

	print "<!DOCTYPE html>";
	print "<html>";
	print "  <head>";
	print "	<title>Dirtbags Tanks</title>";
	print "	<link rel=\"stylesheet\" href=\"style.css\" type=\"text/css\">";
	print "  </head>";
	print "  <body>";
	print "	<h1>Dirtbags Tanks</h1>";

    print "    <p>New here?  Read the <a href=\"intro.html\">introduction</a>.</p>";
    print "    <p>New round every minute.</p>";
    print "    <h2>Rankings</h2>";
    print "    <p>Over the last " ngames" games only.</p>";
    print "    <ol>";
	for (i = rounds - ngames - 1; i > 0 && i < rounds; i += 1) {
		fn = sprintf("round-%04d.html", i)
		while (getline < fn) {
			if ($2 == "score") {
				scores[$3] += $4
				if (scores[$3] > topscore) {
					topscore = scores[$3]
				}
			}
		}
	}

	for (id in scores) {
        if (1 == getline < (id "/name")) {
            names[id] = esc($0)
        } else {
            names[id] = "<i>Unnamed</i>"
        }

        getline < (id "/color")
        if (/^#[0-9A-Fa-f]+$/) {
            color[id] = $0
        } else {
            color[id] = "#c0c0c0"
        }
    }

	for (s = topscore; s >= 0; s -= 1) {
		for (id in scores) {
			if (scores[id] == s) {
				printf("<li><span class=\"swatch\" style=\"background-color: %s;\">#</span> %s (%d points)</li>\n", color[id], names[id], scores[id]);
			}
		}
	}
    print "    </ol>";

    print "    <h2>Rounds</h2>";
    print "    <ul>";
    for (i = rounds - 1; (i >= rounds - 721) && (i > 0); i -= 1) {
        printf("<li><a href=\"round-%04d.html\">%04d</a></li>\n", i, i);
    }
    print "    </ul>";

	while (getline < ENVIRON["NAV_HTML_INC"]) {
		print;
	}

	print "  </body>";
	print "</html>";
}
