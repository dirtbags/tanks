#! /usr/bin/awk -f

BEGIN {
    FS = "\t";
}

{
    tanks[$1] = $2;
    if ($4 == "(nil)") {
        p = $1;
    } else {
        p = $4;
    }
    score[p] += 1;
    if (score[p] == topscore) {
        winners += 1;
    } else if (score[p] > topscore) { 
        winners = 1;
        topscore = score[p];
    }
}

END {
    if (winners > 1) {
        exit;
    }
    for (id in tanks) {
        if (score[id] == topscore) {
            print tanks[id];
        }
    }
}