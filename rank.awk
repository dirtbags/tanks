#! /usr/bin/awk -f

##
## C doesn't have good string handling routines, but awk does.  Figuring
## out rankings, who shot whom, and deciding on a "winner" is therefore
## handled with this awk script.
##
## Input looks like this:
##    0xbff81f28      players/sittingduckwithteeth    shot    0xbff82138      0       None
##    0xbff82030      players/sandlion        shot    0xbff82138      0       None
##    0xbff82138      players/chashtank       (null)  (nil)   0       None
##

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
            reason[id] = reason[id] " (" name[killer[id]] ")";
        }
        print score[id] >> (path[id] "/points");
    }

    # Dole out points

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
            }
        }
    }
    print "</table>";
}
