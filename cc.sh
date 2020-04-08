g++ -O3 src/search/stl-search.cpp -o main -lpthread
g++ -O3 src/search/search.cc -o test -lpthread -D TEST
# echo "* stl search"
# ./main
# echo "* search"
# ./test
# echo "* cmp"
# python utils/cmp.py stl_brute_output.txt search_output.txt