/* SPDX-License-Identifier: GPL-3.0-or-later */
#include <stdio.h>
#include <stdlib.h>
#include "rules_text.h"
#include "probe_digest.h"
int main(int argc, char** argv) {
  if (argc != 2) { fprintf(stderr,"usage: rules-probe alpha.txt\n"); return 20; }
  FILE* file = fopen(argv[1],"rb");
  if (!file) { fprintf(stderr,"cannot open rules file\n"); return 20; }
  if (fseek(file,0,SEEK_END)) { fclose(file); return 20; }
  long length = ftell(file);
  if (length == 0) { fclose(file); fprintf(stderr,"rules: empty file\n"); return 20; }
  if (length < 0 || length > 1024*1024 || fseek(file,0,SEEK_SET)) { fclose(file); return 20; }
  uint8_t* data = (uint8_t*)malloc((size_t)length);
  if (!data) { fclose(file); return 20; }
  size_t read = fread(data,1,(size_t)length,file); fclose(file);
  static alpha::Rules state;
  if (read != (size_t)length) { free(data); return 20; }
  auto error = alpha::read_technologies(data,read,state); free(data);
  if (error != alpha::Error::ok) { fprintf(stderr,"rules: %s\n",alpha::error_name(error)); return 20; }
  printf("rows=%lu active=%lu edges=%lu digest=%08lx\n",(unsigned long)state.count,
    (unsigned long)state.active,(unsigned long)state.edges,(unsigned long)rules_digest(state));
  return 0;
}
