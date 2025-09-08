@echo off
echo Testing ZoraVM Shell Comprehensive Suite
echo ========================================

echo.
echo Starting VM and executing test commands...

echo help > temp_input.txt
echo help grep >> temp_input.txt
echo echo "Testing basic commands" >> temp_input.txt
echo pwd >> temp_input.txt
echo ls >> temp_input.txt
echo mkdir testdir >> temp_input.txt
echo cd testdir >> temp_input.txt
echo pwd >> temp_input.txt
echo echo "Hello World" ^> test.txt >> temp_input.txt
echo cat test.txt >> temp_input.txt
echo echo "Line 1" ^> data.txt >> temp_input.txt
echo echo "Line 3" ^>^> data.txt >> temp_input.txt
echo echo "Line 2" ^>^> data.txt >> temp_input.txt
echo echo "Line 1" ^>^> data.txt >> temp_input.txt
echo sort data.txt >> temp_input.txt
echo sort data.txt ^| uniq >> temp_input.txt
echo wc data.txt >> temp_input.txt
echo cat data.txt ^| wc -l >> temp_input.txt
echo help ^| grep file >> temp_input.txt
echo which ls >> temp_input.txt
echo which nonexistent >> temp_input.txt
echo ln -s test.txt link.txt >> temp_input.txt
echo diff test.txt data.txt >> temp_input.txt
echo date >> temp_input.txt
echo uname -a >> temp_input.txt
echo tree >> temp_input.txt
echo vmstat >> temp_input.txt
echo exit >> temp_input.txt

echo Running ZoraVM with test commands...
.\zora_vm.exe < temp_input.txt > test_output.txt 2>&1

echo.
echo Test completed. Checking results...
type test_output.txt

del temp_input.txt
echo.
echo Test script completed. Check test_output.txt for full results.
