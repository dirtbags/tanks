#ifndef __DUMP_H__

#include <stdio.h>

#ifndef TAU
#define TAU 6.28318530717958647692
#endif

/* Debugging */
#define DUMPf(fmt, args...) fprintf(stderr, "%s:%s:%d " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##args)
#define DUMP() DUMPf("")
#define DUMP_d(v) DUMPf("%s = %d", #v, v)
#define DUMP_x(v) DUMPf("%s = 0x%x", #v, v)
#define DUMP_s(v) DUMPf("%s = %s", #v, v)
#define DUMP_c(v) DUMPf("%s = %c", #v, v)
#define DUMP_f(v) DUMPf("%s = %f", #v, v)
#define DUMP_p(v) DUMPf("%s = %p", #v, v)
#define DUMP_xy(v) DUMPf("%s = (%f, %f)", #v, v[0], v[1]);
#define DUMP_angle(v) DUMPf("%s = %.3fτ", #v, (v/TAU));

/* Tektronix 4014 drawing */
#define TEK_ENABLE "\033[?38h"
#define TEK_DISABLE "\033\003"
#define TEK(fmt, args...) fprintf(stderr, TEK_ENABLE fmt TEK_DISABLE, ##args)
#define TEK_coord(x, y) ((int)y/32)+32,((int)y%32)+96,((int)x/32)+32,((int)x%32)+64

#define TEK_cls() TEK("\033\014")
#define TEK_line(x1, y1, x2, y2) TEK("\035%c%c%c%c%c%c%c%c", TEK_coord(x1, y1), TEK_coord(x2, y2))
#define TEK_point(x, y) TEK("\034%c%c%c%c", TEK_coord(x, y))
#define TEK_text(x, y, s) TEK("\035%c%c%c%c\037%s", TEK_coord(x, y), s)

#endif
