#! /usr/bin/python

import math

print "static float trig_cos_table[] = {"
for i in range(91):
    r = (i * math.pi) / 180.0
    cos = math.cos(r)
    print ("%0f," % cos),
    if 5 == i % 6:
        print
print "};"
