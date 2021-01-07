if [ -d "storage" ]; then

 rm -rf storage

fi

if [ -d "log" ]; then

 rm -rf log

fi

#create kvdb path and paxoslog path
mkdir storage
cd storage
mkdir kvdb_0
mkdir paxoslog_0
mkdir kvdb_1
mkdir paxoslog_1
mkdir kvdb_2
mkdir paxoslog_2
cd ..

#create glog path
mkdir log
