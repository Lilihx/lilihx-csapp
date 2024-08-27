#include "cachelab.h"
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

const unsigned Miss = 0;
const unsigned Hit = 1;
const unsigned Eviction = 2;

typedef struct _CacheLine
{
  unsigned last_used;
  unsigned valid;
  unsigned tag;
  int size;
}CacheLine;

CacheLine* newCacheLine(int B)
{
  CacheLine* line = (CacheLine*)malloc(sizeof(CacheLine));
  line->size = pow(2, B);
  line->valid = 0;
  line->tag = 0;
  return line;
}

unsigned load(CacheLine* line, unsigned tag, unsigned *timer)
{
  if (line->valid == 1 && line->tag == tag)
  {
    line->last_used = (*timer)++;
    return 1;
  }
  else
  {
    return 0;
  }
}

void store(CacheLine* line, unsigned tag, unsigned *timer)
{
  line->valid = 1;
  line->tag = tag;
  line->last_used = (*timer)++;
}


typedef struct _CacheSet
{
  int size;
  unsigned timer;
  CacheLine** lines;
}CacheSet;

CacheSet* newCacheSet(int M, int B)
{
  CacheSet* set = (CacheSet*)malloc(sizeof(CacheSet));
  set->size = M;
  set->timer = 0;
  set->lines = (CacheLine**)malloc(sizeof(CacheLine*) * M);
  for (int i = 0; i < M; i++)
  {
    set->lines[i] = newCacheLine(B);
  }
  return set;
}
/*
 * lruEvict : lru
 * */
unsigned lru(CacheSet* set)
{
  unsigned index = 0;
  unsigned timer = set->lines[0]->last_used;
  for (int i = 0; i < set->size; i++)
  {
    if (set->lines[i]->valid == 0) {
      return i;
    }
    if (set->lines[i]->last_used < timer) {
      timer = set->lines[i]->last_used;
      index = i;
    }
  }
  return index;
}

unsigned loadSet(CacheSet* set, unsigned tag)
{
  // Hit
  for (int i = 0; i < set->size; i++)
  {
    if (load(set->lines[i], tag, &set->timer))
    {
      return Hit;
    }
  }
  // Miss
  for (int i = 0; i < set->size; i++)
  {
    if (set->lines[i]->valid == 0)
    {
      store(set->lines[i], tag, &set->timer);
      return Miss;
    }
  }
  // Eviction
  unsigned index = lru(set);
  store(set->lines[index], tag, &set->timer);
  return Eviction;
}

typedef struct _Cache 
{
  int S;
  CacheSet** sets;
} Cache;

Cache* newCache(int S, int M, int B)
{
  Cache* cache = (Cache*)malloc(sizeof(Cache));
  cache->S = pow(2, S);
  cache->sets = (CacheSet**)malloc(sizeof(CacheSet*) * cache->S);
  for (int i = 0; i < cache->S; i++)
  {
    cache->sets[i] = newCacheSet(M, B);
  }
  return cache;
}

unsigned loadCache(Cache* cache, unsigned tag)
{
  unsigned index = tag % cache->S;
  return loadSet(cache->sets[index], tag);  
}



int main(int argc, char** argv)
{
  int S, E, B, c;
  char* trace_file;
  Cache* cache;
  while ((c = getopt(argc, argv, "s:E:b:t:")) != -1)
  {
    switch (c) 
    {
      case 's':
        S = atoi(optarg);
        break;
      case 'E':
        E = atoi(optarg);
        break;
      case 'b':
        B = atoi(optarg);
        break;
      case 't':
        trace_file = optarg;
        break;
      default:
        break;
    }
  }
    cache = newCache(S, E, B);
    FILE* trace = fopen(trace_file, "r");
    char type;
    unsigned size, tag;
    unsigned addr;
    unsigned hits = 0, misses = 0, evictions = 0;
    while (fscanf(trace, " %c %x,%x", &type, &addr, &size) != EOF)
    {
      int m = 0;
      char s[30];
      tag = addr >> B;
      if (type == 'I')
      {
        continue;
      }
      else if (type == 'M')
      {
        m = 1;
      }
     switch (loadCache(cache, tag))
     {
       case 0:
         strcpy(s, "miss");
         misses ++;
         break;
       case 2:
        strcpy(s, "miss eviction");
         evictions++;
         misses ++;
        break;
       case 1:
         strcpy(s, "hit");
         hits++;
        break;
     }
     if (m == 1)
     {
       hits++;
       strcat(s, " hit");
     }
     printf("%c %x,%x, %s\n", type, addr, size, s);   
  }
    printSummary(hits, misses,evictions);
    return 0;
}
