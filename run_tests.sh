set -e

# For each file in hw5_tests, run the test and print the result
for file in hw5_tests/*.in
do
    echo "Running test $file"
    # Remove extension
    baseName="hw5_tests/$(basename "$file" .in)"
    # Run and ignore status code
    ./hw5 < $file > $baseName.res 2| echo ""
    diff $baseName.out $baseName.res 2>&1
done

echo "All tests done. Good job!"
