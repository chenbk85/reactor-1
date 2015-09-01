#include "reactor.h"
#include "svc_handler.h"
#include "fsnotify.h"
#include <acceptor.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <cassert>
#include <signal.h>

class client : public fsnotify
{
public:
  virtual int handle_fs_close_write(const int wd, const char *name)
  {
    fprintf(stderr, "name = %s!\n", this->pathname(wd));
    return 0;
  }
  virtual int handle_close(const int , reactor_mask m)
  {
    delete this;
    return 0;
  }
};
int main()
{
  int port = 7777;

  signal(SIGPIPE, SIG_IGN);

  reactor r;
  int ret = r.open(1024, 128);
  if (ret != 0)
  {
    fprintf(stderr, "open failed![%s]\n", strerror(errno));
    return -1;
  }

  client fs;
  ret = fs.open(64, &r);
  if (ret == -1)
  {
    fprintf(stderr, "fs open failed![%s]\n", strerror(errno));
    return -1;
  }
  fs.add_watch("/tmp/", fsnotify::FS_IN_CLOSE_WRITE|fsnotify::FS_IN_ONLYDIR);

  r.run_reactor_event_loop();
  return 0;
}
