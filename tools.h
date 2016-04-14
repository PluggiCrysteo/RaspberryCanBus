#include <string>
#include <sstream>
#include <vector>
#include <errno.h>
#include <unistd.h>

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
