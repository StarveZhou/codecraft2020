main:
	g++ src/$(F).cc -o test -lpthread -O3
	./test
cmp:
	python utils/cmp.py test_output.txt resources/result.txt