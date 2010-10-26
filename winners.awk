#! /usr/bin/awk -f

BEGIN {
    FS = "\t";
}

{
    tanks[$1] = $1;
    if ($4 == "(nil)") {
        p = $1;
    } else {
        p = $4;
    }
    score[p] += 1;
    if (score[p] > topscore) {
        topscore = score[p];
    }
}

END {
    for (id in tanks) {
        if (score[id] == topscore) {
            print id;
        }
    }
}