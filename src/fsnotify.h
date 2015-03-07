// -*- C++ -*-

//========================================================================
/**
 * Author   : cuisw
 * Date     : 2012-04-26 16:56
 */
//========================================================================

#ifndef FSNOTIFY_H_
#define FSNOTIFY_H_

#include "ev_handler.h"
#include <sys/inotify.h>

// Forward declarations
class reactor;
class mblock;

/**
 * @class fsnotify
 *
 * @brief 
 */
class fsnotify : public ev_handler
{
public:
  enum
  {
    FS_IN_CLOSE_WRITE  = IN_CLOSE_WRITE,
    FS_IN_ONLYDIR      = IN_ONLYDIR,
  };
  fsnotify();
  ~fsnotify();

  int open(reactor *r);
  void close();

  // On success, return a non-negative watch descriptor
  // On error return -1
  int add_watch(const char *dirname, unsigned int mask);

  // On success, return zero, or -1 if an error occurred
  int rm_watch(const int wd);

  //= virtual method
  virtual int get_handle() const { return this->inotify_fd_; }
  virtual int handle_input(const int );

  //= 
  // 'rm_watch' will be called if below handle_* return -1. 
  virtual int handle_fs_close_write(const char * /*name*/) { return -1; }
private:
  int inotify_fd_;
  mblock *read_buff_;
};
#endif // FSNOTIFY_H_

