#ifndef GENERATOR_H_
#define GENERATOR_H_

// generator/continuation for C++
// author: Andrew Fedoniouk @ terrainformatica.com
// idea borrowed from: "coroutines in C" Simon Tatham,
//   http://www.chiark.greenend.org.uk/~sgtatham/coroutines.html

struct _generator
{
  int _line;
  _generator():_line(0) {}
};

#define $generator(NAME) struct NAME : public _generator

#define $emit(T) bool operator()(T& _rv) { \
                    switch(_line) { case 0:;

#define $stop  } _line = 0; return false; }

#define $yield(...)     \
        do {\
            _line=__LINE__;\
            _rv = (__VA_ARGS__); return true; case __LINE__:;\
        } while (0)


#endif /* GENERATOR_H_ */
