#ifndef TYPEDEFS_H_INCLUDED
#define TYPEDEFS_H_INCLUDED

#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))
#define FILLARRAY(x,y)  std::fill(x, NELEMS(x), y)

typedef unsigned int    id_t;   // thing id
typedef int          coord_t;
typedef unsigned int floor_t;
enum class heading_t : unsigned char {
    NORTH = 0,
    NORTHEAST,
    EAST,
    SOUTHEAST,
    SOUTH,
    SOUTHWEST,
    WEST,
    NORTHWEST
};


#endif // TYPEDEFS_H_INCLUDED
