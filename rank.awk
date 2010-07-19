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
    tanks[i++] = $1;
    reason[$1] = $3;
    killer[$1] = $4;
    errchar[$1] = $5;
    lasterr[$1] = $6;
    kills[$4] += 1;

    getline < ($2 "/name");
    name[$1] = $0;
}

END {
    print "<table id=\"results\">";
    print "<tr><th>Name</th><th>Kills</th><th>Cause of Death</th><th>Killer</th><th>Last Error</th><th>Parse error @</th></tr>";
    for (me in name) {
        print "<tr>";
        print "<td>" esc(name[me]) "</td>";
        print "<td>" kills[me] "</td>";
        print "<td>" reason[me] "</td>";
        print "<td>" esc(name[killer[me]]) "</td>";
        print "<td>" lasterr[me] "</td>";
        print "<td>" errchar[me] "</td>";
        print "</tr>";
    }
    print "</table>";
}