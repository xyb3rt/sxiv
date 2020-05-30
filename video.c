#include "sxiv.h"

// I know... Dirty methods... If you have better ideas
// pull requests are more than welcomed

bool is_video(const char *file) {
#if 1
  // because the other way is extreamly slow...
  // TODO: find a proper way
  const char *ext = file + strlen(file) - 4;
  return
    strcmp(".mp4", ext) == 0 ||
    strcmp(".mkv", ext) == 0;
#else
  bool res = false;
  const char *tmpl = "file -b --mime-type '%s'";
  char *command = (char *) malloc(strlen(file) + strlen(tmpl) + 1);
  sprintf(command, tmpl, file);

  char buffer[128];
  FILE *fpipe = (FILE *) popen(command, "r");
  if (fpipe) {
    while (fgets(buffer, sizeof buffer, fpipe) != NULL);
    char *startswith = "video";
    res = strncmp(startswith, buffer, strlen(startswith)) == 0;
  }

  free(command);
  pclose(fpipe);
  return res;
#endif
}

char *get_video_thumb(char *file) {
  const char *script = "f=\"%s\" \n"
    "t=$(echo \"$f\" | md5sum | awk '{print $1}') \n"
    "t=\"$HOME/.cache/sxiv/vid-$t.jpg\" \n"
    "if [ ! -f \"$t\" ] || [ \"$t\" -ot \"$f\" ]; then \n"
    "    rm -f \"$t\" \n"
    "    ffmpegthumbnailer -i \"$f\" -o \"$t\" -s 0 >/dev/null 2>/dev/null || exit 1 \n"
    "fi \n"
    "printf '%%s' \"$t\" \n";
  char *command = (char *) malloc(strlen(file) + strlen(script) + 1);
  sprintf(command, script, file);

  size_t buffer_size = 1024;
  char *buffer = (char *) malloc(buffer_size);
  FILE *fpipe = (FILE *) popen(command, "r");
  if (fpipe) {
    while (fgets(buffer, buffer_size, fpipe) != NULL);
  }

  free(command);
  pclose(fpipe);
  return buffer;
}
