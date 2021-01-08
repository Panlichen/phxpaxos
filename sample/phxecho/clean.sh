  
for path in `find ~/phxpaxos/sample/phxecho -regex ".*log.*"`
do
    rm -rf $path
done

mkdir log