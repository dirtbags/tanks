#! /usr/bin/awk -f

BEGIN {
    FS = "\t";
}

function esc(s) {
    gsub(/&/, "&amp;", s);
    gsub(/</, "&lt;", s);
    gsub(/>/, "&gt;", s);
    return s;
}

{
    id = $1;
    ntanks += 1;
    tanks[id] = id;
    if ($4 == "(nil)") {
        score[id] += 1;
    } else {
        reason[id] = $3;
        killer[id] = $4;
        kills[$4] += 1;
        score[$4] += 1;
    }
    path[id] = $2;
    if ($5) {
        lasterr[id] = $6 " around char " $5;
    } else {
        lasterr[id] = $6;
    }

    if (1 == getline < (path[id] "/name")) {
        name[id] = esc($0);
    } else {
        name[id] = "<i>Unnamed</i>";
    }

    getline < (path[id] "/color");
    if (/^#[0-9A-Fa-f]+$/) {
        color[id] = $0;
    } else {
        color[id] = "#c0c0c0";
    }
}

END {
    # Fill in who killed whom
    for (id in tanks) {
        if (score[id] > topscore) {
            winner = id;
            topscore = score[id];
        }
        if (killer[id]) {
            reason[id] = reason[id] " (<span style='color:" color[killer[id]] ";'>" name[killer[id]] "</span>)";
        }
		# XXX: track points a different way
        # print score[id] >> (path[id] "/points");
    }

    # Output the table
    print "<table id=\"results\">";
    print "<tr><th>Name</th><th>Score</th><th>Cause of Death</th><th>Last Error</th></tr>";
    for (i = ntanks; i >= 0; i -= 1) {
        for (me in tanks) {
            if (score[me] == i) {
                if (me == winner) {
                    style = "style=\"font-weight: bold; background-color: #666666\"";
                } else {
                    style = "";
                }
                printf("<tr " style ">");
                printf("<td><span class=\"swatch\" style=\"background-color: " color[me] "\">#</span> " name[me] "</td>");
                printf("<td>" score[me] "</td>");
                printf("<td>" reason[me] "</td>");
                printf("<td>" lasterr[me] "</td>");
                printf("</tr>\n");
				printf("<!-- score " path[me] " " i " -->\n");
            }
        }
    }
    print "</table>";
}
