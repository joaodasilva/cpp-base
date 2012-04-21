#include "base/file.h"

#include <fcntl.h>
#include <unistd.h>

#include "base/logging.h"

FileDescriptor::~FileDescriptor() {
  close(fd_);
}

bool FileDescriptor::SetNonBlocking() {
  if (fcntl(fd_, F_SETFL, fcntl(fd_, F_GETFL, 0) | O_NONBLOCK) == -1) {
    DLOGE(ERROR) << "failed to set non-blocking";
    return false;
  }
  return true;
}

bool FileDescriptor::SetCloseOnExec() {
  if (fcntl(fd_, F_SETFD, fcntl(fd_, F_GETFD, 0) | FD_CLOEXEC) == -1) {
    DLOGE(ERROR) << "failed to set close-on-exec";
    return false;
  }
  return true;
}
