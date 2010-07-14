#ifndef __DUMP_H__

/* Debugging help */
#define DUMPf(fmt, args...) fprintf(stderr, "%s:%s:%d " fmt "\n", __FILE__, __FUNCTION__, __LINE__, ##args)
#define DUMP() DUMPf("")
#define DUMP_d(v) DUMPf("%s = %d", #v, v)
#define DUMP_x(v) DUMPf("%s = 0x%x", #v, v)
#define DUMP_s(v) DUMPf("%s = %s", #v, v)
#define DUMP_c(v) DUMPf("%s = %c", #v, v)
#define DUMP_f(v) DUMPf("%s = %f", #v, v)
#define DUMP_p(v) DUMPf("%s = %p", #v, v)
#define DUMP_xy(v) DUMPf("%s = (%f, %f)", #v, v[0], v[1]);

#endif
