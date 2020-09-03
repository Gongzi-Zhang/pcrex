#ifndef IO_H
#define IO_H

// color output
#define RESET     "\033[0m"
#define BOLD      "\033[1m"
#define DIM       "\033[2m"
#define UNDERLINE "\033[4m"
#define BLINK     "\033[5m"
#define INVERSE   "\033[7m"
#define BLACK     "\033[30m"
#define RED       "\033[31m"
#define GREEN     "\033[32m"
#define YELLOW    "\033[33m"
#define BLUE      "\033[34m"
#define MAGENTA   "\033[35m"
#define CYAN      "\033[36m"
#define WHITE     "\033[37m"
#define BOLDBLACK     "\033[1m\033[30m"
#define BOLDRED       "\033[1m\033[31m"
#define BOLDGREEN     "\033[1m\033[32m"
#define BOLDYELLOW    "\033[1m\033[33m"
#define BOLDBLUE      "\033[1m\033[34m"
#define BOLDMAGENTA   "\033[1m\033[35m"
#define BOLDCYAN      "\033[1m\033[36m"
#define BOLDWHITE     "\033[1m\033[37m"

#define OUTPUT	__PRETTY_FUNCTION__ << BOLDBLUE << "--OUTPUT\t"
#define DEBUG   __PRETTY_FUNCTION__ << GREEN << "--DEBUG\t"
#define INFO    __PRETTY_FUNCTION__ << "--INFO\t"
#define WARNING __PRETTY_FUNCTION__ << YELLOW << "--WARNING\t"
#define ALERT   __PRETTY_FUNCTION__ << BOLDYELLOW << "--ALERT\t"
#define OUTLIER __PRETTY_FUNCTION__ << BOLDYELLOW << "--OUTLIER\t"
#define GLITCH  __PRETTY_FUNCTION__ << BOLDYELLOW << "--GLITCH\t"
#define ERROR   __PRETTY_FUNCTION__ << RED << "--ERROR\t"
#define FATAL   __PRETTY_FUNCTION__ << BOLDRED << "--FATAL\t"
#define ENDL    RESET << std::endl

/*
extern void CERR(const char *format, ...) __attribute__((format(printf, 1, 2)));
extern void CERR(const char *format, ...) {
    printf(format, ...);
}
*/

#endif
/* vim: set shiftwidth=2 softtabstop=2 tabstop=2: */
