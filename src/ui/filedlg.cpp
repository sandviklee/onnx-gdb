#include "ui/filedlg.h"
#include <cstdio>
#include <cstring>

std::string open_onnx_file_dialog() {
#ifdef __APPLE__
  const char *script =
      "try\n"
      "set f to POSIX path of (choose file with prompt \"Open ONNX Model\")\n"
      "f\n"
      "on error\n"
      "\"\"\n"
      "end try\n";

  const char *tmp = "/tmp/romll_pick.scpt";
  FILE *sf = fopen(tmp, "w");
  if (!sf)
    return "";
  fputs(script, sf);
  fclose(sf);

  FILE *fp = popen("osascript /tmp/romll_pick.scpt 2>/dev/null", "r");
  if (!fp)
    return "";

  char buf[4096] = {};
  bool got = (fgets(buf, sizeof(buf), fp) != nullptr);
  pclose(fp);

  if (!got)
    return "";

  std::string path(buf);
  if (path.size() < 5 || path.compare(path.size() - 5, 5, ".onnx") != 0) {
    return ""; // or show an error
  }
  return path;

#else
  return "";
#endif
}
