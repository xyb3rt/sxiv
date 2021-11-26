#include "sxiv.h"
#include <ctype.h>

// I know... Dirty methods... If you have better ideas
// pull requests are more than welcomed

#if HAVE_LIBMAGIC
#include <magic.h>
extern magic_t magic;
#endif

const char *video_types[] = {
  ".mp4", ".3gp", ".mkv", ".avi",
  ".flv", ".mov", ".mwv"
};

bool is_video(const char *file) {
  size_t flen = strlen(file);
  // extension check
  if (flen >= 4) {
    static char ext[4];
    const char *ext_ptr = file + flen - 4;
    strcpy(ext, ext_ptr);
    for (char *p = ext; *p; ++p) *p = tolower(*p);
    for (size_t i = 0; i < sizeof(video_types) / sizeof(video_types[0]); ++i) {
      if (strcmp(video_types[i], ext) == 0) {
        return true;
      }
    }
  }
#if HAVE_LIBMAGIC
  // mimetype check
  const char *mime = magic_file(magic, file);
  return strncmp(mime, "video", 5) == 0;
#else
  return false;
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
