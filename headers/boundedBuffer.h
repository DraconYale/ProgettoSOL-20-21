#ifndef BOUNDEDBUFFER_H_DEFINED
#define BOUNDEDBUFFER_H_DEFINED

typedef struct boundedBuffer boundedBuffer;

boundedBuffer* initBuffer(int size);

int enqueueBuffer(boundedBuffer* buf, char* job);

int dequeueBuffer(boundedBuffer* buf, char** retjob);

int cleanBuffer(boundedBuffer* buf);

#endif
