main: compile
	./test
compile:
	g++ src/${F} -o test -pthread -O3 -D TEST
cmp:
	python utils/cmp.py test_output.txt resources/result.txt
cmp-pre:
	python utils/cmp.py test_output.txt resources/pre_result.txt
gen:
	g++ src/generator/${F} -o test -O3
	./test > gen_data.txt
stable:
	g++ -O3 src/stable/${F} -o test -pthread
	./test
clean:
	# only for git bash
	rm *.exe -f
	rm test_output.txt -f
	rm log.txt -f