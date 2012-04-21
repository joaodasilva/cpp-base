#ifndef BASE_FILE_H
#define BASE_FILE_H

#include "base/base.h"

// A class that stores and manages a file descriptor.
class FileDescriptor {
 public:
  explicit FileDescriptor(int fd) : fd_(fd) {}
  virtual ~FileDescriptor();

  int fd() const { return fd_; }

  bool SetNonBlocking();
  bool SetCloseOnExec();

 private:
  int fd_;

  DISALLOW_COPY_AND_ASSIGN(FileDescriptor);
};

#endif  // BASE_FILE_H
