#include "fsnotify.h"
#include "reactor.h"
#include "macros.h"
#include "mblock.h"
#include "socket.h"

#include <unistd.h>

fsnotify::fsnotify() :
  inotify_fd_(-1),
  read_buff_(new mblock((sizeof(struct inotify_event) + 32/*name len*/) * 16))
{ }
fsnotify::~fsnotify()
{ 
  this->close();
  this->read_buff_->release();
}
int fsnotify::open(reactor *r)
{
  this->set_reactor(r);
  this->inotify_fd_ = ::inotify_init();

  ::fcntl(this->inotify_fd_, F_SETFD, FD_CLOEXEC);
  socket::set_nonblock(this->inotify_fd_);

  if (this->get_reactor()->register_handler(this, ev_handler::read_mask) != 0)
  {
    this->close();
    return -1;
  }
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

  return ::inotify_add_watch(this->inotify_fd_, name, mask);
}
int fsnotify::rm_watch(const int wd)
{
  if (wd < 0) return -1;

  return ::inotify_rm_watch(this->inotify_fd_, wd);
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
          if (this->handle_fsmodify(event->name) < 0)
            this->rm_watch(event->wd);
        }else
        {
          if (this->handle_fsmodify("") < 0)
            this->rm_watch(event->wd);
        }
      }
    }
    this->read_buff_->rd_ptr(sizeof(struct inotify_event) + event->len);
  }
  return 0;
}
