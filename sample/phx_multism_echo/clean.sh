  
for path in `find ~/phxpaxos/sample/phx_multism_echo -regex ".*log.*"`
do
    rm -rf $path
done

mkdir log