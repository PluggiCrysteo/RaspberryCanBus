#include <string>
#include <sstream>
#include <vector>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <error.h>

#include "debug.h"

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
	std::stringstream ss(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
	return elems;
}


std::vector<std::string> split(const std::string &s, char delim) {
	std::vector<std::string> elems;
	split(s, delim, elems);
	return elems;
}

int readline(int fd, char *buf, int nbytes) {
	int numread = 0;
	int returnval;
	while (numread < nbytes - 1) {
		returnval = read(fd, buf + numread, 1);
		/* If the read() syscall was interrupted by a signal */
		if ((returnval == -1) && (errno == EINTR))
			continue;
		/* A value of returnval equal to 0 indicates end of file */
		if ( (returnval == 0) && (numread == 0) )
			return 0;
		if (returnval == 0)
			break;
		if (returnval == -1)
			return -1;
		numread++;
		if (buf[numread-1] == '\n') {
			/* make this buffer a null-termianted string */
			buf[numread] = '\0';
			return numread;
		}
	}
	/* set errno to "invalid argument" */
	errno = EINVAL;
	return -1;
}

int get_unix_socket_fd(const char* socket_path){
  struct sockaddr_un addr;
  int sfd;
  int euid = geteuid();
  seteuid(getuid());
  sfd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sfd == -1)
    error(1,errno,"socket");

  /* Construct server socket address, bind socket to it,
     and make this a listening socket */

  if (remove(socket_path) == -1 && errno != ENOENT)
    error(1,errno,"remove-%s", socket_path);

  memset(&addr, 0, sizeof(struct sockaddr_un));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

  if (bind(sfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) == -1)
    error(1,errno,"bind");

  if (listen(sfd, 1) == -1)	// backlog = 1 because its synch server
    error(1,errno,"listen");

  seteuid(euid);
  return sfd;
}
