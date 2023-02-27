#include "stddef.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "unistd.h"

typedef struct {
  size_t user, nice, system, idle, iowait, irq, softirq, steal, guest;
  char name[10];
} cpuinfo;

typedef struct {
  size_t total, free, available, used, buffers, cached, swap_total, swap_free,
      swap_cached;
} meminfo;

enum unit { B, KB, MB };

typedef struct {
  size_t sent, received;
  float sentf, receivedf;
  enum unit s_unit, r_unit;
} netinfo;

void print_stat(cpuinfo *s) {
  printf("user: %zu, nice: %zu, system: %zu, iowait: %zu, softirq: %zu, guest: "
         "%zu\n",
         s->user, s->nice, s->system, s->iowait, s->softirq, s->guest);
}

void print_mem(meminfo *mem) {
  printf("total: %zu, free: %zu, available: %zu, buffers: %zu, cached: %zu, "
         "swap_total: %zu, swap_free: %zu, swap_cached: %zu\n ",
         mem->total, mem->free, mem->available, mem->buffers, mem->cached,
         mem->swap_total, mem->swap_free, mem->swap_cached);
}

void print_netinfo(netinfo *net) {
  printf("sent: %zu, received: %zu, sentf: %f, receivedf: %f, s_unit: %d, "
         "r_unit: %d\n",
         net->sent, net->received, net->sentf, net->receivedf, net->s_unit,
         net->r_unit);
}

int get_stat(cpuinfo *out) {
  char *line = NULL;
  ssize_t res;
  int ret = EXIT_SUCCESS;
  FILE *stat;

  stat = fopen("/proc/stat", "r");
  if (stat == NULL)
    return EXIT_FAILURE;
  if (fscanf(stat, "%s %zu %zu %zu %zu %zu %zu %zu %zu %zu", out->name,
             &out->user, &out->nice, &out->system, &out->idle, &out->iowait,
             &out->irq, &out->softirq, &out->steal, &out->guest) == EOF)
    ret = EXIT_FAILURE;
  fclose(stat);

  if (line)
    free(line);

  return ret;
}

void stat_sub(cpuinfo *i1, cpuinfo *i2, cpuinfo *out) {
  if (i1 == NULL || i2 == NULL)
    return;
  if (out == NULL)
    out = i2;
  out->user = i1->user - i2->user;
  out->nice = i1->nice - i2->nice;
  out->system = i1->system - i2->system;
  out->idle = i1->idle - i2->idle;
  out->iowait = i1->iowait - i2->iowait;
  out->irq = i1->irq - i2->irq;
  out->softirq = i1->softirq - i2->softirq;
  out->steal = i1->steal - i2->steal;
  out->guest = i1->guest - i2->guest;
}

size_t to_mB(size_t value, char *unit) {
  if (unit == NULL)
    return value;
  if (strcmp(unit, "kB") == 0)
    return value / 1000;
  if (strcmp(unit, "B") == 0)
    return value / (1000000);
  if (strcmp(unit, "mB") == 0)
    return value;
  if (strcmp(unit, "kb") == 0)
    return value / 8000;
  if (strcmp(unit, "b") == 0)
    return value / (8000000);
  if (strcmp(unit, "mb") == 0)
    return value / 8;
  if (strcmp(unit, "KB") == 0)
    return value * 1.024 / 1000;
  if (strcmp(unit, "MB") == 0)
    return value * 1.024 * 1.024;
  if (strcmp(unit, "Kb") == 0)
    return value * 1.024 / 8000;
  if (strcmp(unit, "Mb") == 0)
    return value * 1.024 * 1.024 / 8;
  return value;
}

int get_meminfo(meminfo *memory) {
  char *line = NULL;
  char key[128], unit[5];
  size_t value;
  ssize_t res;
  int ret = EXIT_SUCCESS;
  FILE *mem;

  mem = fopen("/proc/meminfo", "r");
  if (mem == NULL)
    return EXIT_FAILURE;
  while (fscanf(mem, "%s %zu %s", key, &value, unit) != EOF) {
    if (!memory->total && strcmp(key, "MemTotal:") == 0) {
      value = to_mB(value, unit);
      memory->total = value;
    } else if (!memory->free && strcmp(key, "MemFree:") == 0) {
      value = to_mB(value, unit);
      memory->free = value;
    } else if (!memory->available && strcmp(key, "MemAvailable:") == 0) {
      value = to_mB(value, unit);
      memory->available = value;
    } else if (!memory->buffers && strcmp(key, "Buffers:") == 0) {
      value = to_mB(value, unit);
      memory->buffers = value;
    } else if (!memory->cached && strcmp(key, "Cached:") == 0) {
      value = to_mB(value, unit);
      memory->cached = value;
    } else if (!memory->swap_cached && strcmp(key, "SwapCached:") == 0) {
      value = to_mB(value, unit);
      memory->swap_cached = value;
    } else if (!memory->swap_total && strcmp(key, "SwapTotal:") == 0) {
      value = to_mB(value, unit);
      memory->swap_total = value;
    } else if (!memory->swap_free && strcmp(key, "SwapFree:") == 0) {
      value = to_mB(value, unit);
      memory->swap_free = value;
    }
  }
  memory->used = memory->total - memory->available;

  fclose(mem);

  if (line)
    free(line);

  return ret;
}

int get_netinfo(netinfo *net) {
  char *line = NULL;
  ssize_t res;
  char name[20];
  size_t sent, received;
  int ret = EXIT_SUCCESS;
  FILE *mem;

  mem = fopen("/proc/net/dev", "r");
  if (mem == NULL)
    return EXIT_FAILURE;
  if (fscanf(mem, "%*[^\n]\n") == EOF)
    return EXIT_FAILURE;
  if (fscanf(mem, "%*[^\n]\n") == EOF)
    return EXIT_FAILURE;
  while (fscanf(mem,
                "%s %zu %*u %*u %*u %*u %*u %*u %*u %zu %*u %*u %*u "
                "%*u %*u %*u %*u",
                name, &received, &sent) != EOF) {
    if (strncmp(name, "eno", 3) == 0) {
      net->received += received;
      net->sent += sent;
    }
  }

  fclose(mem);

  if (line)
    free(line);

  return ret;
}

void netinfo_sub(netinfo *n1, netinfo *n2, netinfo *out) {
  if (n1 == NULL || n2 == NULL)
    return;
  if (out == NULL)
    out = n2;
  out->sent = n1->sent - n2->sent;
  out->received = n1->received - n2->received;
}

float cpu_usage_p(cpuinfo *s) {
  double num = (s->user + s->system + s->nice + s->softirq + s->steal);
  return num / (num + s->idle + s->iowait) * 100;
}

float cpu_io_p(cpuinfo *s) {
  double deno = (s->user + s->system + s->nice + s->softirq + s->steal +
                 s->idle + s->iowait);
  return (s->iowait) / deno * 100;
}

float mem_p(meminfo *mem) { return (float)mem->used / mem->total * 100; }

void bytes_to_readable(netinfo *n) {
  if (n->sent > 1000 * 1000) {
    n->sentf = n->sent / (float)(1024 * 1024);
    n->s_unit = MB;
  } else if (n->sent > 1000) {
    n->sentf = n->sent / (float)(1024);
    n->s_unit = KB;
  } else
    n->s_unit = B;
  if (n->received > 1000 * 1000) {
    n->receivedf = n->received / (float)(1024 * 1024);
    n->r_unit = MB;
  } else if (n->received > 1000) {
    n->receivedf = n->received / (float)(1024);
    n->r_unit = KB;
  } else
    n->r_unit = B;
}

int main(void) {
  cpuinfo prev_stat = {}, stat = {};
  meminfo memory = {};
  netinfo prev_net = {}, net = {};
  char output[50];
  if (get_stat(&prev_stat) != EXIT_SUCCESS)
    exit(EXIT_FAILURE);
  usleep(100000);
  if (get_stat(&stat) != EXIT_SUCCESS)
    exit(EXIT_FAILURE);
  stat_sub(&stat, &prev_stat, &stat);
  if (get_meminfo(&memory) != EXIT_SUCCESS)
    exit(EXIT_FAILURE);
  if (get_netinfo(&prev_net) != EXIT_SUCCESS)
    exit(EXIT_FAILURE);
  usleep(100000);
  if (get_netinfo(&net) != EXIT_SUCCESS)
    exit(EXIT_FAILURE);
  netinfo_sub(&net, &prev_net, &net);
  sprintf(output, "%.2f%%", cpu_usage_p(&stat));
  printf("CPU: %-7s", output);
  // sprintf(output, "%.2f%%", cpu_io_p(&stat));
  // printf(" IO: %-7s", output);
  sprintf(output, "%.2f%% [%zuG/%zuG]", mem_p(&memory), memory.used / 1000,
          memory.total / 1000);
  printf(" MEM: %-16s", output);
  bytes_to_readable(&net);
  sprintf(output, "%.2f%4s", net.sentf,
          net.s_unit == B ? "B/s" : net.s_unit == KB ? "KB/s" : "MB/s");
  printf(" UP: %-10s", output);
  sprintf(output, "%.2f%4s", net.receivedf,
          net.r_unit == B ? "B/s" : net.r_unit == KB ? "KB/s" : "MB/s");
  printf(" DOWN: %-10s", output);
}
