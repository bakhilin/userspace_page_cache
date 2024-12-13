gcc -o cache_test src/cache.c src/main.c -lcunit
./cache_test

gcc -c -fPIC src/cache.c -o cache.o
gcc -shared cache.o -o cache.so