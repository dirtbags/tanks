#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

char *BASE_PATH = "";

struct {
  char   *name;
  size_t  size;
} entries[] = {
  {"name", 20},
  {"author", 40},
  {"color", 10},
  {"program", 8192},
  {NULL, 0}
};


size_t inlen;

int
read_char()
{
  if (inlen) {
    inlen -= 1;
    return getchar();
  }
  return EOF;
}

char
tonum(int c)
{
  if ((c >= '0') && (c <= '9')) {
    return c - '0';
  }
  if ((c >= 'a') && (c <= 'f')) {
    return 10 + c - 'a';
  }
  if ((c >= 'A') && (c <= 'F')) {
    return 10 + c - 'A';
  }
  return 0;
}

char
read_hex()
{
  int a = read_char();
  int b = read_char();

  return tonum(a)*16 + tonum(b);
}

/* Read a key or a value.  Since & and = aren't supposed to appear
   outside of boundaries, we can use the same function for both.
*/
size_t
read_item(char *str, size_t maxlen)
{
  int    c;
  size_t pos = 0;

  while (1) {
    c = read_char();
    switch (c) {
      case EOF:
      case '=':
      case '&':
        str[pos] = '\0';
        return pos;
      case '%':
        c = read_hex();
        break;
      case '+':
        c = ' ';
        break;
    }
    if (pos < maxlen - 1) {
      str[pos] = c;
      pos += 1;
    }
  }
}

size_t
copy_item(char *filename, size_t maxlen)
{
  FILE   *f;
  char    path[132];
  int     c;
  size_t  pos = 0;

  snprintf(path, sizeof(path),
           "%s%05d.%s", BASE_PATH,
           getpid(), filename);
  f = fopen(path, "w");
  if (! f) {
    /* Just send it to the bit bucket */
    maxlen = 0;
  }

  while (1) {
    c = read_char();
    switch (c) {
      case EOF:
      case '=':
      case '&':
        if (f) fclose(f);
        return pos;
      case '%':
        c = read_hex();
        break;
      case '+':
        c = ' ';
        break;
    }
    if (pos < maxlen) {
      fputc(c, f);
      pos += 1;
    }
  }
}

int
croak(char *msg)
{
  int  i;
  char path[132];

  for (i = 0; entries[i].name; i += 1) {
    snprintf(path, sizeof(path),
             "%s%05d.%s", BASE_PATH,
             getpid(), entries[i].name);
    unlink(path);
  }

  printf("Content-type: text/html\n\n");
  printf("<html><head>\n");
  printf("<link rel=\"stylesheet\" href=\"dirtbags.css\" type=\"text/css\">\n");
  printf("<title>Tank submission error</title>\n");
  printf("</head><body><h1>Tank submission error</h1>\n");
  if (msg) {
    printf("<p>%s.</p>\n", msg);
  } else {
    printf("<p>Something went wrong.</p>.\n");
  }
  printf("<p>Sorry it didn't work out.</p>\n");
  printf("<p>You could go back and try again, though.</p>\n");
  printf("</body></html>\n");

  return 0;
}

int
main(int argc, char *argv[])
{
  int    sensor[10][4];
  char   key[20];
  char   token[40];
  size_t len;

  memset(sensor, 0, sizeof(sensor));
  token[0] = '\0';

  BASE_PATH = getenv("BASE_PATH");
  if (! BASE_PATH) {
    BASE_PATH = "";
  }

  {
    char *rm = getenv("REQUEST_METHOD");

    if (! (rm && (0 == strcmp(rm, "POST")))) {
      printf("405 Method not allowed\n");
      printf("Allow: POST\n");
      printf("Content-type: text/html\n");
      printf("\n");
      printf("<h1>Method not allowed</h1>\n");
      printf("<p>I only speak POST.  Sorry.</p>\n");
      return 0;
    }

    inlen = atoi(getenv("CONTENT_LENGTH"));
  }

  while (inlen) {
    len = read_item(key, sizeof(key));
    if (0 == strcmp(key, "token")) {
      read_item(token, sizeof(token));
    } else if ((3 == len) && ('s' == key[0])) {
      /* sensor dealie, key = "s[0-9][rawt]" */
      char val[5];
      int  n = key[1] - '0';
      int  i;
      int  p;

      read_item(val, sizeof(val));

      if (! (n >= 0) && (n <= 9)) {
        break;
      }
      i = atoi(val);

      switch (key[2]) {
        case 'r':
          p = 0;
          break;
        case 'a':
          p = 1;
          break;
        case 'w':
          p = 2;
          break;
        default:
          p = 3;
          i = (val[0] != '\0');
          break;
      }

      sensor[n][p] = i;
    } else {
      int i;

      for (i = 0; entries[i].name; i += 1) {
        if (0 == strcmp(key, entries[i].name)) {
          len = copy_item(key, entries[i].size);
          break;
        }
      }
    }
  }

  /* Sanitize token */
  {
    char *p = token;

    while (*p) {
      if (! isalnum(*p)) {
        *p = '_';
      }
      p += 1;
    }

    if ('\0' == token[0]) {
      token[0] = '_';
      token[1] = '\0';
    }
  }

  /* Move files into their directory */
  {
    char        path[132];
    char        dest[132];
    struct stat st;
    int         i;

    snprintf(path, sizeof(path), "%s%s/", BASE_PATH, token);
    if (-1 == stat(path, &st)) return croak("Invalid token");
    if (! S_ISDIR(st.st_mode)) return croak("Invalid token");
    for (i = 0; entries[i].name; i += 1) {
      snprintf(path, sizeof(path),
               "%s%05d.%s", BASE_PATH,
               getpid(), entries[i].name);
      snprintf(dest, sizeof(dest),
               "%s%s/%s", BASE_PATH,
               token, entries[i].name);
      rename(path, dest);
    }

    for (i = 0; i < 10; i += 1) {
      FILE *f;

      snprintf(dest, sizeof(dest),
               "%s%s/sensor%d", BASE_PATH,
               token, i);
      f = fopen(dest, "w");
      if (! f) break;

      fprintf(f, "%d %d %d %d\n",
              sensor[i][0],
              sensor[i][1],
              sensor[i][2],
              sensor[i][3]);
      fclose(f);
    }
  }

  printf("Content-type: text/html\n\n");
  printf("<!DOCTYPE html>\n");
  printf("<html><head>\n");
  printf("<link rel=\"stylesheet\" href=\"dirtbags.css\" type=\"text/css\">\n");
  printf("<title>Tank submitted</title>\n");
  printf("</head><body><h1>Tank submitted</h1>\n");
  printf("<p>You just uploaded a tank!</p>\n");
  printf("<p>Let's hope it doesn't suck.</p>\n");
  printf("</body></html>\n");

  return 0;
}
