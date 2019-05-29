#include <iostream>
#include <stdio.h>

void compress(const char* infile, const char* outfile) {
  FILE *fd = fopen(infile, "r");
  if (fd == nullptr) return;
  
  fseek(fd, 0L, SEEK_END);
  auto filesize = ftell(fd);

  fseek(fd, 0L, SEEK_SET);
  char* buf = (char*)malloc(filesize * sizeof(char));
  fread(buf, 1, filesize, fd);

  // You can make the offset
  for (int begin = 0, end = filesize - 1; begin < end; ++begin, --end) {
    std::swap(buf[begin], buf[end]);
  }
  fclose(fd);

  FILE* fd_out = fopen(outfile, "w");
  if (fd_out == nullptr) return;

  fwrite(buf, 1, filesize, fd_out);
  free(buf);
  fclose(fd_out);
}

/*
 * The compress and uncompress tools.
 */
int main(int argc, char** argv) {
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " infile outfile" << std::endl;
    return -1;
  }
  auto infile = argv[1];
  auto outfile = argv[2];
  compress(infile, outfile);
  return 0;
}
