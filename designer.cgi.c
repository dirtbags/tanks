#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BASE_PATH "/tmp/"

struct string {
  char   *s;
  size_t  size;
  size_t  len;
};

void
string_append(struct string *str, char c)
{
  if (str->len < str->size) {
    str->s[str->len++] = c;
  }
}

int
string_cmp(struct string *a, char *b, size_t blen)
{
  if (a->len > blen) {
    return 1;
  } else if (a->len < blen) {
    return -1;
  } else {
    return memcmp(a->s, b, blen);
  }
}

void
string_cpy(struct string *dst, struct string *src)
{
  if (dst->size < src->len) {
    dst->len = dst->size;
  } else {
    dst->len = src->len;
  }
  memcpy(dst->s, src->s, dst->len);
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
read_hex(FILE *f)
{
  int  a   = fgetc(f);
  int  b   = fgetc(f);

  return tonum(a)*16 + tonum(b);
}

/* Read a key or a value.  Since & and = aren't supposed to appear
   outside of boundaries, we can use the same function for both.
*/
int
read_item(FILE *f, struct string *str)
{
  int c;

  str->len = 0;

  while (1) {
    c = fgetc(f);
    switch (c) {
      case EOF:
        return 0;
        break;
      case '=':
      case '&':
        return 1;
        break;
      case '%':
        string_append(str, read_hex(f));
        break;
      default:
        string_append(str, c);
        break;
    }
  }
}

int
read_pair(FILE *f, struct string *key, struct string *val)
{
  if (! read_item(f, key)) {
    return 0;
  }
  return read_item(f, val);
}

/* This is ugly and I dislike it. */
#define new_string(name, size)                  \
  char _##name[size];                           \
  struct string name = {_##name, size, 0 }

int
main(int argc, char *argv[])
{
  int sensor[10][4];

  new_string(key, 20);
  new_string(val, 8192);
  new_string(token, 40);
  new_string(name, 20);
  new_string(author, 60);
  new_string(color, 10);
  new_string(program, 8192);

  printf("Content-type: text/plain\n\n");

  while (! feof(stdin)) {
    read_pair(stdin, &key, &val);
    if (0 == string_cmp(&key, "token", 5)) {
      string_cpy(&token, &key);
    } else if (0 == string_cmp(&key, "name", 4)) {
      string_cpy(&name, &key);
    } else if (0 == string_cmp(&key, "author", 6)) {
      string_cpy(&author, &key);
    } else if (0 == string_cmp(&key, "color", 5)) {
      string_cpy(&color, &key);
    } else if (0 == string_cmp(&key, "program", 7)) {
      string_cpy(&program, &key);
    } else if ((3 == key.len) && ('s' == key.s[0])) {
      /* sensor dealie, key = "s[0-9][rawt]" */
      int n = key.s[1] - '0';
      int i;
      int p;

      if (! (n >= 0) && (n <= 9)) {
        break;
      }
      if (val.len > 3) {
        break;
      }
      val.s[val.len] = '\0';
      i = atoi(val.s);

      switch (key.s[2]) {
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
          break;
      }

      sensor[n][p] = i;
    }
    write(1, key.s, key.len);
    write(1, "=", 1);
    write(1, val.s, val.len);
    write(1, "\n", 1);
  }

  return 0;
}
