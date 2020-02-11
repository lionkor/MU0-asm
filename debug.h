#ifndef DEBUG_H
#define DEBUG_H

#include <iostream>

// some ansi color codes
#define ansi_reset "\u001b[0m"
#define ansi_red "\u001b[31m"
#define ansi_gray "\u001b[38;5;8m"

// error macro to report errors, using std::dec to avoid printing line in hex
//  since for some reason that can happen when x specifies it
#define error(x) std::cerr << __FUNCTION__ << ":" << std::dec << __LINE__ << ": " \
                           << ansi_red << "error: " << ansi_reset << x << std::endl
#define fatal(x) std::cerr << __FUNCTION__ << ":" << std::dec << __LINE__ << ": " \
                           << ansi_red << "fatal: " << ansi_reset << x << std::endl
#if 0
#define log(x) std::cout << __FUNCTION__ << ":" << std::dec << __LINE__ << ": log: " \
                         << x << std::endl
#define verbose(x) std::cout << ansi_gray << __FUNCTION__ << ":" << std::dec << __LINE__ << ": verbose: " \
                             << x << ansi_reset << std::endl
#else
#define log(x)
#define verbose(x)
#endif

// macro to turn an expression into a string, for debugging purposes
#define nameof(x) #x

#endif // DEBUG_H
