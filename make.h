#ifndef DMAKE_MAKE_H
#define DMAKE_MAKE_H

#include "makefile.h"

#include <iostream>

#define DEBUG 1

class Make {
	protected:
	private:

	public:
		Make() { }
		virtual ~Make() { }

		virtual void run(Makefile makefile) = 0;
		virtual void run(Makefile makefile, std::string startRule) = 0;
};

#endif
