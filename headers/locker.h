#ifndef LOCKER_H_DEFINED
#define LOCKER_H_DEFINED

typedef struct locker locker;

locker* lockerInit();

int readLock(locker* lock);

int readUnlock(locker* lock);

int writeLock(locker* lock);

int writeUnlock(locker* lock);

int freeLock(locker* lock);

#endif
