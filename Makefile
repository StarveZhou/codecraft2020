main:
	g++ src/$(F) -o test -lpthread -O3
	./test
cmp:
	python utils/cmp.py test_output.txt resources/result.txt
cmp-pre:
	python utils/cmp.py test_output.txt resources/pre_result.txt
gen:
	g++ src/generator/${F} -o test -O3
	./test > gen_data.txt
clean:
	# only for git bash
	rm *.exe -f
	rm test_output.txt -f
	rm log.txt -f