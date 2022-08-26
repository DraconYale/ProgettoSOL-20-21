#ifndef BOUNDEDBUFFER_H_DEFINED
#define BOUNDEDBUFFER_H_DEFINED

typedef struct boundedBuffer boundedBuffer;

boundedBuffer* initBuffer(int size);

int enqueueBuffer(boundedBuffer* buf, char* job);

char* dequeueBuffer(boundedBuffer* buf);

int cleanBuffer(boundedBuffer* buf);

#endif
