#ifndef BOUNDEDBUFFER_H_DEFINED
#define BOUNDEDBUFFER_H_DEFINED

typedef struct txtFile txtFile;

txtFile* txtInit();

int applyConfig(txtFile* conf, const char* pathToConfig);

int cleanconf(txtFile* conf));

#endif
