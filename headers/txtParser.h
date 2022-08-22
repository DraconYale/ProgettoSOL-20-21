#ifndef TXTPARSER_H_DEFINED
#define TXTPARSER_H_DEFINED

typedef struct txtFile txtFile;

txtFile* txtInit();

int applyConfig(txtFile* conf, const char* pathToConfig);

int cleanconf(txtFile* conf));

#endif
