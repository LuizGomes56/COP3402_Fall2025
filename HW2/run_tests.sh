gcc lex.c -o lex || exit 1

for in in test_cases/_i*.txt; do
  n=${in##*/}      # _iN.txt
  n=${n#_i}        # N.txt
  n=${n%.txt}      # N
  ./lex "$in" > "test_cases/_r${n}.txt"
done
