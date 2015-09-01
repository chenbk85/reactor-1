#include "fsnotify.h"
#include "reactor.h"
#include "macros.h"
#include "mblock.h"
#include "socket.h"

#include <unistd.h>

fsnotify::fsnotify() :
  max_wd_size_(16),
  inotify_fd_(-1),
  wd_pathname_(NULL),
  read_buff_(new mblock((sizeof(struct inotify_event) + 32/*name len*/) * 16))
{ }
fsnotify::~fsnotify()
{ 
  this->close();
  this->read_buff_->release();
  if (this->wd_pathname_ != NULL)
  {
    for (int i = 0; i < this->max_wd_size_; ++i)
      delete []this->wd_pathname_[i];
    delete []this->wd_pathname_;
  }
}
int fsnotify::open(const int max_wd_size, reactor *r)
{
  this->max_wd_size_ = max_wd_size;
  if (this->max_wd_size_ < 1)
    return -1;
  this->set_reactor(r);
  this->inotify_fd_ = ::inotify_init();

  ::fcntl(this->inotify_fd_, F_SETFD, FD_CLOEXEC);
  socket::set_nonblock(this->inotify_fd_);

  if (this->get_reactor()->register_handler(this, ev_handler::read_mask) != 0)
  {
    this->close();
    return -1;
  }
  this->wd_pathname_ = new char*[this->max_wd_size_];
  for (int i = 0; i < this->max_wd_size_; ++i)
    this->wd_pathname_[i] = NULL;
  
  return 0;
}
void fsnotify::close()
{
  if (this->inotify_fd_ != -1)
  {
    ::close(this->inotify_fd_);
    this->inotify_fd_ = -1;
  }
}
int fsnotify::add_watch(const char *name, unsigned int mask)
{
  if (name == NULL || mask == 0) return -1;

  int wd = ::inotify_add_watch(this->inotify_fd_, name, mask);
  if (wd == -1)
    return -1;
  if (wd >= this->max_wd_size_)
  {
    ::inotify_rm_watch(this->inotify_fd_, wd);
    return -1;
  }
  if (this->wd_pathname_[wd] != NULL)
    ::strncpy(this->wd_pathname_[wd], name, MAX_NOTIFY_PATHNAME_LEN);
  else
  {
    this->wd_pathname_[wd] = new char[MAX_NOTIFY_PATHNAME_LEN + 1];
    ::strncpy(this->wd_pathname_[wd], name, MAX_NOTIFY_PATHNAME_LEN);
  }
  return 0;
}
int fsnotify::rm_watch(const int wd)
{
  if (wd < 0) return -1;

  int ret = ::inotify_rm_watch(this->inotify_fd_, wd);
  if (ret == -1)
    return -1;

  if (this->wd_pathname_
      && wd < this->max_wd_size_)
  {
    delete []this->wd_pathname_[wd];
    this->wd_pathname_[wd] = NULL;
  }
  return 0;
}
const char *fsnotify::pathname(const int wd)
{
  if (wd < 0
      || wd >= this->max_wd_size_
      || this->wd_pathname_ == NULL)
    return NULL;
  return this->wd_pathname_[wd];
}
int fsnotify::handle_input(const int )
{
  if (this->read_buff_->space() == 0)
  {
    int len = this->read_buff_->length();
    if (len != 0)
      ::memmove(this->read_buff_->data(), this->read_buff_->rd_ptr(), len);
    this->read_buff_->reset();
    this->read_buff_->wr_ptr(len);
  }
  int ret = ::read(this->inotify_fd_,
                   this->read_buff_->wr_ptr(), 
                   this->read_buff_->space());
  if (ret <= 0) return 0;

  this->read_buff_->wr_ptr(ret);

  //= handle event
  while (this->read_buff_->length() >= (int)sizeof(struct inotify_event))
  {
    struct inotify_event *event = (inotify_event *)this->read_buff_->rd_ptr();
    if ((int)(event->len + sizeof(struct inotify_event)) < this->read_buff_->length())
      break;
    
    if (BIT_DISABLED(event->mask, IN_IGNORED))
    {
      if (BIT_ENABLED(event->mask, FS_IN_CLOSE_WRITE))
      {
        if (BIT_ENABLED(event->mask, FS_IN_ONLYDIR))
        {
          if (this->handle_fs_close_write(event->wd, event->name) < 0)
            this->rm_watch(event->wd);
        }else
        {
          if (this->handle_fs_close_write(event->wd, "") < 0)
            this->rm_watch(event->wd);
        }
      }
    }
    this->read_buff_->rd_ptr(sizeof(struct inotify_event) + event->len);
  }
  return 0;
}
