// -*- C++ -*-

//========================================================================
/**
 * Author   : cuisw
 * Date     : 2012-06-27 00:18
 */
//========================================================================

#ifndef IARRAY_H_
#define IARRAY_H_

#include <stddef.h>
#include <assert.h>

template<typename VALUE>
class iarray
{
public:
  iarray(const int size) :
    size_(size),
    obj_items_(NULL)
  {
    ::assert(size > 0);
    obj_items_ = new VALUE[size_]();
    for (int i = 0; i < size_; ++i)
      obj_items_[i] = NULL;
  }

  ~iarray()
  {
    if (this->obj_items_)
      delete []this->obj_items_;
    this->obj_items_ = NULL;
  }

  int insert(const int idx, const VALUE &si)
  {
    if (this->is_valid_idx(idx))
    {
      this->obj_items_[idx] = si;
      return 0;
    }
    return -1;
  }

  void remove(const int idx)
  {
    if (this->is_valid_idx(idx))
      this->obj_items_[idx] = NULL;
  }

  inline bool is_valid_idx(const int idx)
  { return idx < this->size_ && idx >= 0; }

  inline VALUE find(const int idx)
  { 
    if (this->is_valid_idx(idx))
      return this->obj_items_[idx];
    return NULL;
  }
  inline int size()
  { return this->size_; }
private:
  int size_; 

  VALUE *obj_items_;
};
#endif // IARRAY_H_
