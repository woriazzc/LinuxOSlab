export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH
make test_counter
make test_list
make test_hash
./counter_test > counter_res
./list_test 1 > list_res1
./list_test 2 > list_res2
./list_test 3 > list_res3
./hash_test 1 > hash_res1
./hash_test 2 > hash_res2
./hash_test 3 > hash_res3
./hash_test 4 > hash_res4
./hash_test 5 > hash_res5
./hash_test 6 > hash_res6
