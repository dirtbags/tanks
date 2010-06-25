#! /usr/bin/python

# Binary radians with B12 binary scaling (multiply floats by 256)
# pi radians = 128 brads

import math

brad2rad = math.pi/128

print "static bs_t bs_cos_table[] = {"
for i in range(129):
    r = (i * math.pi) / 128.0
    cos = math.sin(r)
    bcos = int(round(cos * 256))
    print ("%3d," % bcos),
print "}"

print "static brad_t bs_asin_table[] = {"
for i in range(257):
    f = i / 256.0
    acos = math.asin(f)
    bacos = int(round(256 * acos / math.pi))
    print ("%3d," % bacos),
print "}"

