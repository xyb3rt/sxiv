#!/usr/bin/awk -f

function printchars() {
  while (n > 0) {
    x = n / 16 >= 1 ? 16 : n;
    printf("0x%x%x,%s", x - 1, ref[c] - 1, ++i % 12 == 0 ? "\n" : " ");
    n -= x;
  }
}

/^$/ {
  printchars();
  printf("\n\n");
  c = "";
  i = 0;
}

/./ {
  if (!ref[$0]) {
    col[cnt++] = $0;
    ref[$0] = cnt;
  }
  if ($0 != c) {
    if (c != "")
      printchars();
    c = $0;
    n = 0;
  }
  n++;
}

END {
  for (i = 0; i < cnt; i++)
    printf("%s,%s", col[i], ++j % 4 == 0 || i + 1 == cnt ? "\n" : " ");
}
