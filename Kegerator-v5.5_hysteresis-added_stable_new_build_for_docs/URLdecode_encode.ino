// Module to en/decode the URL-encoded String that we get when user "submits" web form
// single (simple) function

char specials[] = "$&+,/:;=?@ <>#%{}|~[]`\"\'\\"; ///* String containing chars you want encoded */

void urldecode2(const char *src, char *dst)
{
  char a, b;
  while (*src) {
    if ((*src == '%') &&
        ((a = src[1]) && (b = src[2])) &&
        (isxdigit(a) && isxdigit(b))) {
      if (a >= 'a') a -= 'a' - 'A';
      if (a >= 'A') a -= ('A' - 10);
      else
        a -= '0';
      if (b >= 'a') b -= 'a' - 'A';
      if (b >= 'A') b -= ('A' - 10);
      else
        b -= '0';
      *dst++ = 16 * a + b;
      src += 3;
    } else if ( *src == '+' ) {
      *dst++ = ' ';
      *src++;
    }
    else {
      *dst++ = *src++;
    }
  }
  *dst++ = '\0';
}


static char hex_digit(char c)
{ return "0123456789ABCDEF"[c & 0x0F];
}
char *urlencode(const char *src, char *dst)
{ char c, *d = dst;
  while (c = *src++)
  { if (strchr(specials, c))
    { *d++ = '%';
      *d++ = hex_digit(c >> 4);
      c = hex_digit(c);
    }
    *d++ = c;
  }
  *d = '\0';
  return dst;
}








